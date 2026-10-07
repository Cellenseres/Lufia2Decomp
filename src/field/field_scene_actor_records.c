#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    SCENE_RECORD_STORAGE = 0x7ef000u,
    SCENE_POSITION_X = WRAM_UNK_7E120A,
    SCENE_POSITION_Y = WRAM_UNK_7E120B,
    SCENE_POSITION_AUXILIARY = WRAM_SCENE_ACTOR_AUXILIARY,
    SCENE_ACTOR_ATTRIBUTES = WRAM_UNK_7E120E,
    SCENE_ACTOR_PRESENTATION = WRAM_UNK_7E1212,
    SCENE_ACTOR_SLOT = 0xa7u
};

static Lufia2ExecutionResult SceneRecordUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static bool SceneRecordByteContext(const Lufia2CpuState *cpu, bool wide_index) {
    return cpu->program_bank == 0x80u && cpu->accumulator_is_8_bit &&
        !cpu->decimal && (!wide_index || !cpu->index_is_8_bit);
}

Lufia2ExecutionResult Lufia2SceneReadActorAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SceneRecordByteContext(cpu, true))
        return ExecutionHandoff(cpu, 0x80c01du);
    OpLdx(cpu, 0x10u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 8u);
    ExchangeAccumulatorBytes(cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x80c024u, 0x80bfaau, 3u, 0x80u))
        return SceneRecordUnwound(0x80c024u);
    if (cpu->carry) {
        OpStz(memory, cpu, OpAbs(cpu, SCENE_POSITION_X));
        OpStz(memory, cpu, OpAbs(cpu, SCENE_POSITION_Y));
        OpStz(memory, cpu, OpAbs(cpu, SCENE_POSITION_Y));
        OpStz(memory, cpu, OpAbs(cpu, SCENE_POSITION_AUXILIARY));
        cpu->carry = 1u;
    } else {
        SetAccumulatorWidth(cpu, 0u);
        OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_STORAGE + 1u));
        OpSta(memory, cpu, OpAbs(cpu, SCENE_POSITION_X));
        OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_STORAGE + 3u));
        OpSta(memory, cpu, OpAbs(cpu, SCENE_ACTOR_ATTRIBUTES));
        OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_STORAGE + 5u));
        OpSta(memory, cpu, OpAbs(cpu, SCENE_ACTOR_ATTRIBUTES + 2u));
        SetAccumulatorWidth(cpu, 1u);
        OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_STORAGE + 7u));
        OpSta(memory, cpu, SCENE_ACTOR_PRESENTATION);
        cpu->carry = 0u;
    }
    return ExecutionReturned(0x80c05bu);
}

Lufia2ExecutionResult Lufia2SceneReadActorCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SceneRecordByteContext(cpu, false))
        return ExecutionHandoff(cpu, 0x80c05cu);
    cpu->carry = 0u;
    Push8(memory, cpu, PackStatus(cpu));
    SetIndexWidth(cpu, 0u);
    OpLdx(cpu, 0x0eu);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 3u);
    ExchangeAccumulatorBytes(cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x80c067u, 0x80bfaau, 3u, 0x80u))
        return SceneRecordUnwound(0x80c067u);
    if (cpu->carry) {
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, SCENE_POSITION_X);
        OpSta(memory, cpu, SCENE_POSITION_Y);
        OpSta(memory, cpu, SCENE_POSITION_Y);
        OpSta(memory, cpu, SCENE_POSITION_AUXILIARY);
        UnpackStatus(cpu, Pull8(memory, cpu));
        cpu->carry = 1u;
        return ExecutionReturned(0x80c081u);
    }
    OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_STORAGE + 1u));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_POSITION_X));
    OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_STORAGE + 2u));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_POSITION_Y));
    cpu->carry = 0u;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80c092u);
}

Lufia2ExecutionResult Lufia2SceneReadActorPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SceneRecordByteContext(cpu, true))
        return ExecutionHandoff(cpu, 0x80c093u);
    OpLdx(cpu, 0x14u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 5u);
    ExchangeAccumulatorBytes(cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x80c09au, 0x80bfaau, 3u, 0x80u))
        return SceneRecordUnwound(0x80c09au);
    if (cpu->carry) {
        cpu->carry = 1u;
    } else {
        SetAccumulatorWidth(cpu, 0u);
        OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_STORAGE + 1u));
        OpSta(memory, cpu, OpAbs(cpu, SCENE_POSITION_X));
        OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_STORAGE + 3u));
        OpSta(memory, cpu, OpAbs(cpu, SCENE_POSITION_AUXILIARY));
        SetAccumulatorWidth(cpu, 1u);
        cpu->carry = 0u;
    }
    return ExecutionReturned(0x80c0b6u);
}

Lufia2ExecutionResult Lufia2SceneApplyActorCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SceneRecordByteContext(cpu, false))
        return ExecutionHandoff(cpu, 0x80c1a7u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
    OpStz(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, 0xfbu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SCENE_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_POSITION_X));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_POSITION_Y));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    return ExecutionReturned(0x80c1c2u);
}
