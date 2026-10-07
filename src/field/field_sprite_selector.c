#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    SELECTED_SPRITE = 0x02u,
    HELD_BUTTONS = 0x48u,
    PRESSED_BUTTONS = 0x4cu,
    ACTOR_SLOT = 0xa7u,
    PREVIOUS_SPRITE_BUTTON = 0x20u,
    NEXT_SPRITE_BUTTON = 0x10u,
    SPRITE_SELECTION_LIMIT = 0xf9u,
    SPRITE_RELOAD_STATE = 0x20u,
    SPRITE_SCRIPT_VALUE = 3u
};

static uint8_t SpriteSelectionReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x8eu && !cpu->decimal;
}

static Lufia2ExecutionResult SpriteSelectionUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SpriteSelectionCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint16_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        0x8e0000u | site, target, frame, 0x8eu);
}

Lufia2ExecutionResult Lufia2FieldTakeSpriteSelectionButtons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!SpriteSelectionReady(cpu))
        return ExecutionHandoff(cpu, 0x8ebba8u);
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, HELD_BUTTONS)));
    if (!cpu->zero)
        OpTestBits(memory, cpu, OpDp(cpu, PRESSED_BUTTONS), 0u);
    return ExecutionReturned(0x8ebbaeu);
}

static uint32_t ReloadSelectedSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpStz(memory, cpu, OpDp(cpu, ACTOR_SLOT));
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb60u, 0x83ab4fu, 3u))
        return 0x8ebb60u;
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb64u, 0x83aaafu, 3u))
        return 0x8ebb64u;
    OpLda(memory, cpu, OpDp(cpu, SELECTED_SPRITE));
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb6au, 0x83a9bau, 3u))
        return 0x8ebb6au;
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb6eu, 0x83aa30u, 3u))
        return 0x8ebb6eu;
    Push8(memory, cpu, PackStatus(cpu));
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb73u, 0x83aae1u, 3u))
        return 0x8ebb73u;
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpLoadA(cpu, SPRITE_RELOAD_STATE);
    OpSta(memory, cpu, WRAM_ACTOR_STATE);
    OpLoadA(cpu, SPRITE_SCRIPT_VALUE);
    OpSta(memory, cpu, WRAM_UNK_7E070A);
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb84u, 0x83d416u, 3u))
        return 0x8ebb84u;
    OpSepWidths(cpu, 0x30u);
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb8au, 0x83a21au, 3u))
        return 0x8ebb8au;
    OpStz(memory, cpu, OpDp(cpu, ACTOR_SLOT));
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb90u, 0x83ab4fu, 3u))
        return 0x8ebb90u;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET));
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb97u, 0x80ed9cu, 3u))
        return 0x8ebb97u;
    return 0u;
}

static void RestoreSpriteSelectionRegisters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

Lufia2ExecutionResult Lufia2FieldCycleSelectedSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SpriteSelectionReady(cpu) || !child || cpu->stack < 12u ||
        cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x8ebb2eu);
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    LoadA8(cpu, PREVIOUS_SPRITE_BUTTON);
    if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb37u, 0x8ebba8u, 2u))
        return SpriteSelectionUnwound(0x8ebb37u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, SELECTED_SPRITE));
        OpDecA(cpu);
        OpSta(memory, cpu, OpDp(cpu, SELECTED_SPRITE));
        OpCmpValue(cpu, SPRITE_SELECTION_LIMIT);
        if (cpu->carry) {
            OpLoadA(cpu, SPRITE_SELECTION_LIMIT);
            OpSta(memory, cpu, OpDp(cpu, SELECTED_SPRITE));
        }
    } else {
        LoadA8(cpu, NEXT_SPRITE_BUTTON);
        if (!SpriteSelectionCall(memory, cpu, child, context, 0xbb4du, 0x8ebba8u, 2u))
            return SpriteSelectionUnwound(0x8ebb4du);
        if (cpu->zero) {
            RestoreSpriteSelectionRegisters(memory, cpu);
            return ExecutionReturned(0x8ebba0u);
        }
        OpLda(memory, cpu, OpDp(cpu, SELECTED_SPRITE));
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, SELECTED_SPRITE));
        OpDecA(cpu);
        OpCmpValue(cpu, SPRITE_SELECTION_LIMIT);
        if (cpu->carry)
            OpStz(memory, cpu, OpDp(cpu, SELECTED_SPRITE));
    }
    const uint32_t site = ReloadSelectedSprite(memory, cpu, child, context);
    if (site)
        return SpriteSelectionUnwound(site);
    RestoreSpriteSelectionRegisters(memory, cpu);
    return ExecutionReturned(0x8ebba0u);
}
