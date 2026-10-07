#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "system/wram.h"

enum {
    SCENE_ACTOR_SLOT = 0xa7u,
    SCENE_ACTOR_WORD = 0xa9u,
    SCENE_ACTOR_RECORD = 0xabu,
    SPRITE_SOURCE = 0x54u,
    SPRITE_SOURCE_WORD = 0x56u,
    SPRITE_UPLOAD_CURSOR = WRAM_SPRITE_UPLOAD_CURSOR,
    SPRITE_UPLOAD_ANIMATION = WRAM_SPRITE_UPLOAD_ANIMATION,
    SPRITE_UPLOAD_TARGET = WRAM_SPRITE_UPLOAD_VRAM,
    SPRITE_UPLOAD_BANK = WRAM_SPRITE_UPLOAD_BANK,
    SPRITE_UPLOAD_SOURCE = WRAM_SPRITE_UPLOAD_SOURCE,
    SPRITE_UPLOAD_VRAM = WRAM_ACTOR_SPRITE_VRAM_BASE,
    ACTOR_MOTION_OFFSET = WRAM_ACTOR_SCENE_MOTION_OFFSET,
    ACTOR_MOTION_TABLE = WRAM_ACTOR_SPRITE_MOTION_TABLE,
    ACTOR_DESCRIPTOR_SOURCE = WRAM_ACTOR_SPRITE_DESCRIPTOR_SOURCE,
    ACTOR_ANIMATION_TABLE = WRAM_ACTOR_SPRITE_ANIMATION_TABLE,
    OBJECT_SPRITE_OFFSET = WRAM_OBJECT_SPRITE_VRAM_OFFSET,
    OBJECT_SPRITE_SOURCE = WRAM_OBJECT_SPRITE_SOURCE,
    SPRITE_FRAME_STRIDES = 0x83abf8u,
    SPRITE_ANIMATION_TABLE = 0x83abfcu
};

static Lufia2ExecutionResult ActorSceneUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2ActorResetSceneSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83a686u);
    PushIndex(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 1u);
    OpLdx(cpu, 39u);
    do {
        Write8(memory, DirectAddress(cpu, SCENE_ACTOR_SLOT), (uint8_t)cpu->x);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x83a68eu, 0x83ab4fu, 3u, 0x83u))
            return ActorSceneUnwound(0x83a68eu);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1471));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E070A));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE316));
        LoadA16(cpu, cpu->direct_page);
        const uint32_t arrays[] = {WRAM_ACTOR_SCENE_MOTION_OFFSET, WRAM_UNK_7FE25E, WRAM_UNK_7FE2A6};
        for (unsigned field = 0u; field < 3u; ++field)
            OpSta(memory, cpu, OpLongX(cpu, arrays[field]));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FACING));
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x83a6c1u, 0x83a6dfu, 2u, 0x83u))
            return ActorSceneUnwound(0x83a6c1u);
        OpDex(cpu);
    } while (!cpu->negative);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_FIELD_CLAIMED_ACTOR_IDS);
    OpSta(memory, cpu, WRAM_FIELD_CLAIMED_ACTOR_IDS + 1u);
    OpLdx(cpu, 7u);
    OpLoadA(cpu, 0xffu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD0A6));
        OpDex(cpu);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    return ExecutionReturned(0x83a6deu);
}

Lufia2ExecutionResult Lufia2ActorQueueSceneSpriteUpload(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83aae5u);
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpLongX(cpu, ACTOR_MOTION_OFFSET));
    SetAccumulatorWidth(cpu, 0u);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_WORD)));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsY(cpu, ACTOR_MOTION_TABLE));
    OpSta(memory, cpu, OpDp(cpu, SPRITE_SOURCE));
    SetAccumulatorWidth(cpu, 1u);
    OpLoadA(cpu, 0x83u);
    OpSta(memory, cpu, OpDp(cpu, SPRITE_SOURCE_WORD));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, SPRITE_SOURCE));
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, SPRITE_FRAME_STRIDES));
    OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_RECORD)));
    OpLda(memory, cpu, OpAbsY(cpu, ACTOR_DESCRIPTOR_SOURCE + 2u));
    OpSta(memory, cpu, OpDp(cpu, SPRITE_SOURCE));
    OpLda(memory, cpu, OpAbs(cpu, 0x4216u));
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    SetAccumulatorWidth(cpu, 0u);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsY(cpu, ACTOR_DESCRIPTOR_SOURCE));
    OpSta(memory, cpu, OpDp(cpu, SPRITE_SOURCE_WORD));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, SPRITE_UPLOAD_CURSOR)));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_WORD)));
    OpLda(memory, cpu, OpAbsX(cpu, ACTOR_ANIMATION_TABLE));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_UPLOAD_ANIMATION));
    OpLda(memory, cpu, OpAbsX(cpu, SPRITE_UPLOAD_VRAM));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_UPLOAD_TARGET));
    OpLda(memory, cpu, OpDp(cpu, SPRITE_SOURCE));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_UPLOAD_BANK));
    OpLda(memory, cpu, OpDp(cpu, SPRITE_SOURCE_WORD));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_UPLOAD_SOURCE));
    OpIny(cpu);
    OpIny(cpu);
    Write8(memory, OpAbs(cpu, SPRITE_UPLOAD_CURSOR), (uint8_t)cpu->y);
    return ExecutionReturned(0x83ab4eu);
}

Lufia2ExecutionResult Lufia2ObjectQueueSceneSpriteUpload(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83fcd1u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, SPRITE_UPLOAD_CURSOR)));
    LoadA16(cpu, cpu->direct_page);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpAslA(cpu);
    OpTax(cpu);
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, OpLongX(cpu, SPRITE_ANIMATION_TABLE));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_UPLOAD_ANIMATION));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_WORD)));
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_SPRITE_OFFSET));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_UPLOAD_TARGET));
    SetAccumulatorWidth(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_RECORD)));
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_SPRITE_SOURCE + 2u));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_UPLOAD_BANK));
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_SPRITE_SOURCE));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_UPLOAD_SOURCE));
    SetAccumulatorWidth(cpu, 1u);
    OpIny(cpu);
    OpIny(cpu);
    if (cpu->index_is_8_bit)
        Write8(memory, OpAbs(cpu, SPRITE_UPLOAD_CURSOR), (uint8_t)cpu->y);
    else
        OpWrite16(memory, OpAbs(cpu, SPRITE_UPLOAD_CURSOR), cpu->y);
    return ExecutionReturned(0x83fd0au);
}
