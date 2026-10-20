/* Derived character stats ($81:F4D5). */

#include <stdbool.h>
#include <stddef.h>

#include "core/arithmetic_value.h"
#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "system/wram.h"
#include "lufia2/party.h"

/* Totals: base word plus one or two modifiers. */
enum {
    DP_RECORD = 0xc1,
    TOTAL_CAP_LIMIT = 200, /* a total of 200 or more reads as 199 */
    TOTAL_CAP_VALUE = 199,
};

typedef struct StatTotal {
    uint8_t base;
    uint8_t modifier;
    uint8_t second_modifier; /* zero: none */
    uint8_t total;
    bool capped;
} StatTotal;

static const StatTotal kStatTotals[] = {
    {0x51, 0x74, 0x00, 0x25, false}, {0x53, 0x76, 0x00, 0x27, false},
    {0x5d, 0x84, 0x92, 0x35, false}, {0x5b, 0x82, 0x90, 0x33, true},
    {0x59, 0x80, 0x8e, 0x31, false}, {0x57, 0x7e, 0x8c, 0x2f, false},
    {0x55, 0x7c, 0x8a, 0x2d, false},
};

/* Totals derived from others or overridden by a word. */
enum {
    TOTAL_2D = 0x2d,
    TOTAL_2F = 0x2f,
    TOTAL_29 = 0x29,
    TOTAL_2B = 0x2b,
    ADDEND_29 = 0x86,
    OVERRIDE_29 = 0x78,
    ADDEND_2B = 0x88,
    OVERRIDE_2B = 0x7a,
};

typedef struct StatRecord {
    const Lufia2Memory *memory;
    uint32_t address;
} StatRecord;

static uint16_t RecordWord(StatRecord record, uint16_t offset) {
    return Read16Long(record.memory, LongIndexedAddress(record.address, offset));
}

static void SetRecordWord(StatRecord record, uint16_t offset, uint16_t value) {
    Write16Long(record.memory, LongIndexedAddress(record.address, offset), value);
}

/* The first modifier's carry feeds the second. */
static Word16Result SumStat(StatRecord record, const StatTotal *stat, bool decimal) {
    const uint16_t base = RecordWord(record, stat->base);
    const uint16_t modifier = RecordWord(record, stat->modifier);
    Word16Result total = Sum16Mode(base, modifier, false, decimal);

    if (stat->second_modifier)
        total = Sum16Mode(total.value, RecordWord(record, stat->second_modifier),
            total.carry, decimal);
    return total;
}

static uint16_t ApplyOverride(StatRecord record, uint16_t override_offset,
                             uint16_t total_offset) {
    const uint16_t override = RecordWord(record, override_offset);

    if (override != 0)
        SetRecordWord(record, total_offset, override);
    return override;
}

static Word16Result UpdateDerivedTotals(StatRecord record, bool decimal) {
    Word16Result sum = {0};

    for (size_t i = 0; i < sizeof kStatTotals / sizeof kStatTotals[0]; ++i) {
        const StatTotal *stat = &kStatTotals[i];
        sum = SumStat(record, stat, decimal);
        const uint16_t total = stat->capped && sum.value >= TOTAL_CAP_LIMIT
            ? TOTAL_CAP_VALUE : sum.value;
        SetRecordWord(record, stat->total, total);
    }

    sum = Sum16Mode(sum.value, RecordWord(record, ADDEND_29), false, decimal);
    SetRecordWord(record, TOTAL_29, sum.value);
    (void)ApplyOverride(record, OVERRIDE_29, TOTAL_29);
    const uint16_t first = RecordWord(record, TOTAL_2F);
    const uint16_t second = RecordWord(record, TOTAL_2D);
    sum = Sum16Mode(first, second, false, decimal);
    sum = Sum16Mode((uint16_t)(sum.value >> 1), RecordWord(record, ADDEND_2B),
        false, decimal);
    SetRecordWord(record, TOTAL_2B, sum.value);
    sum.value = ApplyOverride(record, OVERRIDE_2B, TOTAL_2B);
    return sum;
}

static void DerivedStats(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadX16(cpu, Read16Direct(memory, cpu, DP_RECORD));
    SetAccumulatorWidth(cpu, 0);
    const StatRecord record = {memory, AbsoluteIndexedAddress(cpu, 0u, cpu->x)};
    const Word16Result result = UpdateDerivedTotals(record, cpu->decimal != 0);

    LoadA16(cpu, result.value);
    cpu->carry = result.carry;
    cpu->overflow = result.overflow;
    SetAccumulatorWidth(cpu, 1);
    EmitExecutionCheckpoint(memory, cpu, 0x81f576u);
}

