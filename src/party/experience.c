/* Party experience curves ($81:F9E9). */

#include "core/cpu_internal.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/party.h"

enum {
    PARTY_MEMBER = 0x09fau,
    PARTY_LEVEL = 0x09feu,
    PARTY_EXPERIENCE = 0x0a2au, /* 24-bit, $0A2A-$0A2C */
    PARTY_EXPERIENCE_HIGH = 0x0a2cu
};

/* Curve level cap and the experience from there on. */
enum {
    LEVEL_CAP = 0x62u,
    EXPERIENCE_AT_CAP = 0x98967fu /* 9,999,999 */
};

/* Bank $97 growth factor rows and first steps. */
enum {
    GROWTH_TABLE = 0xb633u,
    GROWTH_ROW_SIZE = 0x70u,
    GROWTH_FACTORS_PER_ROW_STEP = 8u,
    GROWTH_START_STEP = 0xb99eu
};

/* Work bytes. */
enum {
    DP_STEP = 0x58u, /* 32-bit growth step */
    DP_SUM = 0x63u,  /* 32-bit running sum, scaled by 256 */
    DP_PRODUCT_LOW = 0x54u,
    DP_PRODUCT_MID = 0x55u,
    DP_PRODUCT_HIGH = 0x56u,
    DP_PRODUCT_TOP = 0x57u
};

/* Step += step * factor / 256 via byte products. */
static void PartyGrowStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram dp = WramViewOfCaller(memory, cpu);
    const Lufia2Wram bus = dp; /* registers are reached through the data bank */
    uint16_t top_product, byte1_product, byte2_product, byte0_product;
    uint32_t sum;
    unsigned carry;

    WramWrite(bus, SNES_WRMPYB, WramRead(dp, DP_STEP + 3u));
    PushIndex(memory, cpu);
    top_product = WramRead16(bus, SNES_RDMPYL);
    WramWrite(bus, SNES_WRMPYB, WramRead(dp, DP_STEP + 1u));
    WramWrite16(dp, DP_PRODUCT_HIGH, top_product);
    byte1_product = WramRead16(bus, SNES_RDMPYL);
    WramWrite(bus, SNES_WRMPYB, WramRead(dp, DP_STEP + 2u));
    WramWrite16(dp, DP_PRODUCT_LOW, byte1_product);
    byte2_product = WramRead16(bus, SNES_RDMPYL);
    WramWrite(bus, SNES_WRMPYB, WramRead(dp, DP_STEP));
    byte0_product = WramRead16(bus, SNES_RDMPYL);

    /* DP low byte is the lowest addend. */
    sum = (((uint32_t)WramRead(dp, DP_PRODUCT_LOW) << 8) |
           ((cpu->direct_page >> 8) & 0xffu)) +
          byte0_product;
    carry = sum >> 16;
    WramWrite(dp, DP_PRODUCT_LOW, (uint8_t)(sum >> 8));

    sum = (uint32_t)byte2_product + WramRead16(dp, DP_PRODUCT_MID) + carry;
    WramWrite16(dp, DP_PRODUCT_MID, (uint16_t)sum);
    if (sum >> 16)
        WramWrite16(dp, DP_PRODUCT_TOP,
                    (uint16_t)(WramRead16(dp, DP_PRODUCT_TOP) + 1u));

    sum = (uint32_t)WramRead16(dp, DP_PRODUCT_LOW) + WramRead16(dp, DP_STEP);
    carry = sum >> 16;
    WramWrite16(dp, DP_STEP, (uint16_t)sum);
    sum = (uint32_t)WramRead16(dp, DP_PRODUCT_HIGH) + WramRead16(dp, DP_STEP + 2u) +
          carry;
    WramWrite16(dp, DP_STEP + 2u, (uint16_t)sum);
    SetAccumulatorWidth(cpu, 1);
    cpu->x = PullIndexValue(memory, cpu);
}

