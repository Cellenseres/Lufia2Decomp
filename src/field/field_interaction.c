#include "lufia2/field.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    SOURCE_ACTOR_ID = 0x56,
    TARGET_ACTOR_ID = 0x57,
    ACTOR_SLOT = 0xa7,
    TARGET_ACTOR_SLOT = 0x5a,
    TARGET_TILE_X = 0x8f,
    TARGET_TILE_Y = 0x91,
    DIRECTION_X = 0x54,
    DIRECTION_Y = 0x55,
    DISTANCE_X = 0x58,
    DISTANCE_Y = 0x59,
    INTERACTION_ACTOR_SLOT = 0x124e,
    INTERACTION_VARIANT = 0x1260,
    FACING_GROUP_LAST_SLOT = 7,
    WRAM_FIELD_INTERACTION_FACING_ACTOR_ID_LIMIT = 15,
    FACE_DOWN = 0,
    FACE_LEFT = 2,
    FACE_UP = 4,
    FACE_RIGHT = 6
};

static uint8_t InteractionContext(const Lufia2CpuState *cpu, uint8_t bank) {
    return cpu->program_bank == bank && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t InteractionChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, (uint8_t)(site >> 16))) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t MeasureFacingAxis(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t target, uint16_t actor_axis, uint8_t direction,
    uint8_t distance, uint32_t subtraction_pc, Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpDp(cpu, target));
    cpu->carry = 1u;
    if (cpu->decimal) {
        *result = ExecutionHandoff(cpu, subtraction_pc);
        return 0u;
    }
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, actor_axis)));
    OpSta(memory, cpu, OpDp(cpu, direction));
    OpSta(memory, cpu, OpDp(cpu, distance));
    if (cpu->negative) {
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, distance));
    }
    return 1u;
}

static void FaceAlongAxis(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t direction,
    uint8_t negative_facing, uint8_t positive_facing) {
    OpLda(memory, cpu, OpDp(cpu, direction));
    OpLoadA(cpu, cpu->negative ? negative_facing : positive_facing);
}

static void StoreActorFacing(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FACING));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
}

Lufia2ExecutionResult Lufia2FieldFaceActorTowardActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!InteractionContext(cpu, 0x80u) || !child)
        return ExecutionHandoff(cpu, 0x80ad57u);
    Lufia2ExecutionResult result;
    OpLda(memory, cpu, OpDp(cpu, TARGET_ACTOR_ID));
    if (!InteractionChild(memory, cpu, child, context,
            0x80ad59u, 0x80bf92u, 3u, &result))
        return result;
    if (cpu->carry)
        return ExecutionReturned(0x80add0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpWriteX(memory, cpu, OpDp(cpu, TARGET_ACTOR_SLOT), cpu->x);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, TARGET_TILE_X));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, TARGET_TILE_Y));
    OpLda(memory, cpu, OpDp(cpu, SOURCE_ACTOR_ID));
    if (!InteractionChild(memory, cpu, child, context,
            0x80ad6fu, 0x80bf92u, 3u, &result))
        return result;
    if (cpu->carry)
        return ExecutionReturned(0x80add0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpStz(memory, cpu, OpDp(cpu, DIRECTION_X));
    OpStz(memory, cpu, OpDp(cpu, DIRECTION_Y));
    if (!MeasureFacingAxis(memory, cpu, TARGET_TILE_X, WRAM_ACTOR_TILE_X,
            DIRECTION_X, DISTANCE_X, 0x80ad7eu, &result))
        return result;
    if (!MeasureFacingAxis(memory, cpu, TARGET_TILE_Y, WRAM_ACTOR_TILE_Y,
            DIRECTION_Y, DISTANCE_Y, 0x80ad8fu, &result))
        return result;
    OpLda(memory, cpu, OpDp(cpu, DISTANCE_Y));
    OpCmp(memory, cpu, OpDp(cpu, DISTANCE_X));
    uint8_t vertical = cpu->carry;
    if (cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, TARGET_ACTOR_SLOT)));
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FACING));
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
        OpCmpValue(cpu, FACE_UP);
        vertical = cpu->zero;
        if (!vertical) {
            OpCmpValue(cpu, FACE_DOWN);
            vertical = cpu->zero;
        }
    }
    if (vertical)
        FaceAlongAxis(memory, cpu, DIRECTION_Y, FACE_UP, FACE_DOWN);
    else
        FaceAlongAxis(memory, cpu, DIRECTION_X, FACE_LEFT, FACE_RIGHT);
    StoreActorFacing(memory, cpu);
    return ExecutionReturned(0x80add0u);
}

