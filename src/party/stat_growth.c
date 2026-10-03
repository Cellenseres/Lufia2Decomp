#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/party.h"
#include <stdbool.h>

typedef struct GrowthContext {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    uint32_t unwind_site;
} GrowthContext;

static bool GrowthCall(GrowthContext *growth, uint16_t site, uint32_t target,
                       uint8_t frame) {
    if (frame == 2u)
        SimulateJsrFrame(growth->memory, growth->cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(growth->memory, growth->cpu, 0x81u, (uint16_t)(site + 3u));
    if (growth->child(growth->context, growth->cpu, target, 0x810000u | site, frame))
        return true;
    growth->unwind_site = 0x810000u | site;
    return false;
}

static Lufia2ExecutionResult GrowthUnwound(const GrowthContext *growth) {
    Lufia2ExecutionResult result = ExecutionReturned(growth->unwind_site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

enum {
    GROWTH_STAT_COUNT = 7,
    GROWTH_ROLL_LIMIT = 5,      /* each level gain is rolled below 5 */
    GROWTH_RECORD_STATS = 0x51, /* stat words sit this far past the member offset */
    GROWTH_DP_STAT_POINTER = 0xb2,
    GROWTH_RECORD_LEVEL_BYTE = 0x0e,
    GROWTH_LEVEL = 0x09feu,
    GROWTH_STAT_CAPS = 0x0a1cu, /* 7 words */
    GROWTH_GAINS = 0x0a38u,     /* 7 bytes */
    GROWTH_ROLL_ENTRIES = GROWTH_STAT_COUNT - 1,
    GROWTH_UNROLLED = 5, /* the sixth gain is always zero */
    GROWTH_BOUNDARY_PC = 0x81f87eu,
};

/* Address of stat word `stat` of the member whose record the pointer in $B2
 * selects. */
static uint32_t GrowthStat(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
                           unsigned stat) {
    return (OpAbs(cpu, Read16Direct(memory, cpu, GROWTH_DP_STAT_POINTER)) + 2u * stat) &
           0xffffffu;
}

/* The calibration BRK: a gain that would overshoot the cap by five or more.
 * Leaves the registers as the code did and asks the caller to stop there. */
static Lufia2ExecutionResult GrowthBoundary(Lufia2CpuState *cpu, uint16_t shortfall,
                                            uint8_t gain, unsigned stat) {
    cpu->accumulator = shortfall;
    OpSepWidths(cpu, 0x20u);
    cpu->x = (uint16_t)stat;
    cpu->y = (uint16_t)(2u * stat);
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpIncA(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, gain);
    OpCmpValue(cpu, GROWTH_ROLL_LIMIT);
    cpu->resume_pc = GROWTH_BOUNDARY_PC;
    return (Lufia2ExecutionResult){LUFIA2_EXECUTION_BOUNDARY, GROWTH_BOUNDARY_PC, 0u};
}

/* Pulls each rolled gain back when its stat would pass the cap, and clears the
 * gain of a stat with no cap. Returns false at the BRK. */
static bool GrowthLimitGains(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             Lufia2Wram wram, Lufia2ExecutionResult *boundary) {
    unsigned stat;

    for (stat = 0; stat < GROWTH_STAT_COUNT; ++stat) {
        const uint8_t gain = WramReadAt(wram, GROWTH_GAINS, (uint16_t)stat);
        const uint16_t cap =
            WramRead16At(wram, GROWTH_STAT_CAPS, (uint16_t)(2u * stat));
        const uint16_t room = (uint16_t)(gain + cap);
        const uint16_t value = Read16Long(memory, GrowthStat(memory, cpu, stat));

        if (room < value) {
            const uint16_t shortfall = (uint16_t)(room - value);
            const uint8_t over = (uint8_t)(0u - (uint8_t)shortfall);
            const uint8_t limited = (uint8_t)(over + gain);

            if (limited >= GROWTH_ROLL_LIMIT) {
                *boundary = GrowthBoundary(cpu, shortfall, gain, stat);
                return false;
            }
            WramWriteAt(wram, GROWTH_GAINS, (uint16_t)stat, limited);
        }
        if (WramRead16At(wram, GROWTH_STAT_CAPS, (uint16_t)(2u * stat)) == 0u)
            WramWrite16At(wram, GROWTH_GAINS, (uint16_t)stat, 0u);
    }
    return true;
}

/* Turns each gain into the stat's new value: the gain byte becomes what the
 * stat actually moves by and the stat word becomes gain + cap. The last add
 * leaves its carry and result in the registers. */
static void GrowthApplyGains(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             Lufia2Wram wram) {
    unsigned stat;

    for (stat = 0; stat < GROWTH_STAT_COUNT; ++stat) {
        const uint8_t gain = WramReadAt(wram, GROWTH_GAINS, (uint16_t)stat);
        const uint16_t cap =
            WramRead16At(wram, GROWTH_STAT_CAPS, (uint16_t)(2u * stat));
        const uint16_t before = Read16Long(memory, GrowthStat(memory, cpu, stat));
        const uint16_t delta = (uint16_t)(gain + cap - before);

        WramWriteAt(wram, GROWTH_GAINS, (uint16_t)stat, (uint8_t)delta);
        LoadA16(cpu, delta);
        cpu->carry = 0;
        OpAdcValue(cpu, Read16Long(memory, GrowthStat(memory, cpu, stat)));
        Write16Long(memory, GrowthStat(memory, cpu, stat), cpu->accumulator);
    }
    cpu->x = GROWTH_STAT_COUNT;
    cpu->y = 2u * GROWTH_STAT_COUNT;
    Compare16(cpu, cpu->x, GROWTH_STAT_COUNT);
}

/* $81:F7CA: apply one party level gain and refresh the derived record. */
Lufia2ExecutionResult Lufia2PartyApplyLevel(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *context) {
    GrowthContext growth = {memory, cpu, child, context, 0u};
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, 0x09fau));
    OpSepWidths(cpu, 0x20u);
    if (!GrowthCall(&growth, 0xf7d4u, 0x81f979u, 2u))
        return GrowthUnwound(&growth);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero) {
        cpu->carry = 0;
        return ExecutionReturned(0x81f7ecu);
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x09fau)));
    if (!GrowthCall(&growth, 0xf7deu, 0x81f7bdu, 2u))
        return GrowthUnwound(&growth);
    OpWriteX(memory, cpu, OpDp(cpu, 0xc1u), cpu->x);
    if (!GrowthCall(&growth, 0xf7e3u, 0x81f7edu, 2u) ||
        !GrowthCall(&growth, 0xf7e6u, 0x81f4edu, 2u))
        return GrowthUnwound(&growth);
    cpu->carry = 1;
    return ExecutionReturned(0x81f7eau);
}

