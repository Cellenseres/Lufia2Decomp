#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"
#include "system/wram.h"
#include <stddef.h>

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

enum {
    DISSOLVE_ROWS_LEFT = 0x00u,
    DISSOLVE_SOURCE_TILE = 0x02u,
    DISSOLVE_MASK_TILE = 0x04u,
    DISSOLVE_COLUMNS = 0x11u,
    DISSOLVE_HEIGHT = 0x12u,
    DISSOLVE_ROW_STRIDE = 0x13u,
    DISSOLVE_ROW_COUNT = 0x15u,
    DISSOLVE_COUNTS = 0x19u,
    DISSOLVE_EMPTY_PREFIX = 0x1cu,
    DISSOLVE_PIXELS = 0x1fu,
    DISSOLVE_PICK = 0x22u,
    DISSOLVE_ROW_BITS = 0x23u,
    DISSOLVE_COLUMN_OFFSET = 0x24u,
    DISSOLVE_PIXEL_OFFSET = 0x28u,
    DISSOLVE_ROWS_PER_TILE = 0x26u,
    DISSOLVE_FIRST_ROW_WEIGHT = 0x27u,
    DISSOLVE_COUNT_POINTER = 0x2au,
    DISSOLVE_MASK_POINTER = 0x2du,
    DISSOLVE_SOURCE_GRAPHICS = 0x6000u,
    DISSOLVE_MASK_GRAPHICS = 0xdf00u,
    DISSOLVE_ROW_WEIGHTS = 0xa000u
};

static void BeginDissolveTile(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpWriteX(memory, cpu, OpDp(cpu, DISSOLVE_SOURCE_TILE), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, DISSOLVE_MASK_TILE), cpu->y);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_ROWS_LEFT));
}

static void FoldDissolveRow(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, DISSOLVE_SOURCE_GRAPHICS));
    OpOra(memory, cpu, OpAbsX(cpu, DISSOLVE_SOURCE_GRAPHICS + 1u));
    OpOra(memory, cpu, OpAbsX(cpu, DISSOLVE_SOURCE_GRAPHICS + 16u));
    OpOra(memory, cpu, OpAbsX(cpu, DISSOLVE_SOURCE_GRAPHICS + 17u));
    OpSta(memory, cpu, OpAbsY(cpu, DISSOLVE_MASK_GRAPHICS));
    OpSta(memory, cpu, OpAbsY(cpu, DISSOLVE_MASK_GRAPHICS + 1u));
    OpIny(cpu);
    OpIny(cpu);
    OpInx(cpu);
    OpInx(cpu);
    OpStepMem(memory, cpu, OpDp(cpu, DISSOLVE_ROWS_LEFT), -1);
}

static void AdvanceDissolveTile(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_MASK_TILE));
    cpu->carry = 1u;
    OpSbcValue(cpu, 16u);
    OpTay(cpu);
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_SOURCE_TILE));
    cpu->carry = 1u;
    OpSbcValue(cpu, 32u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2BattleContinueDissolveMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t pc,
    Lufia2BattleMaskPixelBudget budget, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        (pc != 0x85e7d0u && pc != 0x85e7d8u && pc != 0x85e7f2u))
        return ExecutionHandoff(cpu, pc);
    for (;;) {
        if (budget && !budget(context, cpu, pc))
            return ExecutionHandoff(cpu, pc);
        if (pc == 0x85e7d0u) {
            BeginDissolveTile(memory, cpu);
            pc = 0x85e7d8u;
        } else if (pc == 0x85e7d8u) {
            FoldDissolveRow(memory, cpu);
            if (cpu->zero)
                pc = 0x85e7f2u;
        } else {
            AdvanceDissolveTile(memory, cpu);
            if (!cpu->carry)
                return ExecutionHandoff(cpu, 0x85e806u);
            pc = 0x85e7d0u;
        }
    }
}

