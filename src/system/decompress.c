/* Resource decompressor ($80:8E9D). */

#include "core/cpu_internal.h"
#include "lufia2/system.h"

enum {
    STREAM = 0x5du,                     /* [$5D],Y: compressed bytes */
    LENGTH = 0x58u,
    DEST_END = 0x63u,
    FLAGS = 0x65u,                      /* control byte, bit 7 first */
    FLAGS_LEFT = 0x66u,
    COPY_COUNT = 0x56u,                 /* bytes - 1 for MVN */
};

/* INY; at the end of a bank continue at $8000 of the next one. */
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

/* $80:8F5C: A = source offset (negative), MVN $56 + 1 bytes from
   X + offset to X inside the destination bank. */
static void StreamCopy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t bank) {
    unsigned count;

    StoreXDirect16(memory, cpu, 0x5au);                        /* 8F5C */
    PushY(memory, cpu);
    LoadY16(cpu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x5au));
    TransferAToX(cpu);
    LoadADirect16(memory, cpu, COPY_COUNT);
    count = (unsigned)cpu->accumulator + 1u;
    while (count--) {                                          /* MVN */
        Write8(memory, ((uint32_t)bank << 16) | cpu->y,
            Read8(memory, ((uint32_t)bank << 16) | cpu->x));
        cpu->x = (uint16_t)(cpu->x + 1u);
        cpu->y = (uint16_t)(cpu->y + 1u);
    }
    cpu->accumulator = 0xffffu;
    cpu->data_bank = bank;
    LoadX16(cpu, cpu->y);                                      /* TYX */
    cpu->y = PullIndexValue(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $80:8F40: a flagged byte >= $80 starts a back reference: short
   (12-bit offset, (n & 15) + 2 bytes) or, when n & 15 is 0, long
   (14-bit offset, (m & 63) + 3 bytes). */
static void StreamReference(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t bank) {
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
                Write16Direct(memory, cpu, 0x5au, (uint16_t)(word << 1));
                SetNz16(cpu, (uint16_t)(word << 1));
                RolA16(cpu);
            }
        }
        cpu->x = PullIndexValue(memory, cpu);
    }
    StreamCopy(memory, cpu, bank);
}

/* $80:8EEF / $80:8F93: the stream loop for destination bank $7E or
   $7F. Bytes below $80 are literals; bytes from $80 take the next bit
   of the control byte: 0 literal, 1 back reference. */
static void StreamDecode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t bank) {
    for (;;) {
        StreamByte(memory, cpu);                               /* control */
        StreamNext(memory, cpu);
        StoreADirect8(memory, cpu, FLAGS);
        LoadA8(cpu, 0x08u);
        StoreADirect8(memory, cpu, FLAGS_LEFT);
        for (;;) {
            StreamByte(memory, cpu);                           /* 8EFA */
            if (!cpu->negative) {
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
                IncrementX16(cpu);
                Compare16(cpu, cpu->x, Read16Direct(memory, cpu, DEST_END));
                if (cpu->zero)
                    return;
                StreamNext(memory, cpu);
                continue;
            }
            {
                const uint8_t flags = DirectByte(memory, cpu, FLAGS);

                cpu->carry = (flags & 0x80u) != 0;             /* ASL $65 */
                Write8(memory, DirectAddress(cpu, FLAGS), (uint8_t)(flags << 1));
                SetNz8(cpu, (uint8_t)(flags << 1));
            }
            if (!cpu->carry) {
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x); /* 8F10 */
                IncrementX16(cpu);
            } else {
                StreamReference(memory, cpu, bank);
            }
            Compare16(cpu, cpu->x, Read16Direct(memory, cpu, DEST_END)); /* 8F14 */
            if (cpu->zero)
                return;
            StreamNext(memory, cpu);
            DecrementDirect8(memory, cpu, FLAGS_LEFT);         /* 8F1B */
            if (cpu->zero)
                break;
        }
    }
}

/* $80:8E9D: decompress resource $54 (table $A7:8000, 3 bytes each:
   15-bit address, bank from bit 15 on) to $7E:[$60] or, with $62 bit 0,
   $7F:[$60]; the stream starts with its unpacked length. */
Lufia2ExecutionResult Lufia2DecompressResource(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t bank;

    Push8(memory, cpu, PackStatus(cpu));                       /* 8E9D */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushDataBank(memory, cpu);
    LoadADirect16(memory, cpu, 0x54u);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
    TransferAToX(cpu);                                         /* entry * 3 */
    Write16Direct(memory, cpu, STREAM, 0x0000u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xa78000u, cpu->x)));
    Or16(cpu, 0x8000u);
    TransferAToY(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xa78001u, cpu->x)));
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
    Write8(memory, DirectAddress(cpu, 0x57u), 0x00u);
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
