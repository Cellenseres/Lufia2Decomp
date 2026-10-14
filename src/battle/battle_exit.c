/* Battle exit. */

#include "battle/battle_lifecycle_internal.h"
#include "core/child_call.h"

Lufia2ExecutionResult Lufia2BattleExit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    if (!BattlePrepareExit(&battle))
        return BattleChildUnwound(&battle);
    if (!BattleFadeOut(&battle))
        return BattleChildUnwound(&battle);
    if (!BattleTearDownDisplay(&battle))
        return BattleChildUnwound(&battle);

    return ExecutionReturned(0x8187b6u);
}

enum {
    ROM_SPECIAL_EXIT_MESSAGE = 0xf22au,
    DP_EXIT_BATTLER = 0x11u,
    DP_EXIT_PALETTE_LEVEL = 0x13u,
    DP_EXIT_FRAME_FLAGS = 0x73u,
    SPECIAL_EXIT_LOOP_LIMIT = 4096u
};

static uint8_t SpecialExitChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, 3u, 0x85u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->decimal || cpu->direct_page) {
        *result = ExecutionHandoff(cpu, site + 4u);
        return 0u;
    }
    return 1u;
}

static uint8_t FadeSpecialExitBattlers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, Lufia2ExecutionResult *result) {
    OpLoadA(cpu, 0x40u);
    for (unsigned level = 0u; level < SPECIAL_EXIT_LOOP_LIMIT; ++level) {
        OpSta(memory, cpu, OpDp(cpu, DP_EXIT_PALETTE_LEVEL));
        PushAccumulator8(memory, cpu);
        OpLoadA(cpu, 0x0fu);
        OpSta(memory, cpu, OpDp(cpu, DP_EXIT_BATTLER));
        unsigned battler;
        for (battler = 0u; battler < SPECIAL_EXIT_LOOP_LIMIT; ++battler) {
            if (!SpecialExitChild(memory, cpu, child, context,
                    0x85eb17u, 0x81b444u, result))
                return 0u;
            OpStepMem(memory, cpu, OpDp(cpu, DP_EXIT_BATTLER), -1);
            if (cpu->negative)
                break;
        }
        if (battler == SPECIAL_EXIT_LOOP_LIMIT) {
            *result = ExecutionHandoff(cpu, 0x85eb17u);
            return 0u;
        }
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, DP_EXIT_FRAME_FLAGS));
        if (!SpecialExitChild(memory, cpu, child, context,
                0x85eb23u, 0x85ec81u, result))
            return 0u;
        OpLoadA(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
        OpDecA(cpu);
        if (cpu->negative)
            return 1u;
    }
    *result = ExecutionHandoff(cpu, 0x85eb10u);
    return 0u;
}

static void CopySpecialExitPalette(const Lufia2Memory *memory,
                                   Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x1ffu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_CGRAM_BUFFER));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FF1DB));
        OpDex(cpu);
    } while (!cpu->negative);
}

static uint8_t HoldSpecialExit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, Lufia2ExecutionResult *result) {
    OpLoadA(cpu, 0x2du);
    for (unsigned frame = 0u; frame < SPECIAL_EXIT_LOOP_LIMIT; ++frame) {
        PushAccumulator8(memory, cpu);
        if (!SpecialExitChild(memory, cpu, child, context,
                0x85eb3cu, 0x85ec81u, result))
            return 0u;
        OpLoadA(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
        if (cpu->zero)
            return 1u;
    }
    *result = ExecutionHandoff(cpu, 0x85eb3bu);
    return 0u;
}

static uint8_t FinishSpecialExitFade(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, Lufia2ExecutionResult *result) {
    OpLoadA(cpu, 0x0eu);
    for (unsigned level = 0u; level < SPECIAL_EXIT_LOOP_LIMIT; ++level) {
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        PushAccumulator8(memory, cpu);
        static const uint32_t sites[] = {
            0x85eb4au, 0x85eb4eu, 0x85eb52u, 0x85eb56u};
        for (unsigned frame = 0u; frame < 4u; ++frame)
            if (!SpecialExitChild(memory, cpu, child, context,
                    sites[frame], 0x85ec81u, result))
                return 0u;
        OpLoadA(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
        if (cpu->negative)
            return 1u;
    }
    *result = ExecutionHandoff(cpu, 0x85eb46u);
    return 0u;
}

Lufia2ExecutionResult Lufia2BattleFadeSpecialExit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page || !child)
        return ExecutionHandoff(cpu, 0x85eaefu);
    Lufia2ExecutionResult result;
    OpLdx(cpu, ROM_SPECIAL_EXIT_MESSAGE);
    if (!SpecialExitChild(memory, cpu, child, context,
            0x85eaf2u, 0x8594e7u, &result))
        return result;
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_COUNT), cpu->x);
    OpDex(cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER), cpu->x);
    if (!SpecialExitChild(memory, cpu, child, context,
            0x85eb00u, 0x8595feu, &result))
        return result;
    OpLoadA(cpu, 0xd2u);
    if (!SpecialExitChild(memory, cpu, child, context,
            0x85eb06u, 0x8096ccu, &result) ||
        !SpecialExitChild(memory, cpu, child, context,
            0x85eb0au, 0x809692u, &result) ||
        !FadeSpecialExitBattlers(memory, cpu, child, context, &result))
        return result;
    CopySpecialExitPalette(memory, cpu);
    if (!HoldSpecialExit(memory, cpu, child, context, &result) ||
        !FinishSpecialExitFade(memory, cpu, child, context, &result))
        return result;
    return ExecutionReturned(0x85eb5eu);
}
