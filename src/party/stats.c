/* Derived character stats ($81:F4D5). */

#include "core/cpu_internal.h"
#include "lufia2/party.h"

static uint16_t Field(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t offset) {
    return Read16AbsoluteIndexed(memory, cpu, offset, cpu->x);
}

static void StoreField(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t offset) {
    const uint32_t at = AbsoluteIndexedAddress(cpu, offset, cpu->x);

    Write8(memory, at, (uint8_t)cpu->accumulator);
    Write8(memory, (at + 1u) & 0x00ffffffu, (uint8_t)(cpu->accumulator >> 8));
}

/* A = base + bonus (+ second bonus). */
static void Sum(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t base, uint16_t bonus, uint16_t bonus2) {
    LoadA16(cpu, Field(memory, cpu, base));
    cpu->carry = 0;
    Add16Value(cpu, Field(memory, cpu, bonus));
    if (bonus2)
        Add16Value(cpu, Field(memory, cpu, bonus2));
}

/* $81:F4ED: stats of the block at [$C1]. */
static void DerivedStats(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadX16(cpu, Read16Direct(memory, cpu, 0xc1u));
    SetAccumulatorWidth(cpu, 0);
    Sum(memory, cpu, 0x0051u, 0x0074u, 0);
    StoreField(memory, cpu, 0x0025u);
    Sum(memory, cpu, 0x0053u, 0x0076u, 0);
    StoreField(memory, cpu, 0x0027u);
    Sum(memory, cpu, 0x005du, 0x0084u, 0x0092u);
    StoreField(memory, cpu, 0x0035u);
    Sum(memory, cpu, 0x005bu, 0x0082u, 0x0090u);
    Compare16(cpu, cpu->accumulator, 0x00c8u);
    if (cpu->carry)
        LoadA16(cpu, 0x00c7u);                                 /* cap 199 */
    StoreField(memory, cpu, 0x0033u);
    Sum(memory, cpu, 0x0059u, 0x0080u, 0x008eu);
    StoreField(memory, cpu, 0x0031u);
    Sum(memory, cpu, 0x0057u, 0x007eu, 0x008cu);
    StoreField(memory, cpu, 0x002fu);
    Sum(memory, cpu, 0x0055u, 0x007cu, 0x008au);
    StoreField(memory, cpu, 0x002du);
    cpu->carry = 0;
    Add16Value(cpu, Field(memory, cpu, 0x0086u));
    StoreField(memory, cpu, 0x0029u);
    LoadA16(cpu, Field(memory, cpu, 0x0078u));
    if (!cpu->zero)
        StoreField(memory, cpu, 0x0029u);
    LoadA16(cpu, Field(memory, cpu, 0x002fu));                 /* F55D */
    cpu->carry = 0;
    Add16Value(cpu, Field(memory, cpu, 0x002du));
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Field(memory, cpu, 0x0088u));
    StoreField(memory, cpu, 0x002bu);
    LoadA16(cpu, Field(memory, cpu, 0x007au));
    if (!cpu->zero)
        StoreField(memory, cpu, 0x002bu);
    SetAccumulatorWidth(cpu, 1);
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
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f4e8u);
}

enum {
    STATS = 0x0a1cu,                    /* 7 words */
    MEMBER = 0x09fau,
    LEVEL = 0x09feu,
    COUNT = 0x09f2u,
};

/* $81:F87F: stats of member $09FA at level $09FE: base ($97:B93C)
   plus growth rows ($97:B62C + $70 per member, a row per 8 levels) / 16. */
