/* Capsule monsters: stats ($82:C261) and skills ($82:C4B3, $82:CD1F). */

#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"
#include "lufia2/party.h"
#include "lufia2/system.h"
#include "party/party_internal.h"

enum {
    CAPSULE = 0x11a3u, /* monster 0-6 */
    FORM = 0x11a4u,
    LEVEL = 0x10edu,
    BLOCK = 0x10dfu,              /* stats block for $81:F4D5 */
    STEP = 0x09d5u,               /* 32-bit experience step */
    EXPERIENCE = 0x1141u,         /* 24-bit */
    RECORD = 0x09c4u,             /* pointer into bank $97 */
    SAVED_LEVELS = 0x7ff1a3u,     /* level per monster */
    SAVED_EXPERIENCE = 0x7ff1aau, /* 24-bit experience per monster */
    SAVED_BLOCKS = 0x7ff180u,     /* 5-byte stats block per monster */
    MONSTER_INDEX = 0x09d9u,      /* n */
    EXPERIENCE_OFFSET = 0x09dbu,  /* 3n */
    BLOCK_OFFSET = 0x09ddu,       /* 5n */
    BLOCK_SIZE = 10u,
    EXPERIENCE_CAP = 0x98967fu, /* 9,999,999 */
    EXPERIENCE_CAP_LEVEL = 0x62u,
    GROWTH_FACTORS = 0x8ee4cbu, /* one per 8 levels, plus 1 */
    MULTIPLICAND = 0x4202u,
    MULTIPLIER = 0x4203u,
    PRODUCT = 0x4216u,
    FLAG_ANY = 0x11a5u,
    FLAG_LIST = 0x11c2u,
    FLAG_LIST_SIZE = 0x14u,
    /* Direct-page scratch of the experience step. */
    DP_PRODUCT_0 = 0x11u,
    DP_STEP_ADD = 0x12u,
    /* Direct-page scratch of the stat growth. */
    GROWTH_SUM = 0x11u,
    GROWTH_FORM = 0x13u,
    GROWTH_COUNTER = 0x15u,
    GROWTH_ROW = 0x17u,
    GROWTH_RATE = 0x1cu,
    GROWTH_RATES = 0xa6f420u,
};

static void Jsr(const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t ret) {
    SimulateJsrFrame(memory, cpu, ret);
}

static void Rts(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateRtsFrame(memory, cpu);
}

static uint16_t Word(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t address) {
    return Read16AbsoluteIndexed(memory, cpu, address, 0);
}

static void StoreWord(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t address) {
    Write16Absolute(memory, cpu, address, cpu->accumulator);
}