/* $81:F4D5: derived stats of block X; keeps A, X, Y, P. */
Lufia2ExecutionResult Lufia2PartyDerivedStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreXDirect16(memory, cpu, 0xc1u);
    SimulateJsrFrame(memory, cpu, 0xf4e1u);
    DerivedStats(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    EmitExecutionCheckpoint(memory, cpu, 0x81f4e2u);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f4e8u);
}

enum {
    STATS = WRAM_PARTY_BASE_STAT_WORK,                    /* 7 words */
    MEMBER = WRAM_PARTY_STAT_MEMBER_INDEX,
    LEVEL = WRAM_PARTY_STAT_LEVEL,
    COUNT = 0x09f2u,
};

enum {
    BASE_STAT_COUNT = 7u,
    BASE_GROWTH_ROW = 0xb2u,
    BASE_GROWTH_TABLE = 0xb62cu,
    BASE_VALUES_TABLE = 0xb93cu,
    BASE_GROWTH_ROW_LEVELS = 8u,
    BASE_GROWTH_SCALE_SHIFT = 4u
};

static Byte8Result AddBaseStatByte(
    Lufia2Wram wram, uint16_t field, uint8_t increment, bool decimal) {
    const uint8_t previous = WramReadAt(wram, STATS, field);
    const Byte8Result sum = Sum8Mode(increment, previous, false, decimal);
    WramWriteAt(wram, STATS, field, sum.value);
    if (sum.carry) {
        const uint8_t high = WramReadAt(wram, STATS + 1u, field);
        WramWriteAt(wram, STATS + 1u, field, (uint8_t)(high + 1u));
    }
    return sum;
}

static void AccumulateLevelGrowth(Lufia2Wram wram, bool decimal) {
    for (unsigned stat = 0u; stat < BASE_STAT_COUNT; ++stat)
        WramWrite16At(wram, STATS, (uint16_t)(stat * 2u), 0u);
    const uint16_t member_offset = WramRead16(wram, SNES_RDMPYL);
    const Word16Result first_row = Sum16Mode(BASE_GROWTH_TABLE, member_offset, false, decimal);
    WramWrite16(wram, BASE_GROWTH_ROW, first_row.value);

    uint16_t level = 1u;
    while (level != WramRead16(wram, LEVEL)) {
        const bool next_row = level % BASE_GROWTH_ROW_LEVELS == 0u;
        level = (uint16_t)(level + 1u);
        if (next_row) {
            const uint16_t row = WramRead16(wram, BASE_GROWTH_ROW);
            const Word16Result next = Sum16Mode(row, BASE_GROWTH_ROW_LEVELS, false, decimal);
            WramWrite16(wram, BASE_GROWTH_ROW, next.value);
        }
        for (unsigned stat = 0u; stat < BASE_STAT_COUNT; ++stat) {
            const uint16_t row = WramRead16(wram, BASE_GROWTH_ROW);
            const uint32_t source = (((uint32_t)wram.data_bank << 16) + row + stat) & 0xffffffu;
            const uint8_t increment = Read8(wram.memory, source);
            (void)AddBaseStatByte(wram, (uint16_t)(stat * 2u), increment, decimal);
        }
    }
}

static uint16_t ScaleLevelGrowth(Lufia2Wram wram) {
    uint16_t scaled = 0u;
    for (unsigned stat = BASE_STAT_COUNT; stat > 0u; --stat) {
        const uint16_t field = (uint16_t)((stat - 1u) * 2u);
        scaled = WramRead16At(wram, STATS, field) >> BASE_GROWTH_SCALE_SHIFT;
        WramWrite16At(wram, STATS, field, scaled);
    }
    return scaled;
}

static Byte8Result AddInitialBaseStats(
    Lufia2Wram wram, uint16_t member_offset, bool decimal) {
    Byte8Result total = {0};
    for (unsigned stat = 0u; stat < BASE_STAT_COUNT; ++stat) {
        const uint8_t base = WramReadAt(wram, BASE_VALUES_TABLE, member_offset);
        total = AddBaseStatByte(wram, (uint16_t)(stat * 2u), base, decimal);
        member_offset = (uint16_t)(member_offset + 2u);
    }
    return total;
}