Lufia2ExecutionResult Lufia2PartyBaseStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    LoadAAbsolute8(memory, cpu, MEMBER, 0);
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    StoreA8Absolute(memory, cpu, 0x4203u, 0x0eu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    LoadX16(cpu, 0x0000u);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));   /* member * 14 */
    PushY(memory, cpu);
    StoreA8Absolute(memory, cpu, 0x4203u, 0x70u);
    LoadX16(cpu, 0x0001u);
    SetAccumulatorWidth(cpu, 0);
    for (i = 0; i < 7u; ++i)
        Write16Absolute(memory, cpu, (uint16_t)(STATS + 2u * i), 0);
    LoadA16(cpu, 0xb62cu);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreADirect16(memory, cpu, 0xb2u);
    SetAccumulatorWidth(cpu, 1);
    for (;;) {
        Compare16(cpu, cpu->x, Read16AbsoluteIndexed(memory, cpu, LEVEL, 0));   /* F8BD */
        if (cpu->zero)
            break;
        TransferXToA(cpu);
        IncrementX16(cpu);
        And8(cpu, 0x07u);
        if (cpu->zero) {
            SetAccumulatorWidth(cpu, 0);                       /* next row */
            LoadA16(cpu, Read16Direct(memory, cpu, 0xb2u));
            cpu->carry = 0;
            Add16Value(cpu, 0x0008u);
            StoreADirect16(memory, cpu, 0xb2u);
            SetAccumulatorWidth(cpu, 1);
        }
        LoadY16(cpu, 0x0000u);
        for (i = 0; i < 7u; ++i) {
            const uint16_t sum = (uint16_t)(STATS + 2u * i);

            if (i)
                IncrementY16(cpu);
            LoadA8(cpu, Read8(memory, (((uint32_t)cpu->data_bank << 16) +
                Read16Direct(memory, cpu, 0xb2u) + cpu->y) & 0x00ffffffu));
            cpu->carry = 0;
            Adc8(cpu, AbsoluteByte(memory, cpu, sum, 0));
            StoreAAbsolute8(memory, cpu, sum, 0);
            if (cpu->carry) {
                const uint32_t high = AbsoluteIndexedAddress(cpu, (uint16_t)(sum + 1u), 0);

                Write8(memory, high, (uint8_t)(Read8(memory, high) + 1u));
            }
        }
    }
    SetAccumulatorWidth(cpu, 0);                               /* F945: / 16 */
    LoadX16(cpu, 0x000cu);
    do {
        LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, STATS, cpu->x) >> 4));
        StoreAAbsolute16(memory, cpu, STATS, cpu->x);
        cpu->x = (uint16_t)(cpu->x - 2u);
        SetNz16(cpu, cpu->x);
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    cpu->y = PullIndexValue(memory, cpu);
    LoadX16(cpu, 0x0000u);
    do {                                                       /* base */
        LoadAAbsolute8(memory, cpu, 0xb93cu, cpu->y);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, STATS, cpu->x));
        StoreAAbsolute8(memory, cpu, STATS, cpu->x);
        if (cpu->carry) {
            const uint32_t high = AbsoluteIndexedAddress(cpu, (uint16_t)(STATS + 1u), cpu->x);

            Write8(memory, high, (uint8_t)(Read8(memory, high) + 1u));
        }
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x000eu);
    } while (!cpu->zero);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x81f978u);
}

/* DEC abs, 16-bit. */
static void Decrement16Absolute(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t address) {
    const uint32_t at = AbsoluteIndexedAddress(cpu, address, 0);
    const uint16_t value = (uint16_t)(Read16Long(memory, at) - 1u);

    Write16Long(memory, at, value);
    SetNz16(cpu, value);
}

static uint16_t SourceWord(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t offset) {
    return Read16AbsoluteIndexed(memory, cpu, offset, cpu->x);
}

static void BlockStore16(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t value) {
    Write16Long(memory, DirectLongIndirectY(memory, cpu, 0xb2u), value);
}

static void BlockStore8(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t value) {
    Write8(memory, DirectLongIndirectY(memory, cpu, 0xb2u), value);
}

static void Advance(Lufia2CpuState *cpu, uint16_t bytes) {
    cpu->x = (uint16_t)(cpu->x + bytes);
    SetNz16(cpu, cpu->x);
}

/* Name at $96 up to the $FF, padded with $FF to $BA; then $5F-$62. */
static void MemberName(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadY16(cpu, 0x0094u);
    LoadA16(cpu, SourceWord(memory, cpu, 0));
    Advance(cpu, 2u);
    BlockStore16(memory, cpu, cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    SetAccumulatorWidth(cpu, 1);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        cpu->x = (uint16_t)(cpu->x + 1u);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero)
            break;
        BlockStore8(memory, cpu, A8(cpu));
        IncrementY16(cpu);
    }
    for (;;) {
        Compare16(cpu, cpu->y, 0x00bau);
        if (cpu->zero)
            break;
        BlockStore8(memory, cpu, A8(cpu));
        IncrementY16(cpu);
    }
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, 0x005fu);
    LoadA16(cpu, SourceWord(memory, cpu, 0));
    BlockStore16(memory, cpu, cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadA16(cpu, SourceWord(memory, cpu, 2u));
    BlockStore16(memory, cpu, cpu->accumulator);
    TransferXToA(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0003u);
    TransferAToX(cpu);
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
    {
        const uint32_t member = AbsoluteIndexedAddress(cpu, MEMBER, 0);
        const uint16_t next = (uint16_t)(Read16Long(memory, member) + 1u);

        Write16Long(memory, member, next);
    }
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
    unsigned i;

    BlockPointer(memory, cpu);
    LoadY16(cpu, 0x0000u);
    for (i = 0; i < 3u; ++i) {
        LoadA16(cpu, SourceWord(memory, cpu, (uint16_t)(2u * i)));
        BlockStore16(memory, cpu, cpu->accumulator);
        IncrementY16(cpu);
        IncrementY16(cpu);
    }
    TransferXToA(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0006u);
    TransferAToX(cpu);
    LoadY16(cpu, 0x000eu);
    LoadA16(cpu, SourceWord(memory, cpu, 0));
    Advance(cpu, 2u);
    BlockStore16(memory, cpu, cpu->accumulator);
    MemberName(memory, cpu);
    LoadY16(cpu, 0x0066u);
    LoadA16(cpu, 0x0007u);
    StoreAAbsolute16(memory, cpu, COUNT, 0);
    do {                                                       /* equipment */
        LoadA16(cpu, SourceWord(memory, cpu, 0));
        BlockStore16(memory, cpu, cpu->accumulator);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Decrement16Absolute(memory, cpu, COUNT);
    } while (!cpu->zero);
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
    LoadY16(cpu, 0x000eu);
    LoadA16(cpu, SourceWord(memory, cpu, 0));
    Advance(cpu, 2u);
    BlockStore16(memory, cpu, cpu->accumulator);
    MemberName(memory, cpu);
    LoadY16(cpu, 0x0066u);
    LoadA16(cpu, 0x0007u);
    StoreAAbsolute16(memory, cpu, COUNT, 0);
    do {                                                       /* no equipment */
        TransferDirectToA(cpu);
        BlockStore16(memory, cpu, cpu->accumulator);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Decrement16Absolute(memory, cpu, COUNT);
    } while (!cpu->zero);
    LoadY16(cpu, 0x0074u);
    LoadA16(cpu, 0x0009u);
    StoreAAbsolute16(memory, cpu, COUNT, 0);
    do {                                                       /* bonuses */
        LoadA16(cpu, SourceWord(memory, cpu, 0));
        BlockStore16(memory, cpu, cpu->accumulator);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Decrement16Absolute(memory, cpu, COUNT);
    } while (!cpu->zero);
    LoadY16(cpu, 0x00bcu);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);
    BlockStore8(memory, cpu, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    MemberFinish(memory, cpu, kReturns);
    return ExecutionReturned(0x81ef92u);
}