/* $82:C3D3: $09D9 = n, $09DB = 3n, $09DD = 5n. */
static void CapsuleIndices(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t n = WramRead16(wram, CAPSULE) & 0x00ffu;

    WramWrite16(wram, MONSTER_INDEX, n);
    WramWrite16(wram, EXPERIENCE_OFFSET, (uint16_t)(3u * n));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(4u * n));
    cpu->carry = 0;
    Add16Value(cpu, n);
    WramWrite16(wram, BLOCK_OFFSET, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $82:C3F8: level and saved stats from $7F:F180-$7F:F1AC. */
static void CapsuleLoad(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram saved = WramViewLong(memory);
    uint16_t index;
    uint16_t offset;
    unsigned i;

    Jsr(memory, cpu, 0xc3fau);
    CapsuleIndices(memory, cpu);
    Rts(memory, cpu);
    for (i = 0; i < BLOCK_SIZE; ++i)
        WramWriteAt(wram, BLOCK, (uint16_t)i, 0);
    index = WramRead16(wram, MONSTER_INDEX);
    WramWrite(wram, LEVEL, WramReadAt(saved, SAVED_LEVELS, index));
    offset = WramRead16(wram, EXPERIENCE_OFFSET);
    WramWrite16(wram, 0x113eu, WramRead16At(saved, SAVED_EXPERIENCE, offset));
    WramWrite(wram, 0x1140u, WramReadAt(saved, SAVED_EXPERIENCE + 2u, offset));
    offset = WramRead16(wram, BLOCK_OFFSET);
    WramWrite16(wram, BLOCK, WramRead16At(saved, SAVED_BLOCKS, offset));
    WramWrite16(wram, BLOCK + 2u, WramRead16At(saved, SAVED_BLOCKS + 2u, offset));
    WramWrite(wram, BLOCK + 4u, WramReadAt(saved, SAVED_BLOCKS + 4u, offset));
    /* Registers as the last loads left them. */
    cpu->x = offset;
    cpu->accumulator =
        (uint16_t)(WramRead16(wram, BLOCK + 2u) & 0xff00u) | WramRead(wram, BLOCK + 4u);
    SetNz8(cpu, WramRead(wram, BLOCK + 4u));
    cpu->carry = 1;
}

/* $82:CEAB: add the step, cap 9,999,999, grow the step. */
static void CapsuleLevelStep(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint32_t experience = 0;
    uint32_t added = 0;
    uint32_t step = 0;
    uint32_t grown;
    uint32_t gain;
    uint16_t product[4];
    uint8_t factor;
    unsigned i;

    for (i = 0; i < 3u; ++i) {
        experience |= (uint32_t)WramRead(wram, EXPERIENCE + i) << (8u * i);
        added |= (uint32_t)WramRead(wram, STEP + 1u + i) << (8u * i);
    }
    experience = (experience + added) & 0x00ffffffu;
    for (i = 0; i < 3u; ++i)
        WramWrite(wram, EXPERIENCE + i, (uint8_t)(experience >> (8u * i)));
    if (experience >= EXPERIENCE_CAP) {
        WramWrite(wram, EXPERIENCE, (uint8_t)EXPERIENCE_CAP);
        WramWrite(wram, EXPERIENCE + 1u, (uint8_t)(EXPERIENCE_CAP >> 8));
        WramWrite(wram, EXPERIENCE + 2u, (uint8_t)(EXPERIENCE_CAP >> 16));
    }

    /* The step grows by step * factor / 256, where the factor comes from a
     * table with one entry per 8 levels. */
    factor =
        (uint8_t)(Read8(memory,
                        LongIndexedAddress(
                            GROWTH_FACTORS,
                            (uint16_t)(((WramRead16(wram, LEVEL) & 0x00ffu) + 1u) >>
                                       3))) +
                  1u);
    WramWrite(wram, MULTIPLICAND, factor);
    for (i = 0; i < 4u; ++i) {
        WramWrite(wram, MULTIPLIER, WramRead(wram, STEP + i));
        product[i] = WramRead16(wram, PRODUCT);
        cpu->x = product[i];
        WramWrite16(wram, DP_PRODUCT_0 + 2u * i, product[i]);
    }
    gain = (uint32_t)(product[0] >> 8) + product[1] + ((uint32_t)product[2] << 8) +
           ((uint32_t)product[3] << 16);
    for (i = 0; i < 4u; ++i)
        WramWrite(wram, DP_STEP_ADD + i, (uint8_t)(gain >> (8u * i)));
    for (i = 0; i < 4u; ++i)
        step |= (uint32_t)WramRead(wram, STEP + i) << (8u * i);
    grown = step + gain;
    for (i = 0; i < 3u; ++i)
        WramWrite(wram, STEP + i, (uint8_t)(grown >> (8u * i)));
    /* The top byte's addition leaves the carry and overflow flags. */
    cpu->carry = (uint8_t)((((step & 0x00ffffffu) + (gain & 0x00ffffffu)) >> 24) & 1u);
    cpu->accumulator = (uint8_t)(step >> 24);
    Adc8(cpu, (uint8_t)(gain >> 24));
    WramWrite(wram, STEP + 3u, A8(cpu));
}

/* $82:CE52: experience for the level, minus 10. The level is counted up from
 * zero, one step per level, and the monster's level byte ends up zero. */
static void CapsuleExperience(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint8_t remaining;
    uint32_t experience;
    unsigned i;

    LoadAAbsolute8(memory, cpu, LEVEL, 0);
    Compare8(cpu, A8(cpu), EXPERIENCE_CAP_LEVEL);
    if (cpu->carry) {
        WramWrite(wram, EXPERIENCE, (uint8_t)EXPERIENCE_CAP);
        WramWrite(wram, EXPERIENCE + 1u, (uint8_t)(EXPERIENCE_CAP >> 8));
        WramWrite(wram, EXPERIENCE + 2u, (uint8_t)(EXPERIENCE_CAP >> 16));
        LoadA8(cpu, (uint8_t)(EXPERIENCE_CAP >> 16));
        return;
    }
    for (i = 0; i < 4u; ++i)
        WramWrite(wram, STEP + i, i == 1u ? 0x14u : 0u);
    for (i = 0; i < 3u; ++i)
        WramWrite(wram, EXPERIENCE + i, 0);
    WramWrite(wram, LEVEL, 0);
    do {
        PushAccumulator8(memory, cpu);
        WramWrite(wram, LEVEL, (uint8_t)(WramRead(wram, LEVEL) + 1u));
        Jsr(memory, cpu, 0xce8cu);
        CapsuleLevelStep(memory, cpu);
        Rts(memory, cpu);
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
        remaining = A8(cpu);
    } while (remaining != 0);
    experience = (uint32_t)WramRead(wram, EXPERIENCE) |
                 ((uint32_t)WramRead(wram, EXPERIENCE + 1u) << 8) |
                 ((uint32_t)WramRead(wram, EXPERIENCE + 2u) << 16);
    for (i = 0; i < 3u; ++i)
        WramWrite(wram, EXPERIENCE + i, (uint8_t)((experience - 10u) >> (8u * i)));
    /* The top byte's subtraction leaves the flags. */
    cpu->carry = (experience & 0xffffu) >= 10u;
    LoadA8(cpu, (uint8_t)(experience >> 16));
    Sbc8(cpu, 0x00u);
}

/* $82:F6A4 / $82:F6D4: clear bonus words of the block at [$2A]. */
static void ClearBonuses(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t first, unsigned words) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    unsigned i;

    LoadX16(cpu, WramRead16(wram, 0x2au));
    for (i = 0; i < words; ++i) {
        /* The bonus words sit at absolute addresses below $100, so the
         * index is added without the direct page. */
        const uint32_t at = AbsoluteIndexedAddress(
            cpu, (uint16_t)(first + 2u * i), cpu->x);

        Write8(memory, at, 0);
        Write8(memory, (at + 1u) & 0x00ffffffu, 0);
    }
}