/* $81:F7ED: roll a gain for each of the seven stats, limit them to the caps and
 * apply them, keeping the original calibration BRK. */
Lufia2ExecutionResult Lufia2PartyStatGrowth(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *context) {
    GrowthContext growth = {memory, cpu, child, context, 0u};
    Lufia2ExecutionResult boundary;
    Lufia2Wram wram;

    OpPushX(memory, cpu);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x97u);
    wram = WramViewInBank(memory, cpu, 0x97u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, GROWTH_RECORD_LEVEL_BYTE));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, GROWTH_LEVEL));
    OpSepWidths(cpu, 0x20u);
    if (!GrowthCall(&growth, 0xf800u, 0x81f87fu, 3u))
        return GrowthUnwound(&growth);
    OpLdy(cpu, GROWTH_ROLL_ENTRIES);
    do {
        OpLoadA(cpu, GROWTH_ROLL_LIMIT);
        if (!GrowthCall(&growth, 0xf809u, 0x808299u, 3u))
            return GrowthUnwound(&growth);
        OpSta(memory, cpu, OpAbsY(cpu, GROWTH_GAINS));
        OpDey(cpu);
    } while (!cpu->negative);
    OpStz(memory, cpu, OpAbs(cpu, GROWTH_GAINS + GROWTH_UNROLLED));
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, GROWTH_RECORD_STATS);
    OpSta(memory, cpu, OpDp(cpu, GROWTH_DP_STAT_POINTER));
    if (!GrowthLimitGains(memory, cpu, wram, &boundary))
        return boundary;
    GrowthApplyGains(memory, cpu, wram);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x81f87du);
}
