#include "lufia2/field.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "field/field_internal.h"


static void ResetAnimationSlots(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 7u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_ANIMATION_SLOT_OBJECT));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_ANIMATION_SLOT_STATE));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, 7u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD18C));
        OpDex(cpu);
    } while (!cpu->negative);
}

Lufia2ExecutionResult Lufia2FieldResetPresentationState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_CAMERA_ACTOR));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E0735));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, FIELD_SCREEN_OFFSET_X);
    OpSta(memory, cpu, FIELD_SCREEN_OFFSET_X + 1u);
    OpSta(memory, cpu, FIELD_SCREEN_OFFSET_Y);
    OpSta(memory, cpu, FIELD_SCREEN_OFFSET_Y + 1u);
    ResetAnimationSlots(memory, cpu);
    OpSta(memory, cpu, WRAM_UNK_7E09B3);
    OpSta(memory, cpu, WRAM_UNK_7FD0AE);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TEXT_WAIT_ACTOR));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A6));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09AD));
    OpStz(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_WAIT_COUNTER + 1u)));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_MAP_STATE));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET + 1u));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_SCENE_TEXT_CONTROL));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_REQUESTS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_ANIMATION_MASK));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FIELD_ANIMATION_NEXT_MASK));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8ebae6u);
}