/* $82:C38B: $09C4 = record of monster and form ($97:DCB8). */
static void CapsuleRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint8_t monster = WramRead(wram, CAPSULE);
    const uint8_t form = WramRead(wram, FORM);
    const uint8_t record = (uint8_t)(((monster & 7u) << 2) + monster + form - 1u);

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x97dcb8u, 2u * record)));
    cpu->carry = 0;
    Add16Value(cpu, 0xdcb8u);
    TransferAToX(cpu);
    WramWrite16(wram, RECORD, cpu->x);
    SetAccumulatorWidth(cpu, 1);
}

/* Shifts a word right in place, high byte stored first. */
static void ShiftWordRight(Lufia2Wram wram, uint32_t location) {
    const uint32_t low = WramAddress(wram, location, 0);
    const uint16_t value = (uint16_t)(WramRead16(wram, location) >> 1);

    Write8(wram.memory, WramNextAddress(location, low), (uint8_t)(value >> 8));
    Write8(wram.memory, low, (uint8_t)value);
}

/* $82:D31C: growth of a stat over the levels, / 16. The stat's growth rate is
 * taken from the table at $A6:F420 in steps of 16 levels; a level at or past
 * the form number only counts a half or a quarter. The running values live in
 * direct-page scratch. */
static void CapsuleGrowth(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t level = WramRead16(wram, LEVEL) & 0x00ffu;

    WramWrite16(wram, GROWTH_ROW, (uint16_t)((cpu->accumulator & 0x00ffu) << 3));
    WramWrite16(wram, GROWTH_SUM, 0);
    if (level != 1u) {
        uint16_t counter;

        WramWrite16(wram, GROWTH_FORM, WramRead16(wram, FORM));
        WramWrite16(wram, GROWTH_FORM + 1u, 0);
        WramWrite16(wram, GROWTH_COUNTER, 1);
        do {
            uint16_t block;

            counter = WramRead16(wram, GROWTH_COUNTER);
            block = (uint16_t)(counter >> 4);
            cpu->accumulator = block;
            PushAccumulator16(memory, cpu);
            cpu->x = (uint16_t)(block + WramRead16(wram, GROWTH_ROW));
            WramWrite16(wram, GROWTH_RATE,
                        Read16Long(memory, LongIndexedAddress(GROWTH_RATES, cpu->x)) &
                            0x00ffu);
            PullAccumulator16(memory, cpu);
            if (block >= WramRead16(wram, GROWTH_FORM)) {
                ShiftWordRight(wram, GROWTH_RATE);
                if (block != WramRead16(wram, GROWTH_FORM))
                    ShiftWordRight(wram, GROWTH_RATE);
            }
            WramWrite16(wram, GROWTH_SUM,
                        (uint16_t)(WramRead16(wram, GROWTH_RATE) +
                                   WramRead16(wram, GROWTH_SUM)));
            counter = WramStep16(wram, GROWTH_COUNTER, 1);
        } while (counter != level);
    }
    cpu->accumulator = level;
    for (unsigned shift = 0; shift < 4u; ++shift)
        OpLsrMem(memory, cpu, OpDp(cpu, GROWTH_SUM));
}

