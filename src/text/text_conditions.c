/* Conditional text expressions ($80:A074, opcode $14). */

#include "core/cpu_internal.h"
#include "lufia2/item.h"
#include "field/event_script_internal.h"
#include "text/text_internal.h"

/* $80:A1CC: argument -> record offset. Absolute accesses deliberately use
 * the current DB: $4202/$4203/$4216 are hardware only in its MMIO banks.
 * The ROM has enough instructions between multiply trigger and result read. */
Lufia2ExecutionResult Lufia2TextConditionRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xa1ceu);
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    LoadA8(cpu, 0xbeu);
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x0badu);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x80a1e3u);
}

/* $80:A1E4: word item ID -> low-byte possession count in DP:$54-$55. */
Lufia2ExecutionResult Lufia2TextConditionItem(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextWord(memory, cpu, 0xa1e6u);
    SetAccumulatorWidth(cpu, 0);
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xa1edu);
    Lufia2ItemPossessionCount(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    SetNz16(cpu, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, 0x54u);
    Write8(memory, DirectAddress(cpu, 0x55u), 0);
    return ExecutionReturned(0x80a1f5u);
}

static void TextConditionRecordCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Lufia2TextConditionRecord(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static void TextConditionItemCall(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xa128u);
    Lufia2TextConditionItem(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $80:A20C: eight selector values, with 5/6/7 sharing unsigned less-than.
 * Greater-than is the ROM's DEC/CMP, including 0000 -> FFFF wrap. */
static void TextConditionCompare(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned comparison;
    int passes;
    StoreZeroAbsolute8(memory, cpu, 0x1267u, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x22u));
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x0007u);
    comparison = cpu->accumulator;
    if (comparison != 0u) Compare16(cpu, cpu->accumulator, 1u);
    if (comparison > 1u) Compare16(cpu, cpu->accumulator, 2u);
    if (comparison > 2u) Compare16(cpu, cpu->accumulator, 3u);
    if (comparison > 3u) Compare16(cpu, cpu->accumulator, 4u);
    if (comparison == 3u) {
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
    } else {
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        if (comparison == 4u) LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x56u));
    }
    passes = comparison == 0u ? cpu->zero : comparison == 1u ? !cpu->zero :
        comparison <= 4u ? cpu->carry : !cpu->carry;
    SetAccumulatorWidth(cpu, 1);
    if (passes) {
        LoadA8(cpu, 0xffu);
        StoreAAbsolute8(memory, cpu, 0x1267u, 0);
    }
    SetAccumulatorWidth(cpu, 1); /* A264 reached with both M states. */
}

/* $80:A0A4: flag predicate followed by assign / AND / OR. */
static void TextConditionFlag(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    int inverted;
    LoadA8(cpu, DirectByte(memory, cpu, 0x22u));
    BitImmediate8(cpu, 1u);
    inverted = !cpu->zero;
    LoadA8(cpu, 0xffu);
    StoreAAbsolute8(memory, cpu, 0x1268u, 0);
    Lufia2TextNextByte(memory, cpu, inverted ? 0xa0c3u : 0xa0b1u);
    Lufia2TextTestFlag(memory, cpu, inverted ? 0xa0c6u : 0xa0b4u);
    if (inverted ? !cpu->zero : cpu->zero)
        StoreZeroAbsolute8(memory, cpu, 0x1268u, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x22u));
    BitImmediate8(cpu, 0x80u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x1267u, 0);
        And8(cpu, AbsoluteByte(memory, cpu, 0x1268u, 0));
    } else {
        BitImmediate8(cpu, 0x40u);
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x1267u, 0);
            Or8(cpu, AbsoluteByte(memory, cpu, 0x1268u, 0));
        } else LoadAAbsolute8(memory, cpu, 0x1268u, 0);
    }
    StoreAAbsolute8(memory, cpu, 0x1267u, 0);
}

/* $80:A0F9: item/record/actor predicates. Returns 1 for the predicates
 * that already produced $1267, 0 when an operand comparison follows. */
