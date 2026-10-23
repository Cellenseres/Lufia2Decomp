/* Resource decompressor ($80:8E9D). */

#include "core/cpu_ops.h"
#include "lufia2/resource_format.h"
#include "lufia2/system.h"
#include "lufia2/decompress_steps.h"
#include "system/dp_scratch.h"

enum {
    STREAM = 0x5du,                     /* [$5D],Y: compressed bytes */
    LENGTH = 0x58u,
    DEST_END = 0x63u,
    FLAGS = 0x65u,                      /* control byte, bit 7 first */
    FLAGS_LEFT = 0x66u,
    COPY_COUNT = 0x56u,                 /* bytes - 1 for MVN */
};

/* INY; past $FFFF continue at $8000, next bank. */
static void StreamNext(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    IncrementY16(cpu);
    if (cpu->zero) {
        LoadY16(cpu, 0x8000u);
        IncrementDirect8(memory, cpu, 0x5fu);
    }
}

static uint8_t StreamByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, Read8IndirectLongY(memory, cpu, STREAM));
    return A8(cpu);
}

static void StreamBeginCopy(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StoreXDirect16(memory, cpu, 0x5au);
    PushY(memory, cpu);
    LoadY16(cpu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x5au));
    TransferAToX(cpu);
    LoadADirect16(memory, cpu, COPY_COUNT);
}

static void StreamMoveByte(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t bank) {
    cpu->data_bank = bank;
    const uint8_t value = Read8(memory, ((uint32_t)bank << 16) | cpu->x);
    Write8(memory, ((uint32_t)bank << 16) | cpu->y, value);
    cpu->accumulator = (uint16_t)(cpu->accumulator - 1u);
    cpu->x = (uint16_t)(cpu->x + 1u);
    cpu->y = (uint16_t)(cpu->y + 1u);
}

static void StreamFinishCopy(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, cpu->y);
    cpu->y = PullIndexValue(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $80:8F40: short or long back reference. */
static void StreamPrepareReference(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StreamNext(memory, cpu);                                   /* 8F44 */
    ExchangeAccumulatorBytes(cpu);                             /* 8F47 */
    StreamByte(memory, cpu);
    And8(cpu, 0x0fu);
    if (!cpu->zero) {
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreADirect8(memory, cpu, COPY_COUNT);
        StreamByte(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        Or16(cpu, 0xf000u);
    } else {
        StreamByte(memory, cpu);                               /* 8F6F */
        PushIndex(memory, cpu);
        TransferAToX(cpu);
        StreamNext(memory, cpu);
        StreamByte(memory, cpu);                               /* 8F76 */
        StoreADirect8(memory, cpu, 0x5bu);
        And8(cpu, 0x3fu);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 2u));
        StoreADirect8(memory, cpu, COPY_COUNT);
        SetAccumulatorWidth(cpu, 0);
        TransferXToA(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        Or16(cpu, 0xf000u);
        {
            unsigned i;

            for (i = 0; i < 2u; ++i) {                         /* ASL $5A; ROL */
                const uint16_t word = Read16Direct(memory, cpu, 0x5au);

                cpu->carry = (word & 0x8000u) != 0;
                const uint16_t shifted = (uint16_t)(word << 1);
                const uint32_t address = OpDp(cpu, 0x5au);
                /* Original 16-bit ASL writes the high byte first. */
                Write8(memory, OpNextByte(address), (uint8_t)(shifted >> 8));
                Write8(memory, address, (uint8_t)shifted);
                SetNz16(cpu, (uint16_t)(word << 1));
                RolA16(cpu);
            }
        }
        cpu->x = PullIndexValue(memory, cpu);
    }
    StreamBeginCopy(memory, cpu);
}

static void StreamReference(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t bank) {
    StreamPrepareReference(memory, cpu);
    do {
        StreamMoveByte(memory, cpu, bank);
    } while (cpu->accumulator != 0xffffu);
    StreamFinishCopy(memory, cpu);
}

static void StreamControl(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StreamByte(memory, cpu);
    StreamNext(memory, cpu);
    StoreADirect8(memory, cpu, FLAGS);
    LoadA8(cpu, 8u);
    StoreADirect8(memory, cpu, FLAGS_LEFT);
}

static void StreamLiteral(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StoreAAbsolute8(memory, cpu, 0u, cpu->x);
    IncrementX16(cpu);
}

static bool StreamAtEnd(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Compare16(cpu, cpu->x, Read16Direct(memory, cpu, DEST_END));
    return cpu->zero != 0;
}

static void StreamDecode(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t bank) {
    for (;;) {
        StreamControl(memory, cpu);
        for (;;) {
            StreamByte(memory, cpu);
            if (!cpu->negative) {
                StreamLiteral(memory, cpu);
                if (StreamAtEnd(memory, cpu))
                    return;
                StreamNext(memory, cpu);
                continue;
            }
            AslDirect8(memory, cpu, FLAGS);
            if (cpu->carry)
                StreamReference(memory, cpu, bank);
            else
                StreamLiteral(memory, cpu);
            if (StreamAtEnd(memory, cpu))
                return;
            StreamNext(memory, cpu);
            DecrementDirect8(memory, cpu, FLAGS_LEFT);
            if (cpu->zero)
                break;
        }
    }
}

static uint8_t DecompressPhaseBank(const Lufia2CpuState *cpu,
    uint32_t entry, uint32_t first, uint8_t accumulator_width) {
    if (cpu->program_bank != 0x80u || cpu->direct_page || cpu->decimal ||
        cpu->index_is_8_bit || cpu->accumulator_is_8_bit != accumulator_width)
        return 0u;
    if (cpu->data_bank == 0x7eu && entry == first)
        return 0x7eu;
    if (cpu->data_bank == 0x7fu && entry == first + 0xa4u)
        return 0x7fu;
    return 0u;
}

static uint32_t DecompressPhasePc(uint8_t bank, uint32_t first) {
    return first + (bank == 0x7fu ? 0xa4u : 0u);
}

static uint32_t StreamContinueFlagged(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t bank) {
    if (StreamAtEnd(memory, cpu))
        return DecompressPhasePc(bank, 0x808f21u);
    StreamNext(memory, cpu);
    DecrementDirect8(memory, cpu, FLAGS_LEFT);
    return DecompressPhasePc(bank, cpu->zero ? 0x808eefu : 0x808efau);
}

Lufia2ExecutionResult Lufia2DecompressFlagStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    const uint8_t bank = DecompressPhaseBank(cpu, entry, 0x808f40u, 1u);
    if (!bank)
        return ExecutionHandoff(cpu, entry);
    AslDirect8(memory, cpu, FLAGS);
    if (cpu->carry)
        return ExecutionHandoff(cpu, DecompressPhasePc(bank, 0x808f44u));
    StreamLiteral(memory, cpu);
    return ExecutionHandoff(cpu, StreamContinueFlagged(memory, cpu, bank));
}