/* $82:D283: base stats plus growth into the block. */
static void CapsuleStats(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t kStats[6][2] = {
        {0x1du, 0x15u}, {0x1eu, 0x18u}, {0x1fu, 0x19u},
        {0x20u, 0x1au}, {0x21u, 0x1bu}, {0x22u, 0x1cu}};
    static const uint16_t kTargets[6] = {
        0x1130u, 0x1134u, 0x1136u, 0x1138u, 0x113au, 0x113cu};
    static const uint16_t kReturns[6] = {
        0xd2a7u, 0xd2bcu, 0xd2ceu, 0xd2e0u, 0xd2f2u, 0xd304u};
    unsigned i;

    Jsr(memory, cpu, 0xd285u);
    CapsuleRecord(memory, cpu);
    Rts(memory, cpu);
    LoadY16(cpu, Word(memory, cpu, RECORD));
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x97u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0016u, cpu->y));
    And16(cpu, 0x00ffu);
    StoreWord(memory, cpu, 0x1165u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0017u, cpu->y));
    And16(cpu, 0x00ffu);
    StoreWord(memory, cpu, 0x1167u);
    for (i = 0; i < 6u; ++i) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, kStats[i][0], cpu->y));
        Jsr(memory, cpu, kReturns[i]);
        CapsuleGrowth(memory, cpu);
        Rts(memory, cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, kStats[i][1], cpu->y));
        And16(cpu, 0x00ffu);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x11u));
        StoreWord(memory, cpu, kTargets[i]);
        if (i == 0)
            StoreWord(memory, cpu, 0x1104u);
    }
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    LoadX16(cpu, BLOCK);
    SimulateJslFrame(memory, cpu, 0x82u, 0xd31au);
    (void)Lufia2PartyDerivedStats(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:D270: clear bonuses, then stats into the block. */
static void CapsuleBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, BLOCK);
    StoreXDirect16(memory, cpu, 0x2au);
    Jsr(memory, cpu, 0xd279u);
    ClearBonuses(memory, cpu, 0x0086u, 7u);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xd27cu);
    ClearBonuses(memory, cpu, 0x0074u, 9u);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    Jsr(memory, cpu, 0xd281u);
    CapsuleStats(memory, cpu);
    Rts(memory, cpu);
}

