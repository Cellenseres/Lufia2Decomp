#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    TRANSITION_ENABLED = WRAM_BATTLE_EFFECT_SPECIAL_PARTY & 0xffffu,
    TRANSITION_PHASE = (WRAM_NMI_SCROLL_REGISTERS + 4u) & 0xffffu,
    TRANSITION_MASK = WRAM_BATTLE_TRANSITION_MASK,
    HDMA_CONTROL = WRAM_BATTLE_TRANSITION_HDMA_CONTROL & 0xffffu,
    HDMA_REGISTER = WRAM_BATTLE_TRANSITION_HDMA_REGISTER & 0xffffu,
    HDMA_SOURCE = WRAM_BATTLE_TRANSITION_HDMA_SOURCE & 0xffffu,
    HDMA_SOURCE_BANK = WRAM_BATTLE_TRANSITION_HDMA_SOURCE_BANK & 0xffffu,
    HDMA_INDIRECT_BANK = 0x004317u,
    HDMA_ENABLE = 0xdau,
    MASK_INITIAL = 0x1fu,
    MASK_CHANGED = 0x1du,
    RANDOM_MASK_SLOTS = 0x80u
};

typedef struct TransitionPhase {
    uint32_t change_site;
    uint32_t restore_site;
    uint32_t frames[4];
} TransitionPhase;

static const TransitionPhase transition_phases[] = {
    {0x85e93bu, 0x85e95fu,
        {0x85e98bu, 0x85e98eu, 0x85e99eu, 0x85e9a1u}},
    {0x85e9b6u, 0x85e9dau,
        {0x85ea06u, 0x85ea09u, 0x85ea19u, 0x85ea1cu}}
};

static Lufia2ExecutionResult TransitionChildUnwound(uint32_t site) {
    const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
    return result;
}

Lufia2ExecutionResult Lufia2BattlePrepareTransitionMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffbu)
        return ExecutionHandoff(cpu, 0x85abe4u);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x0100u);
    OpLoadA(cpu, 0x1f1fu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, TRANSITION_MASK));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x41u);
    OpSta(memory, cpu, OpAbs(cpu, HDMA_CONTROL));
    OpLoadA(cpu, 0x2cu);
    OpSta(memory, cpu, OpAbs(cpu, HDMA_REGISTER));
    OpLdx(cpu, 0xa10du);
    OpWriteX(memory, cpu, OpAbs(cpu, HDMA_SOURCE), cpu->x);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpAbs(cpu, HDMA_SOURCE_BANK));
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, HDMA_INDIRECT_BANK);
    OpLoadA(cpu, 2u);
    OpTestBits(memory, cpu, OpDp(cpu, HDMA_ENABLE), 1u);
    return ExecutionReturned(0x85ac15u);
}

Lufia2ExecutionResult Lufia2BattleAdvanceTransitionFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x85eae4u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85eaeau, 0x85ec81u, 3u, 0x85u))
        return TransitionChildUnwound(0x85eaeau);
    return ExecutionReturned(0x85eaeeu);
}

static void ToggleTransitionPhase(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, TRANSITION_PHASE));
    OpLoadA(cpu, OpA(cpu) ^ 0xffffu);
    OpSta(memory, cpu, OpAbs(cpu, TRANSITION_PHASE));
    OpSepWidths(cpu, 0x20u);
}

static uint32_t AdvanceTransitionFrames(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    const uint32_t *sites, unsigned count) {
    for (unsigned frame = 0u; frame < count; ++frame) {
        if (!CallChildWithFrame(memory, cpu, child, context,
                sites[frame], 0x85eae4u, 2u, 0x85u))
            return sites[frame];
    }
    return 0u;
}

static uint32_t ChangeRandomMaskSlot(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint8_t before, uint8_t after) {
    do {
        OpLoadA(cpu, RANDOM_MASK_SLOTS);
        if (!CallChildWithFrame(memory, cpu, child, context,
                site, 0x808299u, 3u, 0x85u))
            return site;
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xffu);
        OpAslA(cpu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, TRANSITION_MASK));
        OpCmpValue(cpu, before);
    } while (!cpu->zero);
    OpLoadA(cpu, after);
    OpSta(memory, cpu, OpLongX(cpu, TRANSITION_MASK));
    return 0u;
}