/* Stats of the selected member at the requested level. */
Lufia2ExecutionResult Lufia2PartyBaseStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, MEMBER, 0);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    StoreA8Absolute(memory, cpu, SNES_WRMPYB, 0x0eu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    LoadX16(cpu, 0u);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    PushY(memory, cpu);
    StoreA8Absolute(memory, cpu, SNES_WRMPYB, 0x70u);
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const bool decimal = cpu->decimal != 0u;
    SetAccumulatorWidth(cpu, 0u);
    AccumulateLevelGrowth(wram, decimal);
    LoadA16(cpu, ScaleLevelGrowth(wram));
    SetAccumulatorWidth(cpu, 1u);
    cpu->y = PullIndexValue(memory, cpu);
    const Byte8Result total = AddInitialBaseStats(wram, cpu->y, decimal);
    LoadA8(cpu, total.value);
    cpu->overflow = total.overflow;
    cpu->carry = 1u;
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x81f978u);
}

enum {
    MEMBER_RECORD_POINTER = 0xb2u,
    MEMBER_RECORD_BANK = 0xb4u,
    MEMBER_STORED_LEVEL = 0x000eu,
    MEMBER_NAME_PREFIX = 0x0094u,
    MEMBER_NAME_FIRST = 0x0096u,
    MEMBER_NAME_END = 0x00bau,
    MEMBER_TRAILER_FIRST = 0x005fu,
    MEMBER_TRAILER_SECOND = 0x0061u
};

static uint16_t MemberSourceWord(
    Lufia2Wram wram, uint16_t source, uint16_t offset) {
    const uint32_t address = (((uint32_t)wram.data_bank << 16) + source + offset) & 0xffffffu;
    return Read16Long(wram.memory, address);
}

static uint32_t MemberRecordAddress(Lufia2Wram wram, uint16_t field) {
    const uint16_t offset = WramRead16(wram, MEMBER_RECORD_POINTER);
    const uint8_t bank = WramRead(wram, MEMBER_RECORD_BANK);
    return ((((uint32_t)bank << 16) | offset) + field) & 0xffffffu;
}

static void MemberRecordWord(Lufia2Wram wram, uint16_t field, uint16_t value) {
    Write16Long(wram.memory, MemberRecordAddress(wram, field), value);
}

static void MemberRecordByte(Lufia2Wram wram, uint16_t field, uint8_t value) {
    Write8(wram.memory, MemberRecordAddress(wram, field), value);
}

static uint16_t SourceWord(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t offset) {
    return MemberSourceWord(WramViewOfCaller(memory, cpu), cpu->x, offset);
}

static void BlockStore16(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t value) {
    MemberRecordWord(WramViewOfCaller(memory, cpu), cpu->y, value);
}

static void BlockStore8(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t value) {
    MemberRecordByte(WramViewOfCaller(memory, cpu), cpu->y, value);
}

static Word16Result RestoreMemberHeader(
    Lufia2Wram wram, uint16_t source, bool decimal) {
    for (uint16_t field = 0u; field < 6u; field += 2u) {
        const uint16_t value = MemberSourceWord(wram, source, field);
        MemberRecordWord(wram, field, value);
    }
    return Sum16Mode(source, 6u, false, decimal);
}

static Word16Result RestoreMemberIdentity(
    Lufia2Wram wram, uint16_t source, bool decimal) {
    const uint16_t level = MemberSourceWord(wram, source, 0u);
    source = (uint16_t)(source + 2u);
    MemberRecordWord(wram, MEMBER_STORED_LEVEL, level);
    const uint16_t prefix = MemberSourceWord(wram, source, 0u);
    source = (uint16_t)(source + 2u);
    MemberRecordWord(wram, MEMBER_NAME_PREFIX, prefix);

    uint16_t field = MEMBER_NAME_FIRST;
    for (;;) {
        const uint8_t letter = Read8(wram.memory, ((uint32_t)wram.data_bank << 16) | source);
        source = (uint16_t)(source + 1u);
        if (letter == 0xffu)
            break;
        MemberRecordByte(wram, field, letter);
        field = (uint16_t)(field + 1u);
    }
    while (field != MEMBER_NAME_END) {
        MemberRecordByte(wram, field, 0xffu);
        field = (uint16_t)(field + 1u);
    }
    const uint16_t first = MemberSourceWord(wram, source, 0u);
    MemberRecordWord(wram, MEMBER_TRAILER_FIRST, first);
    const uint16_t second = MemberSourceWord(wram, source, 2u);
    MemberRecordWord(wram, MEMBER_TRAILER_SECOND, second);
    /* The stored trailer advances three source bytes. */
    return Sum16Mode(source, 3u, false, decimal);
}