/* $82:C261: when $0A7F is 7, build the capsule monster's stats. */
Lufia2ExecutionResult Lufia2CapsuleLoadStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, cpu->program_bank);                     /* PHK PLB */
    PullDataBank(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x0a7fu, 0);
    Compare8(cpu, A8(cpu), 0x07u);
    if (!cpu->zero)
        return ExecutionReturned(0x82c26au);
    Jsr(memory, cpu, 0xc26du);
    CapsuleLoad(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc270u);
    CapsuleExperience(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc273u);
    CapsuleBlock(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc276u);
    CapsuleRecord(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc279u);                                 /* C3C4 */
    LoadAAbsolute8(memory, cpu, CAPSULE, 0);
    AslA8(cpu);
    AslA8(cpu);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, CAPSULE, 0));
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, FORM, 0));
    DecrementA8(cpu);
    Rts(memory, cpu);
    StoreAAbsolute8(memory, cpu, 0x1144u, 0);
    StoreZeroAbsolute8(memory, cpu, 0x10eeu, 0);
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x97u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StoreA8Absolute(memory, cpu, 0x1126u, 0x97u);
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, Word(memory, cpu, RECORD));
    Write16Absolute(memory, cpu, 0x1124u, cpu->y);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0023u, cpu->y));
    StoreWord(memory, cpu, 0x1127u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0025u, cpu->y));
    StoreWord(memory, cpu, 0x1129u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0027u, cpu->y));
    StoreWord(memory, cpu, 0x112bu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0029u, cpu->y));
    StoreWord(memory, cpu, 0x112du);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x82c2adu);
}

/* $82:C37B: clear $11A5 and $11C2-$11D5. */
static void CapsuleClearFlags(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    unsigned i;

    WramWrite(wram, FLAG_ANY, 0);
    for (i = 0; i < FLAG_LIST_SIZE; ++i)
        WramWriteAt(wram, FLAG_LIST, (uint16_t)i, 0);
    LoadX16(cpu, FLAG_LIST_SIZE);
    Compare16(cpu, cpu->x, FLAG_LIST_SIZE);
}

/* $82:C515: forms from $8E:E485 for present monsters. */
Lufia2ExecutionResult Lufia2CapsuleSetForms(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x0a7fu, 0);
    Compare8(cpu, A8(cpu), 0x07u);
    if (!cpu->zero)
        return ExecutionReturned(0x82c539u);
    LoadY16(cpu, 0x0000u);
    do {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11bbu, cpu->y));
        And16(cpu, 0x00ffu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        if (!cpu->zero)
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x8ee485u, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x11a6u, cpu->y);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x0007u);
    } while (!cpu->zero);
    return ExecutionReturned(0x82c539u);
}

/* Tail of $82:C2FD and $82:C352: forms, then stats. */
static void CapsuleRefresh(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t forms_return) {
    SimulateJslFrame(memory, cpu, 0x82u, forms_return);
    (void)Lufia2CapsuleSetForms(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x82u, (uint16_t)(forms_return + 4u));
    (void)Lufia2CapsuleLoadStats(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:C2FD: every capsule back to level 1, then stats. */
Lufia2ExecutionResult Lufia2CapsuleReset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x0a7fu, 0);
    Compare8(cpu, A8(cpu), 0x07u);
    if (!cpu->zero)
        return ExecutionReturned(0x82c351u);
    LoadX16(cpu, 0x0000u);
    LoadA8(cpu, 0x01u);
    do {
        Write8(memory, LongIndexedAddress(0x7ff1a3u, cpu->x), A8(cpu));
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0007u);
    } while (!cpu->zero);
    LoadX16(cpu, 0x0000u);
    LoadA8(cpu, 0x00u);
    do {
        Write8(memory, LongIndexedAddress(0x7ff1aau, cpu->x), A8(cpu));
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0015u);
    } while (!cpu->zero);
    Jsr(memory, cpu, 0xc324u);
    CapsuleClearFlags(memory, cpu);
    Rts(memory, cpu);
    LoadX16(cpu, 0x0000u);
    do {                                                       /* present: 0/1 */
        LoadAAbsolute8(memory, cpu, 0x11bbu, cpu->x);
        cpu->carry = 0;
        Adc8(cpu, 0xffu);
        LoadA8(cpu, 0x00u);
        Adc8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, 0x11bbu, cpu->x);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0007u);
    } while (!cpu->zero);
    StoreA8Absolute(memory, cpu, FORM, 0x01u);
    Jsr(memory, cpu, 0xc342u);
    CapsuleLoad(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc345u);
    CapsuleExperience(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc348u);
    CapsuleBlock(memory, cpu);
    Rts(memory, cpu);
    CapsuleRefresh(memory, cpu, 0xc34cu);
    return ExecutionReturned(0x82c351u);
}