/* Growth factor of row Y, plus one, into the multiplier. */
static void SelectGrowthFactor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram bus = WramViewOfCaller(memory, cpu);

    LoadA8(cpu, (uint8_t)(Read8(memory, AbsoluteIndexedAddress(cpu, 0u, cpu->y)) + 1u));
    WramWrite(bus, SNES_WRMPYA, A8(cpu));
}

/* $81:F9E9: experience needed for level $09FE of $09FA. */
Lufia2ExecutionResult Lufia2PartyExperienceForLevel(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram dp = WramViewOfCaller(memory, cpu);
    Lufia2Wram bus;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81f9e9u);
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x97u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    PushIndex(memory, cpu);
    bus = WramViewOfCaller(memory, cpu);

    /* Y = member's growth row, X = member * 2. */
    LoadA8(cpu, GROWTH_ROW_SIZE);
    WramWrite(bus, SNES_WRMPYA, A8(cpu));
    TransferDirectToA(cpu);
    LoadA8(cpu, WramRead(bus, PARTY_MEMBER));
    WramWrite(bus, SNES_WRMPYB, A8(cpu));
    AslA8(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, GROWTH_TABLE);
    cpu->carry = 0;
    Add16Value(cpu, WramRead16(bus, SNES_RDMPYL));
    TransferAToY(cpu);

    /* The start step is stored big-endian in the table. */
    LoadA16(cpu, WramRead16At(bus, GROWTH_START_STEP, cpu->x));
    ExchangeAccumulatorBytes(cpu);
    WramWrite16(dp, DP_STEP, cpu->accumulator);
    WramWrite16(dp, DP_STEP + 2u, 0u);
    WramWrite16(dp, DP_SUM, 0u);
    WramWrite16(dp, DP_SUM + 2u, 0u);
    SetAccumulatorWidth(cpu, 1);

    LoadA8(cpu, WramRead(bus, PARTY_LEVEL));
    Compare8(cpu, A8(cpu), LEVEL_CAP);
    if (cpu->carry) {
        LoadX16(cpu, (uint16_t)(EXPERIENCE_AT_CAP & 0xffffu));
        WramWrite16(bus, PARTY_EXPERIENCE, cpu->x);
        LoadA8(cpu, (uint8_t)(EXPERIENCE_AT_CAP >> 16));
        WramWrite(bus, PARTY_EXPERIENCE_HIGH, A8(cpu));
    } else {
        SelectGrowthFactor(memory, cpu);
        LoadX16(cpu, 1u);
        for (;;) {
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, WramRead16(dp, DP_STEP));
            cpu->carry = 0;
            Add16Value(cpu, WramRead16(dp, DP_SUM));
            WramWrite16(dp, DP_SUM, cpu->accumulator);
            LoadA16(cpu, WramRead16(dp, DP_STEP + 2u));
            Add16Value(cpu, WramRead16(dp, DP_SUM + 2u));
            WramWrite16(dp, DP_SUM + 2u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            Compare16(cpu, cpu->x, WramRead16(bus, PARTY_LEVEL));
            if (cpu->zero)
                break;
            /* A new growth factor every eight levels. */
            TransferXToA(cpu);
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            And8(cpu, GROWTH_FACTORS_PER_ROW_STEP - 1u);
            if (cpu->zero) {
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, cpu->y);
                cpu->carry = 0;
                Add16Value(cpu, GROWTH_FACTORS_PER_ROW_STEP);
                TransferAToY(cpu);
                SetAccumulatorWidth(cpu, 1);
                SelectGrowthFactor(memory, cpu);
            }
            PartyGrowStep(memory, cpu);
            IncrementX16(cpu);
        }
        /* Requirement: sum above the low byte, minus ten. */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, WramRead16(dp, DP_SUM + 1u));
        Subtract16(cpu, 0x000au);
        WramWrite16(bus, PARTY_EXPERIENCE, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, WramRead(dp, DP_SUM + 3u));
        Sbc8(cpu, 0x00u);
        WramWrite(bus, PARTY_EXPERIENCE_HIGH, A8(cpu));
    }
    cpu->x = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81fac8u);
}