static void SetDissolveDimensions(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLdx(cpu, DISSOLVE_MASK_GRAPHICS);
    OpWriteX(memory, cpu, OpDp(cpu, DISSOLVE_PIXELS), cpu->x);
    OpLdx(cpu, DISSOLVE_ROW_WEIGHTS);
    OpWriteX(memory, cpu, OpDp(cpu, DISSOLVE_COUNTS), cpu->x);
    OpLoadA(cpu, 16u);
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_COLUMNS));
    OpLoadA(cpu, 15u);
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_HEIGHT));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_COLUMNS));
    OpSta(memory, cpu, SNES_WRMPYA);
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_COLUMNS));
    /* Shift the packed dimensions, including height. */
    for (unsigned shift = 0u; shift < 5u; ++shift)
        OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_ROW_STRIDE));
    OpLda(memory, cpu, SNES_RDMPYL);
    for (unsigned shift = 0u; shift < 3u; ++shift)
        OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_ROW_COUNT));
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2BattleRunDissolveEffectWithBudget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2BattleMaskPixelBudget budget, void *budget_context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffbu || !child)
        return ExecutionHandoff(cpu, 0x85e7c5u);
    if (budget && !budget(budget_context, cpu, 0x85e7c5u))
        return ExecutionHandoff(cpu, 0x85e7c5u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLdx(cpu, 0x1fe0u);
    OpLdy(cpu, 0x0ff0u);
    const uint32_t caller_resume = cpu->resume_pc;
    const Lufia2ExecutionResult folded = Lufia2BattleContinueDissolveMask(
        memory, cpu, 0x85e7d0u, budget, budget_context);
    if (folded.pc != 0x85e806u)
        return folded;
    cpu->resume_pc = caller_resume;
    if (budget && !budget(budget_context, cpu, 0x85e806u))
        return ExecutionHandoff(cpu, 0x85e806u);
    SetDissolveDimensions(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85e835u, 0x85e8e8u, 2u, 0x85u))
        return TransitionChildUnwound(0x85e835u);
    if (budget && !budget(budget_context, cpu, 0x85e838u))
        return ExecutionHandoff(cpu, 0x85e838u);
    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpDp(cpu, DISSOLVE_EMPTY_PREFIX), cpu->x);
    for (;;) {
        if (budget && !budget(budget_context, cpu, 0x85e83du))
            return ExecutionHandoff(cpu, 0x85e83du);
        OpLoadA(cpu, 0x80u);
        do {
            if (budget && !budget(budget_context, cpu, 0x85e83fu))
                return ExecutionHandoff(cpu, 0x85e83fu);
            PushAccumulator8(memory, cpu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x85e840u, 0x85e85fu, 2u, 0x85u))
                return TransitionChildUnwound(0x85e840u);
            LoadA8(cpu, Pull8(memory, cpu));
            if (cpu->carry) {
                PullDataBank(memory, cpu);
                return ExecutionReturned(0x85e85eu);
            }
            OpDecA(cpu);
        } while (!cpu->zero);
        if (budget && !budget(budget_context, cpu, 0x85e849u))
            return ExecutionHandoff(cpu, 0x85e849u);
        OpRepWidths(cpu, 0x20u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x85e84bu, 0x859c4du, 3u, 0x85u))
            return TransitionChildUnwound(0x85e84bu);
        if (budget && !budget(budget_context, cpu, 0x85e84fu))
            return ExecutionHandoff(cpu, 0x85e84fu);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, WRAM_BATTLE_EFFECT_FRAME_MARKER);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x85e857u, 0x85ec81u, 3u, 0x85u))
            return TransitionChildUnwound(0x85e857u);
        if (budget && !budget(budget_context, cpu, 0x85e85bu))
            return ExecutionHandoff(cpu, 0x85e85bu);
    }
}

Lufia2ExecutionResult Lufia2BattleRunDissolveEffect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return Lufia2BattleRunDissolveEffectWithBudget(
        memory, cpu, child, context, NULL, NULL);
}

static uint32_t DissolveRowAddress(const Lufia2Memory *memory,
    const Lufia2CpuState *cpu, uint8_t pointer) {
    return AbsoluteIndexedAddress(cpu,
        Read16Direct(memory, cpu, pointer), cpu->y);
}