static void RestoreMemberIdentityState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Word16Result end = RestoreMemberIdentity(
        WramViewOfCaller(memory, cpu), cpu->x, cpu->decimal != 0u);
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, MEMBER_TRAILER_SECOND);
    LoadA16(cpu, end.value);
    TransferAToX(cpu);
    cpu->carry = end.carry;
    cpu->overflow = end.overflow;
}

typedef struct MemberWordRun {
    uint16_t source;
    uint16_t field;
    uint16_t value;
} MemberWordRun;

static MemberWordRun RestoreMemberWords(
    Lufia2Wram wram, uint16_t source, uint16_t field,
    uint16_t count, bool stored) {
    MemberWordRun run = {source, field, wram.direct_page};
    WramWrite16(wram, COUNT, count);
    do {
        if (stored)
            run.value = MemberSourceWord(wram, run.source, 0u);
        MemberRecordWord(wram, run.field, run.value);
        if (stored)
            run.source = (uint16_t)(run.source + 2u);
        run.field = (uint16_t)(run.field + 2u);
    } while (WramStep16(wram, COUNT, -1) != 0u);
    return run;
}

static void RestoreMemberWordState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t field, uint16_t count, bool stored) {
    const MemberWordRun run = RestoreMemberWords(
        WramViewOfCaller(memory, cpu), cpu->x, field, count, stored);
    cpu->accumulator = run.value;
    cpu->x = run.source;
    cpu->y = run.field;
    SetNz16(cpu, 0u);
}

/* Shared tail: stats, experience, derived stats, full HP and MP. */
static void MemberFinish(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    const uint16_t returns[3]) {
    static const uint16_t kStats[7] = {
        0x0051u, 0x0053u, 0x0055u, 0x0057u, 0x0059u, 0x005bu, 0x005du};
    unsigned i;

    IncrementX16(cpu);
    PushIndex(memory, cpu);
    LoadY16(cpu, Read16Direct(memory, cpu, 0xb2u));
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x000eu, cpu->y) & 0x00ffu));
    StoreAAbsolute16(memory, cpu, LEVEL, 0);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x97u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x81u, returns[0]);
    (void)Lufia2PartyBaseStats(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    PullDataBank(memory, cpu);
    StoreYDirect16(memory, cpu, 0xb2u);
    LoadX16(cpu, cpu->y);
    SimulateJslFrame(memory, cpu, 0x81u, returns[1]);
    (void)Lufia2PartyExperienceForLevel(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadY16(cpu, Read16Direct(memory, cpu, 0xb2u));
    LoadAAbsolute8(memory, cpu, 0x0a2cu, 0);
    StoreAAbsolute8(memory, cpu, 0x0064u, cpu->y);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a2au, 0));
    StoreAAbsolute16(memory, cpu, 0x0062u, cpu->y);
    for (i = 0; i < 7u; ++i) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (uint16_t)(STATS + 2u * i), 0));
        StoreAAbsolute16(memory, cpu, kStats[i], cpu->y);
    }
    OpStepMem(memory, cpu, OpAbs(cpu, MEMBER), 1);
    LoadX16(cpu, Read16Direct(memory, cpu, 0xb2u));
    SimulateJslFrame(memory, cpu, 0x81u, returns[2]);
    (void)Lufia2PartyDerivedStats(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    LoadY16(cpu, 0x0025u);
    LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xb2u));
    LoadY16(cpu, 0x0011u);
    BlockStore16(memory, cpu, cpu->accumulator);               /* HP = max */
    LoadY16(cpu, 0x0027u);
    LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xb2u));
    LoadY16(cpu, 0x0013u);
    BlockStore16(memory, cpu, cpu->accumulator);               /* MP = max */
}

static void BlockPointer(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreYDirect16(memory, cpu, 0xb2u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7eu);
    StoreADirect8(memory, cpu, 0xb4u);
    SetAccumulatorWidth(cpu, 0);
}

/* $81:ED8E: member block $7E:Y from its stored form at X. */
Lufia2ExecutionResult Lufia2PartyUnpackMember(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kReturns[3] = {0xee32u, 0xee3au, 0xee7du};

    BlockPointer(memory, cpu);
    const Word16Result header = RestoreMemberHeader(
        WramViewOfCaller(memory, cpu), cpu->x, cpu->decimal != 0u);
    LoadA16(cpu, header.value);
    TransferAToX(cpu);
    cpu->carry = header.carry;
    cpu->overflow = header.overflow;
    RestoreMemberIdentityState(memory, cpu);
    RestoreMemberWordState(memory, cpu, 0x0066u, 7u, true);
    LoadY16(cpu, 0x00bcu);
    LoadA16(cpu, SourceWord(memory, cpu, 0));
    BlockStore16(memory, cpu, cpu->accumulator);
    MemberFinish(memory, cpu, kReturns);
    return ExecutionReturned(0x81ee93u);
}

