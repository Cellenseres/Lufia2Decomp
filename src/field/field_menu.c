#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    MENU_FIELD_SERVICE_BANK = 0x6au,
    MENU_FIELD_SELECTED_X = 0x8fu,
    MENU_FIELD_SELECTED_Y = 0x91u,
    MENU_FIELD_SELECTED_SCRIPT = 0x97u,
    MENU_FIELD_TEXT_FIRST = WRAM_UNK_7E0562,
    MENU_FIELD_TEXT_SECOND = WRAM_UNK_7E0563,
    MENU_FIELD_MAP = WRAM_FIELD_MAP_ID,
    MENU_FIELD_DESTINATION = WRAM_FIELD_DESTINATION_MAP,
    MENU_FIELD_TRANSITION = WRAM_FIELD_RELOAD_FLAGS,
    MENU_FIELD_STATE = WRAM_FIELD_TRANSITION_SOURCE_MAP,
    MENU_FIELD_FLAGS = WRAM_FIELD_FLAGS,
    MENU_FIELD_SELECTED_DESTINATION = WRAM_FIELD_UNK_05C0,
    MENU_FIELD_ACTOR_X = WRAM_ACTOR_TILE_X,
    MENU_FIELD_ACTOR_Y = WRAM_ACTOR_TILE_Y,
    MENU_FIELD_RESULT = WRAM_UNK_7E09C9,
    MENU_FIELD_RETURN_MAP = WRAM_MENU_RETURN_MAP
};

static uint8_t FieldMenuCall(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, 3u, (uint8_t)(site >> 16));
}

static Lufia2ExecutionResult FieldMenuUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void FieldMenuSetTransition(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t transition) {
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpAbs(cpu, MENU_FIELD_FLAGS), 1u);
    OpLoadA(cpu, transition);
    OpTestBits(memory, cpu, OpAbs(cpu, MENU_FIELD_TRANSITION), 1u);
}

static uint32_t FieldMenuReloadEvent(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0x22u);
    if (!FieldMenuCall(memory, cpu, child, context, 0x8eb026u, 0x80e722u))
        return 0x8eb026u;
    if (!FieldMenuCall(memory, cpu, child, context, 0x8eb02au, 0x80cbaeu))
        return 0x8eb02au;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return 0u;
}

Lufia2ExecutionResult Lufia2FieldRunMenu(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x8eb000u);
    if (!FieldMenuCall(memory, cpu, child, context, 0x8eb000u, 0x83afeau))
        return FieldMenuUnwound(0x8eb000u);
    OpStz(memory, cpu, OpDp(cpu, MENU_FIELD_SERVICE_BANK));
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, OpAbs(cpu, MENU_FIELD_TEXT_FIRST));
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, MENU_FIELD_TEXT_SECOND));
    if (!FieldMenuCall(memory, cpu, child, context, 0x8eb011u, 0x829a4eu))
        return FieldMenuUnwound(0x8eb011u);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, MENU_FIELD_TRANSITION));
    if (!FieldMenuCall(memory, cpu, child, context, 0x8eb019u, 0x8385dcu))
        return FieldMenuUnwound(0x8eb019u);
    const uint32_t event_site = FieldMenuReloadEvent(memory, cpu, child, context);
    if (event_site)
        return FieldMenuUnwound(event_site);
    if (!FieldMenuCall(memory, cpu, child, context, 0x8eb02fu, 0x83afcdu))
        return FieldMenuUnwound(0x8eb02fu);
    OpLda(memory, cpu, OpAbs(cpu, MENU_FIELD_RESULT));
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        OpLoadA(cpu, 0x80u);
        OpTestBits(memory, cpu, OpAbs(cpu, MENU_FIELD_FLAGS), 1u);
        OpLoadA(cpu, 8u);
        OpTestBits(memory, cpu, OpAbs(cpu, MENU_FIELD_TRANSITION), 1u);
    }
    OpCmpValue(cpu, 1u);
    if (!cpu->zero) {
        OpCmpValue(cpu, 2u);
        if (!cpu->zero)
            return ExecutionReturned(0x8eb04eu);
        OpLda(memory, cpu, OpAbs(cpu, MENU_FIELD_SELECTED_DESTINATION));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, MENU_FIELD_SELECTED_DESTINATION));
            OpSta(memory, cpu, OpAbs(cpu, MENU_FIELD_DESTINATION));
            OpStz(memory, cpu, OpAbs(cpu, MENU_FIELD_DESTINATION + 1u));
            FieldMenuSetTransition(memory, cpu, 0x40u);
            OpLda(memory, cpu, OpAbs(cpu, MENU_FIELD_MAP));
            OpCmpValue(cpu, 0xf1u);
            if (!cpu->zero)
                OpCmpValue(cpu, 0xf0u);
            if (cpu->zero) {
                TransferDirectToA(cpu);
                OpSta(memory, cpu, OpAbs(cpu, MENU_FIELD_STATE));
                OpSta(memory, cpu, WRAM_UNK_7FE75A);
            }
            if (!FieldMenuCall(memory, cpu, child, context,
                    0x8eb07cu, 0x83b82fu))
                return FieldMenuUnwound(0x8eb07cu);
            return ExecutionReturned(0x8eb080u);
        }
        OpLda(memory, cpu, OpAbs(cpu, MENU_FIELD_MAP));
        OpSta(memory, cpu, OpAbs(cpu, MENU_FIELD_RETURN_MAP));
    }
    OpStz(memory, cpu, OpAbs(cpu, MENU_FIELD_DESTINATION));
    OpStz(memory, cpu, OpAbs(cpu, MENU_FIELD_DESTINATION + 1u));
    FieldMenuSetTransition(memory, cpu, 0x80u);
    if (!FieldMenuCall(memory, cpu, child, context, 0x8eb097u, 0x83b82fu))
        return FieldMenuUnwound(0x8eb097u);
    return ExecutionReturned(0x8eb09bu);
}

Lufia2ExecutionResult Lufia2FieldRefreshMenuSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b82fu);
    OpLda(memory, cpu, OpAbs(cpu, MENU_FIELD_FLAGS));
    OpBitValue(cpu, 8u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, MENU_FIELD_ACTOR_X));
        OpSta(memory, cpu, OpDp(cpu, MENU_FIELD_SELECTED_X));
        OpLda(memory, cpu, OpAbs(cpu, MENU_FIELD_ACTOR_Y));
        OpSta(memory, cpu, OpDp(cpu, MENU_FIELD_SELECTED_Y));
        OpLoadA(cpu, 0x4eu);
        OpSta(memory, cpu, OpDp(cpu, MENU_FIELD_SELECTED_SCRIPT));
        OpLdy(cpu, 0u);
        if (!FieldMenuCall(memory, cpu, child, context, 0x83b847u, 0x80b404u))
            return FieldMenuUnwound(0x83b847u);
        OpLoadA(cpu, 8u);
        OpTestBits(memory, cpu, OpAbs(cpu, MENU_FIELD_FLAGS), 0u);
    }
    return ExecutionReturned(0x83b850u);
}