static uint16_t Abs16X(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t offset) {
    return Read16AbsoluteIndexed(memory, cpu, offset, cpu->x);
}

/* CLC, then ADC of each field. */
static void SumFields(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t first, uint16_t second, uint16_t third) {
    LoadA16(cpu, Abs16X(memory, cpu, first));
    cpu->carry = 0;
    Add16Value(cpu, Abs16X(memory, cpu, second));
    if (third)
        Add16Value(cpu, Abs16X(memory, cpu, third));
}

/* $81:F4ED: member $C1 stats = base + equipment bonuses, caps. */
Lufia2ExecutionResult Lufia2PartyStatTotals(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, Read16Direct(memory, cpu, 0xc1u));
    SetAccumulatorWidth(cpu, 0);
    SumFields(memory, cpu, 0x0051u, 0x0074u, 0);
    StoreAAbsolute16(memory, cpu, 0x0025u, cpu->x);
    SumFields(memory, cpu, 0x0053u, 0x0076u, 0);
    StoreAAbsolute16(memory, cpu, 0x0027u, cpu->x);
    SumFields(memory, cpu, 0x005du, 0x0084u, 0x0092u);
    StoreAAbsolute16(memory, cpu, 0x0035u, cpu->x);
    SumFields(memory, cpu, 0x005bu, 0x0082u, 0x0090u);
    Compare16(cpu, cpu->accumulator, 0x00c8u);
    if (cpu->carry)
        LoadA16(cpu, 0x00c7u);
    StoreAAbsolute16(memory, cpu, 0x0033u, cpu->x);
    SumFields(memory, cpu, 0x0059u, 0x0080u, 0x008eu);
    StoreAAbsolute16(memory, cpu, 0x0031u, cpu->x);
    SumFields(memory, cpu, 0x0057u, 0x007eu, 0x008cu);
    StoreAAbsolute16(memory, cpu, 0x002fu, cpu->x);
    SumFields(memory, cpu, 0x0055u, 0x007cu, 0x008au);
    StoreAAbsolute16(memory, cpu, 0x002du, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Abs16X(memory, cpu, 0x0086u));
    StoreAAbsolute16(memory, cpu, 0x0029u, cpu->x);
    LoadA16(cpu, Abs16X(memory, cpu, 0x0078u));
    if (!cpu->zero)
        StoreAAbsolute16(memory, cpu, 0x0029u, cpu->x);
    SumFields(memory, cpu, 0x002fu, 0x002du, 0);
    cpu->carry = cpu->accumulator & 1u;
    LoadA16(cpu, (uint16_t)(cpu->accumulator >> 1));
    cpu->carry = 0;
    Add16Value(cpu, Abs16X(memory, cpu, 0x0088u));
    StoreAAbsolute16(memory, cpu, 0x002bu, cpu->x);
    LoadA16(cpu, Abs16X(memory, cpu, 0x007au));
    if (!cpu->zero)
        StoreAAbsolute16(memory, cpu, 0x002bu, cpu->x);
    SetAccumulatorWidth(cpu, 1);
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