/* $82:C352: present flags = A, then capsule 0 at form 1. */
Lufia2ExecutionResult Lufia2CapsuleSetAll(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    for (i = 0; i < 7u; ++i)
        StoreAAbsolute8(memory, cpu, (uint16_t)(0x11bbu + i), 0);
    Jsr(memory, cpu, 0xc369u);
    CapsuleClearFlags(memory, cpu);
    Rts(memory, cpu);
    StoreZeroAbsolute8(memory, cpu, CAPSULE, 0);
    StoreA8Absolute(memory, cpu, FORM, 0x01u);
    CapsuleRefresh(memory, cpu, 0xc375u);
    return ExecutionReturned(0x82c37au);
}

/* $82:C443: level, experience and stats back to $7F:F180+. */
static void CapsuleSave(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram saved = WramViewLong(memory);
    uint16_t offset;

    Jsr(memory, cpu, 0xc445u);
    CapsuleIndices(memory, cpu);
    Rts(memory, cpu);
    WramWriteAt(saved, SAVED_LEVELS, WramRead16(wram, MONSTER_INDEX),
                WramRead(wram, LEVEL));
    offset = WramRead16(wram, EXPERIENCE_OFFSET);
    WramWrite16At(saved, SAVED_EXPERIENCE, offset, WramRead16(wram, 0x113eu));
    WramWriteAt(saved, SAVED_EXPERIENCE + 2u, offset, WramRead(wram, 0x1140u));
    offset = WramRead16(wram, BLOCK_OFFSET);
    WramWrite16At(saved, SAVED_BLOCKS, offset, WramRead16(wram, BLOCK));
    WramWrite16At(saved, SAVED_BLOCKS + 2u, offset, WramRead16(wram, BLOCK + 2u));
    WramWriteAt(saved, SAVED_BLOCKS + 4u, offset, WramRead(wram, BLOCK + 4u));
    /* Registers as the last loads left them. */
    cpu->x = offset;
    cpu->accumulator =
        (uint16_t)(WramRead16(wram, BLOCK + 2u) & 0xff00u) | WramRead(wram, BLOCK + 4u);
    SetNz8(cpu, WramRead(wram, BLOCK + 4u));
}

