#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "system/wram.h"

enum {
    ACTOR_SLOT = 0xa7u,
    ACTOR_WORD = 0xa9u,
    ACTOR_RECORD = 0xabu,
    ACTOR_POSITION_CELL = 0x54u,
    ACTOR_DESCRIPTOR = 0x5du,
    ACTOR_DESCRIPTOR_BANK = 0x5fu,
    ACTOR_DESCRIPTOR_TABLE = 0xcff000u,
    ACTOR_ANIMATION_TABLE = 0x83abfcu,
    ACTOR_MOTION_TABLE = 0x83ac14u
};

static Lufia2ExecutionResult ActorBootstrapUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2ActorResetTransientState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83a6dfu);
    LoadA16(cpu, cpu->direct_page);
    const uint32_t arrays[] = {
        WRAM_UNK_7FE57E, WRAM_ACTOR_CLAIMED_OBJECT_RECORD,
        WRAM_ACTOR_CLAIMED_PENDING_OBJECT, WRAM_UNK_7FE4B6,
        WRAM_ACTOR_WALK_COUNTER, WRAM_UNK_7FE466,
        WRAM_ACTOR_SECONDARY_SCRIPT, WRAM_ACTOR_PRIMARY_TIMER
    };
    for (unsigned index = 0u; index < sizeof(arrays) / sizeof(arrays[0]); ++index)
        OpSta(memory, cpu, OpLongX(cpu, arrays[index]));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE4DE));
    SetAccumulatorWidth(cpu, 0u);
    PushIndex(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_WORD)));
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_FINE_X));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_FINE_Y));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X));
    OpPullX(memory, cpu);
    SetAccumulatorWidth(cpu, 1u);
    return ExecutionReturned(0x83a71bu);
}

Lufia2ExecutionResult Lufia2ActorSetFinePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83a71cu);
    SetAccumulatorWidth(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_WORD)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_FINE_X));
    for (unsigned bit = 0u; bit < 4u; ++bit)
        OpLsrA(cpu);
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, ACTOR_POSITION_CELL));
    OpTya(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_FINE_Y));
    for (unsigned bit = 0u; bit < 4u; ++bit)
        OpLsrA(cpu);
    OpAdcValue(cpu, 0u);
    SetAccumulatorWidth(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    OpLda(memory, cpu, OpDp(cpu, ACTOR_POSITION_CELL));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    return ExecutionReturned(0x83a745u);
}

Lufia2ExecutionResult Lufia2ActorHasSpecialSceneSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83a97eu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
    OpCmpValue(cpu, 0xfdu);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
        const uint8_t sprites[] = {0x71u, 0x72u, 0x73u};
        for (unsigned index = 0u; index < 3u; ++index) {
            OpCmpValue(cpu, sprites[index]);
            if (cpu->zero) {
                cpu->carry = 1u;
                return ExecutionReturned(0x83a995u);
            }
        }
    }
    cpu->carry = 0u;
    return ExecutionReturned(0x83a997u);
}

Lufia2ExecutionResult Lufia2ObjectResetSceneSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83a998u);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 1u);
    OpLdx(cpu, WRAM_OBJECT_STATE_COUNT - 1u);
    do {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_ID));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FRAME));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_DRAW_FLAGS));
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_OBJECT_STATE));
        OpDex(cpu);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83a9b9u);
}

Lufia2ExecutionResult Lufia2ActorReadSpriteDescriptor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83a9e5u);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0u);
    SetIndexWidth(cpu, 0u);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ACTOR_DESCRIPTOR_TABLE));
    OpSta(memory, cpu, OpDp(cpu, ACTOR_DESCRIPTOR));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 1u);
    OpLoadA(cpu, 0xcfu);
    OpSta(memory, cpu, OpDp(cpu, ACTOR_DESCRIPTOR_BANK));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, ACTOR_DESCRIPTOR));
    OpAndValue(cpu, 7u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, ACTOR_DESCRIPTOR));
    OpAndValue(cpu, 0xf8u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1291));
    OpLdy(cpu, 1u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, ACTOR_DESCRIPTOR));
    OpAslA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_SPRITE_DESCRIPTOR_STEP));
    OpIny(cpu);
    SetAccumulatorWidth(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_RECORD)));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, ACTOR_DESCRIPTOR));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_SPRITE_DESCRIPTOR_SOURCE));
    OpIny(cpu);
    OpIny(cpu);
    SetAccumulatorWidth(cpu, 1u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, ACTOR_DESCRIPTOR));
    OpSta(memory, cpu, OpAbsX(cpu, (WRAM_ACTOR_SPRITE_DESCRIPTOR_SOURCE + 2u)));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x83aa2fu);
}

Lufia2ExecutionResult Lufia2ActorSelectSpriteTables(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83aa7du);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpAslA(cpu);
    OpTax(cpu);
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, OpLongX(cpu, ACTOR_ANIMATION_TABLE));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_ACTOR_SPRITE_ANIMATION_TABLE));
    SetAccumulatorWidth(cpu, 1u);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E1471));
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E1291));
    OpAndValue(cpu, 0x18u);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpTax(cpu);
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, OpLongX(cpu, ACTOR_MOTION_TABLE));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_WORD)));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_ACTOR_SPRITE_MOTION_TABLE));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83aaaeu);
}

Lufia2ExecutionResult Lufia2ActorSetSpriteHeightOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83aa30u);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    LoadA16(cpu, cpu->direct_page);
    OpLoadA(cpu, 0xf0u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpBitValue(cpu, 1u);
    if (cpu->zero)
        LoadA16(cpu, cpu->direct_page);
    ExchangeAccumulatorBytes(cpu);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x83aa45u, 0x83fafau, 2u, 0x83u))
        return ActorBootstrapUnwound(0x83aa45u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_WORD)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_Y));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83aa4fu);
}

Lufia2ExecutionResult Lufia2ActorRefreshSpriteDescriptor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83a9d0u);
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 1u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x83a9d7u, 0x83a9e5u, 3u, 0x83u))
        return ActorBootstrapUnwound(0x83a9d7u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x83a9dbu, 0x83aa7du, 3u, 0x83u))
        return ActorBootstrapUnwound(0x83a9dbu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x83a9e4u);
}