static void LocateDissolvePixel(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, cpu->data_bank);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WRDIVL), cpu->y);
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_COLUMNS));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVB));
    OpPushX(memory, cpu);
    OpPullX(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SNES_RDDIVL)));
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_COLUMN_OFFSET));
    OpTxa(cpu);
    OpAslA(cpu);
    OpAndValue(cpu, 0x000eu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DISSOLVE_COLUMN_OFFSET));
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_PIXEL_OFFSET));
    OpTxa(cpu);
    OpSepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xf8u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_COLUMNS));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_PIXEL_OFFSET));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleClearRandomMaskPixelWithBudget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2BattleMaskPixelBudget budget, void *budget_context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x85e85fu);

    for (;;) {
        if (budget && !budget(budget_context, cpu, 0x85e85fu))
            return ExecutionHandoff(cpu, 0x85e85fu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x85e85fu, 0x8082c7u, 3u, 0x85u))
            return TransitionChildUnwound(0x85e85fu);
        OpSta(memory, cpu, OpDp(cpu, DISSOLVE_PICK));
        OpLdy(cpu, Read16Direct(memory, cpu, DISSOLVE_EMPTY_PREFIX));
        LoadA16(cpu, cpu->direct_page);
        uint8_t selected = 0u;
        for (;;) {
            if (budget && !budget(budget_context, cpu, 0x85e868u))
                return ExecutionHandoff(cpu, 0x85e868u);
            OpIny(cpu);
            OpCpy(cpu, Read16Direct(memory, cpu, DISSOLVE_ROW_COUNT));
            if (cpu->zero)
                break;
            cpu->carry = 0u;
            OpAdc(memory, cpu, DissolveRowAddress(memory, cpu, DISSOLVE_COUNTS));
            if (cpu->zero)
                OpWriteX(memory, cpu, OpDp(cpu, DISSOLVE_EMPTY_PREFIX), cpu->y);
            OpCmp(memory, cpu, OpDp(cpu, DISSOLVE_PICK));
            if (cpu->carry && !cpu->zero) {
                selected = 1u;
                break;
            }
        }
        if (selected)
            break;
        if (budget && !budget(budget_context, cpu, 0x85e87cu))
            return ExecutionHandoff(cpu, 0x85e87cu);
        OpCmpValue(cpu, 0u);
        if (cpu->zero) {
            cpu->carry = 1u;
            return ExecutionReturned(0x85e881u);
        }
    }

    if (budget && !budget(budget_context, cpu, 0x85e882u))
        return ExecutionHandoff(cpu, 0x85e882u);
    cpu->carry = 1u;
    OpSbcValue(cpu, Read8(memory, OpDp(cpu, DISSOLVE_PICK)));
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_PICK));
    OpLda(memory, cpu, DissolveRowAddress(memory, cpu, DISSOLVE_COUNTS));
    OpDecA(cpu);
    OpSta(memory, cpu, DissolveRowAddress(memory, cpu, DISSOLVE_COUNTS));
    LocateDissolvePixel(memory, cpu);
    OpLda(memory, cpu, DissolveRowAddress(memory, cpu, DISSOLVE_PIXELS));
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_ROW_BITS));
    OpLoadA(cpu, 1u);
    for (;;) {
        OpBit(memory, cpu, OpDp(cpu, DISSOLVE_ROW_BITS));
        if (!cpu->zero) {
            OpStepMem(memory, cpu, OpDp(cpu, DISSOLVE_PICK), -1);
            if (cpu->zero)
                break;
        }
        OpAslA(cpu);
        if (cpu->carry)
            return ExecutionHandoff(cpu, 0x85e8dcu);
    }
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpAndValue(cpu, OpReadM(memory, cpu,
        DissolveRowAddress(memory, cpu, DISSOLVE_PIXELS)));
    OpSta(memory, cpu, DissolveRowAddress(memory, cpu, DISSOLVE_PIXELS));
    OpIny(cpu);
    OpSta(memory, cpu, DissolveRowAddress(memory, cpu, DISSOLVE_PIXELS));
    cpu->carry = 0u;
    return ExecutionReturned(0x85e8e7u);
}

Lufia2ExecutionResult Lufia2BattleClearRandomMaskPixel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return Lufia2BattleClearRandomMaskPixelWithBudget(
        memory, cpu, child, context, NULL, NULL);
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

static Lufia2ExecutionResult RunTransitionPhases(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
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
    return RunTransitionPhases(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2BattleRunDissolveTransitionWithBudget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2BattleMaskPixelBudget budget, void *budget_context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f14u || cpu->stack > 0x1ffbu || !child)
        return ExecutionHandoff(cpu, 0x85e8e8u);
    if (budget && !budget(budget_context, cpu, 0x85e8e8u))
        return ExecutionHandoff(cpu, 0x85e8e8u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_ROW_STRIDE));
    cpu->carry = 1u;
    OpSbcValue(cpu, 16u);
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_COLUMN_OFFSET));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_HEIGHT));
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_ROW_BITS));
    OpLdx(cpu, Read16Direct(memory, cpu, DISSOLVE_COUNTS));
    OpWriteX(memory, cpu, OpDp(cpu, DISSOLVE_COUNT_POINTER), cpu->x);
    OpLdx(cpu, Read16Direct(memory, cpu, DISSOLVE_PIXELS));
    OpWriteX(memory, cpu, OpDp(cpu, DISSOLVE_MASK_POINTER), cpu->x);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_ROWS_PER_TILE));
    LoadA16(cpu, cpu->direct_page);
    OpRepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu,
        Read16Direct(memory, cpu, DISSOLVE_COUNT_POINTER)));
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    PushY(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DISSOLVE_COLUMNS));
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_PICK));
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, AbsoluteIndexedAddress(cpu,
        Read16Direct(memory, cpu, DISSOLVE_MASK_POINTER), cpu->y));
    OpTax(cpu);
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 0u;
    OpLda(memory, cpu, OpLongX(cpu, 0x97b418u));
    OpSta(memory, cpu, OpDp(cpu, DISSOLVE_FIRST_ROW_WEIGHT));
    if (budget && !budget(budget_context, cpu, 0x85e91fu))
        return ExecutionHandoff(cpu, 0x85e91fu);
    return RunTransitionPhases(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2BattleRunDissolveTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return Lufia2BattleRunDissolveTransitionWithBudget(
        memory, cpu, child, context, NULL, NULL);
}