Lufia2ExecutionResult Lufia2FieldFaceActorGroupTowardActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!InteractionContext(cpu, 0x80u) || !child)
        return ExecutionHandoff(cpu, 0x80ad39u);
    OpSta(memory, cpu, OpDp(cpu, TARGET_ACTOR_ID));
    OpLdx(cpu, FACING_GROUP_LAST_SLOT);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        OpBitValue(cpu, 4u);
        if (!cpu->zero) {
            OpDex(cpu);
            continue;
        }
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
        OpCmpValue(cpu, WRAM_FIELD_INTERACTION_FACING_ACTOR_ID_LIMIT);
        if (!cpu->carry) {
            OpSta(memory, cpu, OpDp(cpu, SOURCE_ACTOR_ID));
            OpPushX(memory, cpu);
            Lufia2ExecutionResult result;
            if (!InteractionChild(memory, cpu, child, context,
                    0x80ad4fu, 0x80ad57u, 2u, &result))
                return result;
            OpPullX(memory, cpu);
        }
        OpDex(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x80ad56u);
}

Lufia2ExecutionResult Lufia2FieldSelectInteractionVariant(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!InteractionContext(cpu, 0x80u) || !child)
        return ExecutionHandoff(cpu, 0x80c1dfu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_INTERACTION_ACTOR_ID));
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        Lufia2ExecutionResult result;
        if (!InteractionChild(memory, cpu, child, context,
                0x80c1e6u, 0x80bf92u, 3u, &result))
            return result;
        if (!cpu->carry) {
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1291));
            for (unsigned bit = 0u; bit < 5u; ++bit)
                OpLsrA(cpu);
            cpu->carry = 0u;
            if (cpu->decimal)
                return ExecutionHandoff(cpu, 0x80c1f7u);
            OpAdcValue(cpu, 0x45u);
            OpSta(memory, cpu, OpAbs(cpu, INTERACTION_VARIANT));
        }
    }
    return ExecutionReturned(0x80c1fcu);
}

static void FaceInteractionLeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    if (!cpu->zero) {
        OpLoadA(cpu, FACE_UP);
        if (!cpu->carry)
            OpLoadA(cpu, FACE_DOWN);
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
        OpCmp(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
        OpLoadA(cpu, FACE_LEFT);
        if (!cpu->carry)
            OpLoadA(cpu, FACE_RIGHT);
    }
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FACING));
}

static Lufia2ExecutionResult RejectInteraction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PullDataBank(memory, cpu);
    cpu->carry = 1u;
    return ExecutionReturned(0x83bb75u);
}

Lufia2ExecutionResult Lufia2FieldTryActorInteraction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!InteractionContext(cpu, 0x83u) || cpu->direct_page || !child)
        return ExecutionHandoff(cpu, 0x83bb08u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, INTERACTION_ACTOR_SLOT), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, ACTOR_SLOT), cpu->x);
    Lufia2ExecutionResult result;
    if (!InteractionChild(memory, cpu, child, context,
            0x83bb10u, 0x83ab4fu, 3u, &result))
        return result;
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
    OpSta(memory, cpu, OpDp(cpu, DIRECTION_X));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_INTERACTION_ACTOR_ID));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_INTERACTION_FACING_ACTOR_ID));
    if (!InteractionChild(memory, cpu, child, context,
            0x83bb1fu, 0x80bfe7u, 3u, &result))
        return result;
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09B7)));
    OpCpx(cpu, 0xffffu);
    if (cpu->zero)
        return RejectInteraction(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero)
        return RejectInteraction(memory, cpu);
    OpOraValue(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_ACTOR_STATE), 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    FaceInteractionLeader(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_INTERACTION_FACING_ACTOR_ID));
    if (!InteractionChild(memory, cpu, child, context,
            0x83bb65u, 0x80ad39u, 3u, &result))
        return result;
    if (!InteractionChild(memory, cpu, child, context,
            0x83bb69u, 0x83bb76u, 2u, &result))
        return result;
    PullDataBank(memory, cpu);
    if (!InteractionChild(memory, cpu, child, context,
            0x83bb6du, 0x80c1dfu, 3u, &result))
        return result;
    cpu->carry = 0u;
    return ExecutionReturned(0x83bb72u);
}