static int TextConditionSpecial(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned kind;
    int byte_operand = 0;
    LoadA8(cpu, DirectByte(memory, cpu, 0x22u));
    And8(cpu, 0xf8u);
    kind = A8(cpu);
    /* Preserve the original ordered selector CMPs. */
    for (unsigned k = 0xc0u; k <= kind; k += 8u) {
        Compare8(cpu, A8(cpu), (uint8_t)k);
        if (cpu->zero) break;
    }
    switch (kind) {
    case 0xc0u:
        TextConditionItemCall(memory, cpu);
        break;
    case 0xc8u:
    case 0xd0u:
        TextConditionRecordCall(memory, cpu, kind == 0xc8u ? 0xa165u : 0xa173u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
            kind == 0xc8u ? 0x0011u : 0x0013u, cpu->x));
        StoreADirect16(memory, cpu, 0x54u);
        SetAccumulatorWidth(cpu, 1);
        break;
    case 0xd8u:
        Lufia2TextNextByte(memory, cpu, 0xa181u); /* discarded byte */
        LoadA8(cpu, 0xffu);
        StoreADirect8(memory, cpu, 0x54u);
        StoreADirect8(memory, cpu, 0x57u); /* $55 deliberately remains stale */
        break;
    case 0xe0u:
        StoreZeroAbsolute8(memory, cpu, 0x1267u, 0);
        TextConditionRecordCall(memory, cpu, 0xa18fu);
        Lufia2TextNextByte(memory, cpu, 0xa192u);
        StoreADirect8(memory, cpu, 0x54u);
        PushY(memory, cpu);
        LoadY16(cpu, 0x0023u);
        do {
            LoadAAbsolute8(memory, cpu, 0x0096u, cpu->x);
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x54u));
            if (cpu->zero) break;
            IncrementX16(cpu);
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->negative);
        if (cpu->zero) {
            LoadA8(cpu, 0xffu);
            StoreAAbsolute8(memory, cpu, 0x1267u, 0);
        }
        cpu->y = PullIndexValue(memory, cpu);
        SetNz16(cpu, cpu->y);
        return 1;
    case 0xe8u:
        TextConditionRecordCall(memory, cpu, 0xa1b1u);
        LoadAAbsolute8(memory, cpu, 0x000eu, cpu->x);
        StoreADirect8(memory, cpu, 0x54u);
        Write8(memory, DirectAddress(cpu, 0x55u), 0);
        byte_operand = 1;
        break;
    case 0xf0u:
        Lufia2TextNextByte(memory, cpu, 0xa148u);
        LoadA8(cpu, Read8(memory, 0x0005aeu));
        Compare8(cpu, A8(cpu), 2u);
        if (cpu->carry) LoadA8(cpu, 0xffu);
        else {
            LoadA8(cpu, Read8(memory, 0x0005b2u));
            for (unsigned i = 0; i < 4u; ++i) LsrA8(cpu);
        }
        StoreADirect8(memory, cpu, 0x54u);
        Write8(memory, DirectAddress(cpu, 0x55u), 0);
        byte_operand = 1;
        break;
    case 0xf8u:
        Lufia2TextNextByte(memory, cpu, 0xa12eu);
        Lufia2EventFindActorId(memory, cpu, 0xa132u);
        Lufia2TextNextByte(memory, cpu, 0xa135u);
        PushY(memory, cpu);
        LoadX16(cpu, Read16Direct(memory, cpu, 0xa7u));
        LoadAAbsolute8(memory, cpu, 0x0736u, cpu->x);
        BitImmediate8(cpu, 0x20u);
        if (!cpu->zero) {
            LoadA8(cpu, 0xffu);
            StoreAAbsolute8(memory, cpu, 0x1267u, 0);
        } /* False preserves the preceding result. */
        cpu->y = PullIndexValue(memory, cpu);
        SetNz16(cpu, cpu->y);
        return 1;
    }
    if (byte_operand) {
        Lufia2TextNextByte(memory, cpu, 0xa1bbu);
        StoreADirect8(memory, cpu, 0x56u);
        Write8(memory, DirectAddress(cpu, 0x57u), 0);
    } else {
        Lufia2TextNextWord(memory, cpu, 0xa1c4u);
        StoreADirect8(memory, cpu, 0x56u);
        ExchangeAccumulatorBytes(cpu);
        StoreADirect8(memory, cpu, 0x57u);
    }
    return 0;
}

uint32_t Lufia2TextEvaluateCondition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreZeroAbsolute8(memory, cpu, 0x1267u, 0); /* A074 */
    for (;;) {
        unsigned kind;
        Lufia2TextNextByte(memory, cpu, 0xa079u);
        StoreADirect8(memory, cpu, 0x22u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x22u));
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero) return 0x809d00u;
        And8(cpu, 0xc0u);
        Compare8(cpu, A8(cpu), 0xc0u);
        if (cpu->zero) {
            if (TextConditionSpecial(memory, cpu)) continue;
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, 0x22u));
            And8(cpu, 0x30u);
            kind = A8(cpu);
            Compare8(cpu, A8(cpu), 0x10u);
            if (kind != 0x10u) Compare8(cpu, A8(cpu), 0x20u);
            if (kind != 0x10u && kind != 0x20u) Compare8(cpu, A8(cpu), 0x30u);
            if (kind == 0x20u || kind == 0x30u) {
                LoadAAbsolute8(memory, cpu, 0x1267u, 0);
                if (kind == 0x20u ? !cpu->zero : cpu->zero) return 0x80a3c6u;
                Lufia2TextNextByte(memory, cpu, kind == 0x20u ? 0xa270u : 0xa27eu);
                Lufia2TextNextByte(memory, cpu, kind == 0x20u ? 0xa273u : 0xa281u);
                continue;
            }
            if (kind == 0u) {
                TextConditionFlag(memory, cpu);
                continue;
            }
            Lufia2TextNextByte(memory, cpu, 0xa1f8u);
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, 0);
            ExchangeAccumulatorBytes(cpu);
            TransferAToX(cpu);
            LoadAAbsolute8(memory, cpu, 0x079eu, cpu->x);
            StoreADirect8(memory, cpu, 0x54u);
            Write8(memory, DirectAddress(cpu, 0x55u), 0);
            Lufia2TextNextByte(memory, cpu, 0xa207u);
            StoreADirect8(memory, cpu, 0x56u);
            Write8(memory, DirectAddress(cpu, 0x57u), 0);
        }
        TextConditionCompare(memory, cpu);
    }
}