/* $82:CD83: level up if due; carry clear with gains in $0A38. */
Lufia2ExecutionResult Lufia2CapsuleLevelUp(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t kStats[6] = {
        0x1104u, 0x110cu, 0x110eu, 0x1110u, 0x1112u, 0x1114u};
    static const uint8_t kGains[6] = {0xdfu, 0xe1u, 0xe3u, 0xe5u, 0xe7u, 0xeau};
    static const uint16_t kOut[6] = {
        0x0a38u, 0x0a3au, 0x0a3bu, 0x0a3cu, 0x0a3du, 0x0a3eu};
    unsigned i;
    int due = 0;

    LoadAAbsolute8(memory, cpu, LEVEL, 0);
    Compare8(cpu, A8(cpu), 0x63u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x113eu, 0);
        cpu->carry = 1;
        Sbc8(cpu, AbsoluteByte(memory, cpu, EXPERIENCE, 0));
        LoadAAbsolute8(memory, cpu, 0x113fu, 0);
        Sbc8(cpu, AbsoluteByte(memory, cpu, (uint16_t)(EXPERIENCE + 1u), 0));
        LoadAAbsolute8(memory, cpu, 0x1140u, 0);
        Sbc8(cpu, AbsoluteByte(memory, cpu, (uint16_t)(EXPERIENCE + 2u), 0));
        due = cpu->carry;
    }
    if (!due) {
        Jsr(memory, cpu, 0xcda1u);
        CapsuleSave(memory, cpu);
        Rts(memory, cpu);
        cpu->carry = 1;
        return ExecutionReturned(0x82cda3u);
    }
    SetAccumulatorWidth(cpu, 0);
    for (i = 0; i < 6u; ++i) {
        LoadA16(cpu, Word(memory, cpu, kStats[i]));
        StoreADirect16(memory, cpu, kGains[i]);
    }
    SetAccumulatorWidth(cpu, 1);
    {
        const uint32_t level = AbsoluteIndexedAddress(cpu, LEVEL, 0);
        const uint8_t value = (uint8_t)(Read8(memory, level) + 1u);

        Write8(memory, level, value);
        SetNz8(cpu, value);
    }
    Jsr(memory, cpu, 0xcdcbu);
    CapsuleExperience(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xcdceu);
    CapsuleBlock(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    for (i = 0; i < 6u; ++i) {
        LoadA16(cpu, Word(memory, cpu, kStats[i]));
        Subtract16(cpu, Read16Direct(memory, cpu, kGains[i]));
        StoreADirect16(memory, cpu, kGains[i]);
    }
    SetAccumulatorWidth(cpu, 1);
    for (i = 0; i < 6u; ++i) {
        LoadA8(cpu, DirectByte(memory, cpu, kGains[i]));
        StoreAAbsolute8(memory, cpu, kOut[i], 0);
    }
    cpu->carry = 0;
    return ExecutionReturned(0x82ce22u);
}

void Lufia2CapsuleRecordPointer(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    CapsuleRecord(memory, cpu);
}

/* $82:CE23: experience at the level start ($113E) and the next. */
Lufia2ExecutionResult Lufia2CapsuleExperienceRange(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    StoreZeroAbsolute8(memory, cpu, 0x113eu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x113fu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x1140u, 0);
    LoadAAbsolute8(memory, cpu, LEVEL, 0);
    Compare8(cpu, A8(cpu), 0x01u);
    if (!cpu->zero) {
        const uint32_t level = AbsoluteIndexedAddress(cpu, LEVEL, 0);

        Write8(memory, level, (uint8_t)(Read8(memory, level) - 1u));
        Jsr(memory, cpu, 0xce38u);
        CapsuleExperience(memory, cpu);
        Rts(memory, cpu);
        Write8(memory, level, (uint8_t)(Read8(memory, level) + 1u));
        for (i = 0; i < 3u; ++i) {
            LoadAAbsolute8(memory, cpu, (uint16_t)(EXPERIENCE + i), 0);
            StoreAAbsolute8(memory, cpu, (uint16_t)(0x113eu + i), 0);
        }
    }
    Jsr(memory, cpu, 0xce50u);
    CapsuleExperience(memory, cpu);
    Rts(memory, cpu);
    return ExecutionReturned(0x82ce51u);
}

void Lufia2BonusClear(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t first, unsigned words) {
    ClearBonuses(memory, cpu, first, words);
}

/* $82:CCFE: $11 = learned-bit mask of slot A for the form. */
static void CapsuleSkillMask(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ed8c3u, cpu->x)));
    StoreADirect16(memory, cpu, 0x11u);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x11a4u, 0) & 0x00ffu));
    for (;;) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));       /* CD19 */
        if (cpu->zero)
            break;
        {
            uint16_t mask = Read16Direct(memory, cpu, 0x11u);

            mask = (uint16_t)(mask << 3);
            Write16Direct(memory, cpu, 0x11u, mask);
        }
    }
    SetAccumulatorWidth(cpu, 1);
}