/* $81:EE94: as $81:ED8E without equipment, with 9 bonus words at $74. */
Lufia2ExecutionResult Lufia2PartyUnpackMemberBare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kReturns[3] = {0xef31u, 0xef39u, 0xef7cu};

    BlockPointer(memory, cpu);
    RestoreMemberIdentityState(memory, cpu);
    RestoreMemberWordState(memory, cpu, 0x0066u, 7u, false);
    RestoreMemberWordState(memory, cpu, 0x0074u, 9u, true);
    LoadY16(cpu, 0x00bcu);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);
    BlockStore8(memory, cpu, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    MemberFinish(memory, cpu, kReturns);
    return ExecutionReturned(0x81ef92u);
}

/* $81:F4ED: totals of the record at [$C1]. */
Lufia2ExecutionResult Lufia2PartyStatTotals(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    DerivedStats(memory, cpu);
    return ExecutionReturned(0x81f576u);
}

/* $81:F4E9: far entry of Lufia2PartyStatTotals. */
Lufia2ExecutionResult Lufia2PartyStatTotalsFar(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xf4ebu);
    (void)Lufia2PartyStatTotals(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x81f4ecu);
}

enum {
    STAT_COPY_RECORD = 0x3800,
    STAT_COPY_RECORD_BYTES = 0xbe,
    STAT_COPY_MODIFIERS = 0x3886,
    STAT_COPY_TOTALS = 0x3829,
    MEMBER_TOTALS_OFFSET = 0x37,
    MEMBER_TOTALS_BYTES = 14
};

/* Copy forwards: a member can overlap the scratch record. */
static void CopyMemberBytes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t member_offset, uint16_t scratch, uint16_t length) {
    uint16_t member = cpu->x;
    uint8_t last = 0;

    for (uint16_t byte = 0; byte < length; ++byte) {
        last = AbsoluteByte(memory, cpu, member_offset, member);
        Write8(memory, AbsoluteIndexedAddress(cpu, scratch, byte), last);
        member = (uint16_t)(member + 1u);
    }
    LoadA8(cpu, last);
    cpu->x = member;
    cpu->y = length;
    Compare16(cpu, length, length);
}

static void StoreCopiedTotals(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint16_t member = cpu->x;
    uint8_t last = 0;

    for (uint16_t byte = 0; byte < MEMBER_TOTALS_BYTES; ++byte) {
        last = AbsoluteByte(memory, cpu, STAT_COPY_TOTALS, byte);
        Write8(memory,
            AbsoluteIndexedAddress(cpu, MEMBER_TOTALS_OFFSET, member), last);
        member = (uint16_t)(member + 1u);
    }
    LoadA8(cpu, last);
    cpu->x = member;
    cpu->y = MEMBER_TOTALS_BYTES;
    Compare16(cpu, MEMBER_TOTALS_BYTES, MEMBER_TOTALS_BYTES);
}

/* Calculate on a scratch record, then copy only the derived totals back. */
Lufia2ExecutionResult Lufia2PartyStatTotalsOfCopy(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    /* Other stacks or DP scratch can overwrite an inlined return frame. */
    if (cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x81f481u);

    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreXDirect16(memory, cpu, DP_RECORD);
    OpSetDataBank(memory, cpu, 0x7eu);

    CopyMemberBytes(memory, cpu, 0, STAT_COPY_RECORD, STAT_COPY_RECORD_BYTES);
    /* The original reloads absolute $C1, independently of direct page. */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, DP_RECORD, 0));
    PushIndex(memory, cpu);
    CopyMemberBytes(memory, cpu, MEMBER_TOTALS_OFFSET,
        STAT_COPY_MODIFIERS, MEMBER_TOTALS_BYTES);

    LoadX16(cpu, STAT_COPY_RECORD);
    StoreXDirect16(memory, cpu, DP_RECORD);
    SimulateJsrFrame(memory, cpu, 0xf4bbu);
    (void)Lufia2PartyStatTotals(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    SetNz16(cpu, cpu->x);
    StoreCopiedTotals(memory, cpu);

    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81f4d4u);
}