Lufia2ExecutionResult Lufia2DecompressAdvanceStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    const uint8_t bank = DecompressPhaseBank(cpu, entry, 0x808f14u, 1u);
    if (!bank)
        return ExecutionHandoff(cpu, entry);
    return ExecutionHandoff(cpu, StreamContinueFlagged(memory, cpu, bank));
}

Lufia2ExecutionResult Lufia2DecompressControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    const uint8_t bank = DecompressPhaseBank(cpu, entry, 0x808eefu, 1u);
    if (!bank)
        return ExecutionHandoff(cpu, entry);
    StreamControl(memory, cpu);
    return ExecutionHandoff(cpu, DecompressPhasePc(bank, 0x808efau));
}

Lufia2ExecutionResult Lufia2DecompressSymbolStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    const uint8_t bank = DecompressPhaseBank(cpu, entry, 0x808efau, 1u);
    if (!bank)
        return ExecutionHandoff(cpu, entry);
    StreamByte(memory, cpu);
    if (cpu->negative)
        return ExecutionHandoff(cpu, DecompressPhasePc(bank, 0x808f40u));
    StreamLiteral(memory, cpu);
    if (StreamAtEnd(memory, cpu))
        return ExecutionHandoff(cpu, DecompressPhasePc(bank, 0x808f21u));
    StreamNext(memory, cpu);
    return ExecutionHandoff(cpu, entry);
}

Lufia2ExecutionResult Lufia2DecompressCopyByteStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    const uint8_t bank = DecompressPhaseBank(cpu, entry, 0x808f66u, 0u);
    if (!bank)
        return ExecutionHandoff(cpu, entry);
    StreamMoveByte(memory, cpu, bank);
    if (cpu->accumulator != 0xffffu)
        return ExecutionHandoff(cpu, entry);
    StreamFinishCopy(memory, cpu);
    return ExecutionHandoff(cpu, DecompressPhasePc(bank, 0x808f14u));
}

/* $80:8E9D: resource $54 ($A7:8000) to $7E/$7F:[$60]. */
Lufia2ExecutionResult Lufia2DecompressResource(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t bank;

    Push8(memory, cpu, PackStatus(cpu));                       /* 8E9D */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushDataBank(memory, cpu);
    LoadADirect16(memory, cpu, DP_SCRATCH_A);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, DP_SCRATCH_A));
    TransferAToX(cpu);                                         /* entry * 3 */
    Write16Direct(memory, cpu, STREAM, 0x0000u);
    LoadA16(cpu, Read16Long(memory,
        LongIndexedAddress(LUFIA2_RESOURCE_TABLE_ADDRESS, cpu->x)));
    Or16(cpu, 0x8000u);
    TransferAToY(cpu);
    LoadA16(cpu, Read16Long(memory,
        LongIndexedAddress(LUFIA2_RESOURCE_TABLE_ADDRESS + 1u, cpu->x)));
    AslA16(cpu);
    SetAccumulatorWidth(cpu, 1);
    ExchangeAccumulatorBytes(cpu);
    Adc8(cpu, 0xa7u);
    StoreADirect8(memory, cpu, 0x5fu);                         /* stream bank */
    StreamByte(memory, cpu);
    StoreADirect8(memory, cpu, LENGTH);
    StreamNext(memory, cpu);
    StreamByte(memory, cpu);
    StoreADirect8(memory, cpu, (uint8_t)(LENGTH + 1u));
    StreamNext(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* 8ED5 */
    LoadADirect16(memory, cpu, 0x60u);
    TransferAToX(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, LENGTH));
    StoreADirect16(memory, cpu, DEST_END);
    SetAccumulatorWidth(cpu, 1);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_D), 0x00u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x62u));
    Or8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LsrA8(cpu);
    bank = cpu->carry ? 0x7fu : 0x7eu;
    StreamDecode(memory, cpu, bank);
    PullDataBank(memory, cpu);                                 /* 8F21 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(bank == 0x7fu ? 0x808fc7u : 0x808f23u);
}
