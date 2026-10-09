#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "system/wram.h"
#include "lufia2/field.h"
#include "lufia2/menu.h"

enum {
    DP_SCENE_FINISHED = 0x6au,
    DP_TITLE_STATE = 0x30u,
    TITLE_SAVE_STATE = 5u,
    DP_CHARACTER_RECORD = 0x2au,
    CHARACTER_FLAGS = 0x0fu,
    CHARACTER_CURRENT_HP = 0x11u,
    CHARACTER_MAXIMUM_HP = 0x25u,
    CHARACTER_SUMMARY_CURSOR = 0x1560u,
    CHARACTER_SUMMARY_TEXT = 0xd439u,
    DP_TEXT_CHARACTER = 0xe7u,
    DP_TEXT_SOURCE_BANK = 0x5fu
};

static Lufia2ExecutionResult MenuDispatcherUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2FieldRunSaveMenu(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x838386u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x838386u, 0x83afeau, 3u, cpu->program_bank))
        return MenuDispatcherUnwound(0x838386u);
    OpStz(memory, cpu, OpDp(cpu, DP_SCENE_FINISHED));
    OpLoadA(cpu, TITLE_SAVE_STATE);
    OpSta(memory, cpu, OpDp(cpu, DP_TITLE_STATE));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x838390u, 0x82e746u, 3u, cpu->program_bank))
        return MenuDispatcherUnwound(0x838390u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x838397u, 0x8385dcu, 3u, cpu->program_bank))
        return MenuDispatcherUnwound(0x838397u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83839bu, 0x83afcdu, 3u, cpu->program_bank))
        return MenuDispatcherUnwound(0x83839bu);
    return ExecutionReturned(0x83839fu);
}

static void CharacterSummaryAttribute(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_CHARACTER_SUMMARY_ATTRIBUTE));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, CHARACTER_FLAGS));
    OpBitValue(cpu, 5u);
    unsigned critical = !cpu->zero;
    if (!critical) {
        OpLda(memory, cpu, OpAbsY(cpu, CHARACTER_MAXIMUM_HP));
        for (unsigned shift = 0u; shift < 3u; ++shift) OpLsrA(cpu);
        OpCmp(memory, cpu, OpAbsY(cpu, CHARACTER_CURRENT_HP));
        critical = cpu->carry;
        if (!critical) {
            OpLda(memory, cpu, OpAbsY(cpu, CHARACTER_MAXIMUM_HP));
            for (unsigned shift = 0u; shift < 2u; ++shift) OpLsrA(cpu);
            OpCmp(memory, cpu, OpAbsY(cpu, CHARACTER_CURRENT_HP));
            if (cpu->carry) {
                OpSepWidths(cpu, 0x20u);
                OpLoadA(cpu, 3u);
                OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_CHARACTER_SUMMARY_ATTRIBUTE));
            }
        }
    }
    if (critical) {
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 5u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_CHARACTER_SUMMARY_ATTRIBUTE));
    }
}

Lufia2ExecutionResult Lufia2MenuDrawCharacterSummary(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8294c0u);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_CHARACTER_RECORD)));
    CharacterSummaryAttribute(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLdy(cpu, CHARACTER_SUMMARY_CURSOR);
    OpWriteX(memory, cpu, OpDp(cpu, DP_TEXT_CHARACTER), cpu->y);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_TEXT_SOURCE_BANK));
    OpLdy(cpu, CHARACTER_SUMMARY_TEXT);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x829509u, 0x808878u, 3u, cpu->program_bank))
        return MenuDispatcherUnwound(0x829509u);
    return ExecutionReturned(0x82950du);
}