/* $82:C4B3: skill id of slot A (learned or not). */
void Lufia2CapsuleSkill(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect8(memory, cpu, 0x19u);
    Write8(memory, DirectAddress(cpu, 0x1au), 0);
    Jsr(memory, cpu, 0xc4b9u);
    CapsuleSkillMask(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc4beu);
    Lufia2CapsuleRecordPointer(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xc4c1u);                                 /* C4A2 */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x11a3u, 0) & 0x00ffu));
    AslA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x11cau);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    Rts(memory, cpu);
    LoadA8(cpu, 0x97u);
    StoreADirect8(memory, cpu, 0x1bu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09c4u, 0));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x19u));
    StoreADirect16(memory, cpu, 0x19u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    {
        const uint16_t mask = Read16Direct(memory, cpu, 0x11u);

        cpu->zero = (cpu->accumulator & mask) == 0;            /* BIT $11 */
        cpu->negative = (mask & 0x8000u) != 0;
        cpu->overflow = (mask & 0x4000u) != 0;
    }
    LoadY16(cpu, cpu->zero ? 0x000fu : 0x0012u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x19u));
}

/* $82:C4A2: X = skill flags of the capsule ($11CA + 2n). */
static void CapsuleSkillFlags(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x11a3u, 0) & 0x00ffu));
    AslA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x11cau);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $82:CD41: learn skill slot A if the form has it; carry = no. */
static void CapsuleLearn(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect8(memory, cpu, 0x13u);
    Write8(memory, DirectAddress(cpu, 0x14u), 0);
    Jsr(memory, cpu, 0xcd47u);
    CapsuleSkillMask(memory, cpu);
    Rts(memory, cpu);
    Jsr(memory, cpu, 0xcd4au);
    CapsuleSkillFlags(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    {
        const uint16_t mask = Read16Direct(memory, cpu, 0x11u);  /* BIT $11 */

        cpu->zero = (cpu->accumulator & mask) == 0;
        cpu->negative = (mask & 0x8000u) != 0;
        cpu->overflow = (mask & 0x4000u) != 0;
    }
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 1);
        cpu->carry = 1;
        return;
    }
    SetAccumulatorWidth(cpu, 1);
    Jsr(memory, cpu, 0xcd58u);
    Lufia2CapsuleRecordPointer(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, 0x97u);
    StoreADirect8(memory, cpu, 0x1bu);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x13u));
    StoreADirect16(memory, cpu, 0x19u);
    SetAccumulatorWidth(cpu, 1);
    LoadY16(cpu, 0x0012u);
    LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x19u));
    if (cpu->zero) {
        cpu->carry = 1;
        return;
    }
    Jsr(memory, cpu, 0xcd70u);
    CapsuleSkillFlags(memory, cpu);
    Rts(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x) |
        Read16Direct(memory, cpu, 0x11u)));
    StoreAAbsolute16(memory, cpu, 0x0000u, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    cpu->carry = 0;
}

static void RandomBelow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t limit, uint16_t return_address) {
    LoadA8(cpu, limit);
    SimulateJslFrame(memory, cpu, 0x82u, return_address);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:CD1F: 1 in 8, learn a random skill; carry clear, A = skill. */
Lufia2ExecutionResult Lufia2CapsuleTryLearn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    RandomBelow(memory, cpu, 0x08u, 0xcd24u);
    Compare8(cpu, A8(cpu), 0x00u);
    if (!cpu->zero) {
        cpu->carry = 1;
        return ExecutionReturned(0x82cd40u);
    }
    RandomBelow(memory, cpu, 0x03u, 0xcd2eu);
    Compare8(cpu, A8(cpu), 0x03u);
    if (cpu->carry)
        return ExecutionReturned(0x82cd40u);
    PushAccumulator8(memory, cpu);
    Jsr(memory, cpu, 0xcd36u);
    CapsuleLearn(memory, cpu);
    Rts(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    if (cpu->carry)
        return ExecutionReturned(0x82cd40u);
    Jsr(memory, cpu, 0xcd3cu);
    Lufia2CapsuleSkill(memory, cpu);
    Rts(memory, cpu);
    cpu->carry = 0;
    return ExecutionReturned(0x82cd3eu);
}
