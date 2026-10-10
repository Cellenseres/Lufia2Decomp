#include "lufia2/field.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    PLAYER_DIRECTION_INPUT = 0x47,
    PLAYER_DIRECTION_LATCH = 0x4b,
    PLAYER_DIRECTION = 0x22,
    PLAYER_STEP_CODE = 0x23,
};

Lufia2ExecutionResult Lufia2FieldReadPlayerDirection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION_INPUT));
    OpAndValue(cpu, 0x0fu);
    cpu->carry = 0u;
    if (!cpu->zero) {
        TrbDirect8(memory, cpu, PLAYER_DIRECTION_LATCH);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x83d437u));
        OpSta(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
        OpSta(memory, cpu, WRAM_FIELD_SELECTED_DIRECTION);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x83c1b0u));
        OpSta(memory, cpu, OpDp(cpu, PLAYER_STEP_CODE));
        cpu->carry = 1u;
    }
    return ExecutionReturned(0x83c17eu);
}

Lufia2ExecutionResult Lufia2FieldClearSecondaryActionState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        OpAndValue(cpu, 0xfdu);
        OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
        TransferDirectToA(cpu);
        OpSta(memory, cpu, WRAM_ACTOR_SCENE_MOTION_OFFSET);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1471));
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, WRAM_ACTOR_SCENE_MOTION_OFFSET);
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, WRAM_UNK_7FE4DE);
    }
    return ExecutionReturned(0x83c1a4u);
}

Lufia2ExecutionResult Lufia2FieldReadSelectedItemByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpPushX(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpAndValue(cpu, 0x01ffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x96cf69u));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x96cf6au));
    OpPullX(memory, cpu);
    return ExecutionReturned(0x83c6a9u);
}