static uint32_t ChangeMaskSlots(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint8_t count, uint8_t before, uint8_t after) {
    OpLoadA(cpu, count);
    do {
        PushAccumulator8(memory, cpu);
        uint32_t failed = ChangeRandomMaskSlot(memory, cpu, child,
            context, site, before, after);
        if (failed)
            return failed;
        LoadA8(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    return 0u;
}

static uint32_t AnimateTransitionPhase(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    const TransitionPhase *phase, uint8_t rounds) {
    OpLoadA(cpu, rounds);
    do {
        PushAccumulator8(memory, cpu);
        uint32_t failed = ChangeMaskSlots(memory, cpu, child, context,
            phase->change_site, 3u, MASK_INITIAL, MASK_CHANGED);
        if (failed)
            return failed;
        failed = ChangeMaskSlots(memory, cpu, child, context,
            phase->restore_site, 2u, MASK_CHANGED, MASK_INITIAL);
        if (failed)
            return failed;
        ToggleTransitionPhase(memory, cpu);
        failed = AdvanceTransitionFrames(memory, cpu, child, context,
            phase->frames, 2u);
        if (failed)
            return failed;
        ToggleTransitionPhase(memory, cpu);
        failed = AdvanceTransitionFrames(memory, cpu, child, context,
            phase->frames + 2u, 2u);
        if (failed)
            return failed;
        LoadA8(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    return 0u;
}

Lufia2ExecutionResult Lufia2BattlePlayTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f12u || cpu->stack > 0x1ffbu || !child)
        return ExecutionHandoff(cpu, 0x85e7bcu);
    OpLda(memory, cpu, OpAbs(cpu, TRANSITION_ENABLED));
    if (cpu->zero)
        return ExecutionReturned(0x85e7c4u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85e91fu, 0x859671u, 3u, 0x85u))
        return TransitionChildUnwound(0x85e91fu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85e923u, 0x85abe4u, 3u, 0x85u))
        return TransitionChildUnwound(0x85e923u);
    OpLoadA(cpu, 0x0du);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85e929u, 0x80953bu, 3u, 0x85u))
        return TransitionChildUnwound(0x85e929u);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpAbs(cpu, TRANSITION_PHASE), cpu->x);
    uint32_t failed = AnimateTransitionPhase(memory, cpu, child, context,
        &transition_phases[0], 0x72u);
    if (failed)
        return TransitionChildUnwound(failed);
    OpLoadA(cpu, 0x6bu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85e9aau, 0x80953bu, 3u, 0x85u))
        return TransitionChildUnwound(0x85e9aau);
    failed = AnimateTransitionPhase(memory, cpu, child, context,
        &transition_phases[1], 0x0cu);
    if (failed)
        return TransitionChildUnwound(failed);
    const uint32_t final_random_sites[] = {0x85ea25u, 0x85ea42u, 0x85ea5fu};
    for (unsigned slot = 0u; slot < 3u; ++slot) {
        failed = ChangeRandomMaskSlot(memory, cpu, child, context,
            final_random_sites[slot], slot == 2u ? MASK_CHANGED : MASK_INITIAL,
            slot == 2u ? MASK_INITIAL : MASK_CHANGED);
        if (failed)
            return TransitionChildUnwound(failed);
    }
    const uint32_t final_frames[] =
        {0x85ea87u, 0x85ea8au, 0x85ea9au, 0x85ea9du};
    ToggleTransitionPhase(memory, cpu);
    failed = AdvanceTransitionFrames(memory, cpu, child, context, final_frames, 2u);
    if (failed)
        return TransitionChildUnwound(failed);
    ToggleTransitionPhase(memory, cpu);
    failed = AdvanceTransitionFrames(memory, cpu, child, context, final_frames + 2u, 2u);
    if (failed)
        return TransitionChildUnwound(failed);
    failed = ChangeRandomMaskSlot(memory, cpu, child, context,
        0x85eaa2u, MASK_INITIAL, MASK_CHANGED);
    if (failed)
        return TransitionChildUnwound(failed);
    const uint32_t closing_frames[] =
        {0x85eacau, 0x85eacdu, 0x85eaddu, 0x85eae0u};
    ToggleTransitionPhase(memory, cpu);
    failed = AdvanceTransitionFrames(memory, cpu, child, context, closing_frames, 2u);
    if (failed)
        return TransitionChildUnwound(failed);
    ToggleTransitionPhase(memory, cpu);
    failed = AdvanceTransitionFrames(memory, cpu, child, context, closing_frames + 2u, 2u);
    if (failed)
        return TransitionChildUnwound(failed);
    return ExecutionReturned(0x85eae3u);
}
