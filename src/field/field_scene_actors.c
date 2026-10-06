#include "actor/actor_slot_view.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    FIELD_ACTOR_OBJECT_OFFSET = 40u,
    FIELD_SCENE_LOOP_LIMIT = 8192u
};

static Lufia2ExecutionResult SceneActorChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x83u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    return ExecutionReturned(site + frame + 1u);
}

static Lufia2ExecutionResult RebuildActors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLdx(cpu, WRAM_ACTOR_STATE_COUNT - 1u);
    unsigned iterations = 0u;
    do {
        if (iterations++ == FIELD_SCENE_LOOP_LIMIT)
            return ExecutionHandoff(cpu, 0x83a776u);
        const Lufia2ActorSlotView slot =
            Lufia2ActorSlotAt(memory, cpu, cpu->x);
        LoadA8(cpu, Lufia2ActorSlotReadMirrored(&slot, WRAM_UNK_7E05D2));
        OpCmpValue(cpu, 0xfeu);
        if (!cpu->zero) {
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero) {
                OpDex(cpu);
                continue;
            }
        }
        OpWriteX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT), cpu->x);
        Lufia2ExecutionResult result = SceneActorChild(
            memory, cpu, child, context, 0x83a783u, 0x83ab4fu, 3u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        if (!cpu->accumulator_is_8_bit)
            return ExecutionHandoff(cpu, 0x83a787u);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
        OpCmpValue(cpu, 0xfeu);
        if (cpu->zero) {
            OpRepWidths(cpu, 0x10u);
            result = SceneActorChild(
                memory, cpu, child, context, 0x83a7bbu, 0x83f6cau, 2u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            if (!cpu->accumulator_is_8_bit)
                return ExecutionHandoff(cpu, 0x83a7beu);
            OpSepWidths(cpu, 0x10u);
        } else {
            result = SceneActorChild(
                memory, cpu, child, context, 0x83a78eu, 0x83a9bau, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            if (!cpu->accumulator_is_8_bit)
                return ExecutionHandoff(cpu, 0x83a792u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
            OpBitValue(cpu, 1u);
            if (cpu->zero) {
                OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
                OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
                OpBitValue(cpu, 0x20u);
                if (cpu->zero) {
                    OpLoadA(cpu, 4u);
                    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE4DE));
                }
            }
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1471));
            result = SceneActorChild(
                memory, cpu, child, context, 0x83a7aeu, 0x83aae5u, 2u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            result = SceneActorChild(
                memory, cpu, child, context, 0x83a7b1u, 0x848193u, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            if (!cpu->accumulator_is_8_bit)
                return ExecutionHandoff(cpu, 0x83a7b5u);
            OpSepWidths(cpu, 0x10u);
        }
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
        OpDex(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x83a7c5u);
}

static Lufia2ExecutionResult RebuildObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, WRAM_OBJECT_STATE_COUNT - 1u);
    unsigned iterations = 0u;
    do {
        if (iterations++ == FIELD_SCENE_LOOP_LIMIT)
            return ExecutionHandoff(cpu, 0x83a7cau);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_OBJECT_STATE));
        OpBitValue(cpu, 0x80u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_ID));
            OpCmpValue(cpu, 0xffu);
            if (!cpu->zero) {
                OpPushX(memory, cpu);
                OpTxa(cpu);
                OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
                Lufia2ExecutionResult result = SceneActorChild(
                    memory, cpu, child, context, 0x83a7e2u, 0x83ab4fu, 3u);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                result = SceneActorChild(
                    memory, cpu, child, context, 0x83a7e6u, 0x83ecdeu, 2u);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
                    return ExecutionHandoff(cpu, 0x83a7e9u);
                OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
                cpu->carry = 0u;
                OpAdcValue(cpu, FIELD_ACTOR_OBJECT_OFFSET);
                OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
                result = SceneActorChild(
                    memory, cpu, child, context, 0x83a7f0u, 0x83ab4fu, 3u);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                result = SceneActorChild(
                    memory, cpu, child, context, 0x83a7f4u, 0x83fcd1u, 2u);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                result = SceneActorChild(
                    memory, cpu, child, context, 0x83a7f7u, 0x848193u, 3u);
                if (result.flow != LUFIA2_EXECUTION_RETURNED)
                    return result;
                if (!cpu->accumulator_is_8_bit)
                    return ExecutionHandoff(cpu, 0x83a7fbu);
                OpRepWidths(cpu, 0x10u);
                OpPullX(memory, cpu);
            }
        }
        OpDex(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x83a801u);
}

static void CopySharedObjectSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, WRAM_OBJECT_STATE_COUNT - 1u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_OBJECT_STATE));
        OpBitValue(cpu, 0x80u);
        if (!cpu->zero) {
            /* The original high byte contributes to the source index. */
            TransferDirectToA(cpu);
            OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE3A6));
            OpCmpValue(cpu, 0xffu);
            if (!cpu->zero) {
                OpPushX(memory, cpu);
                OpTax(cpu);
                OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_B));
                ExchangeAccumulatorBytes(cpu);
                OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_A));
                OpPullX(memory, cpu);
                OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_A));
                ExchangeAccumulatorBytes(cpu);
                OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_B));
            }
        }
        OpDex(cpu);
    } while (!cpu->negative);
}

Lufia2ExecutionResult Lufia2FieldRebuildSceneActors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu,
            ((uint32_t)cpu->program_bank << 16) | 0xa76du);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    Lufia2ExecutionResult result = SceneActorChild(
        memory, cpu, child, context, 0x83a770u, 0x83ac7au, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (!cpu->accumulator_is_8_bit || !cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83a774u);
    result = RebuildActors(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = RebuildObjects(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    CopySharedObjectSprites(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83a82du);
}
