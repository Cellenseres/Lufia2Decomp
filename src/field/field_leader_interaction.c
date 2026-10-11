#include "lufia2/field.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    ACTION_BUTTONS = 0x46,
    PROBE_X = 0x8f,
    PROBE_Y = 0x91,
    PROBE_HEIGHT = 0x56,
    CLAIMED_ACTOR = 0xa7,
    LEADER_FACING_ACTIONS = 0x83c1a5,
    OBJECT_RECORDS = 0x7ef000
};

static uint8_t InteractionContext(const Lufia2CpuState *cpu, uint8_t bank) {
    return cpu->program_bank == bank && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t InteractionChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame,
    Lufia2ExecutionResult *result) {
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

Lufia2ExecutionResult Lufia2FieldStartFacingPointEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!InteractionContext(cpu, 0x80u) || !child)
        return ExecutionHandoff(cpu, 0x80e817u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, PROBE_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, PROBE_Y));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    Lufia2ExecutionResult result;
    if (!InteractionChild(memory, cpu, child, context,
            0x80e824u, 0x83fb12u, 3u, &result))
        return result;
    OpLdx(cpu, 0x20u);
    OpLdy(cpu, 3u);
    if (!InteractionChild(memory, cpu, child, context,
            0x80e82eu, 0x83b851u, 3u, &result))
        return result;
    if (!cpu->carry)
        return ExecutionReturned(0x80e843u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_RECORDS));
    OpAslA(cpu);
    OpTay(cpu);
    OpLdx(cpu, 2u);
    if (!InteractionChild(memory, cpu, child, context,
            0x80e83eu, 0x80e722u, 3u, &result))
        return result;
    cpu->carry = 1u;
    return ExecutionReturned(0x80e843u);
}

Lufia2ExecutionResult Lufia2FieldClaimFacingActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!InteractionContext(cpu, 0x83u) || !child)
        return ExecutionHandoff(cpu, 0x83bfb4u);
    Lufia2ExecutionResult result;
    if (!InteractionChild(memory, cpu, child, context,
            0x83bfb4u, 0x83c0efu, 2u, &result) ||
        !InteractionChild(memory, cpu, child, context,
            0x83bfb7u, 0x83f988u, 2u, &result))
        return result;
    OpSta(memory, cpu, OpDp(cpu, PROBE_HEIGHT));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    if (!InteractionChild(memory, cpu, child, context,
            0x83bfbfu, 0x83fb12u, 3u, &result) ||
        !InteractionChild(memory, cpu, child, context,
            0x83bfc3u, 0x83f988u, 2u, &result))
        return result;
    OpCmp(memory, cpu, OpDp(cpu, PROBE_HEIGHT));
    if (!cpu->zero)
        return ExecutionReturned(0x83c01bu);
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpOraValue(cpu, 1u);
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, LEADER_FACING_ACTIONS));
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x83bfddu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0cu);
    if (!InteractionChild(memory, cpu, child, context,
            0x83bfe0u, 0x83d3a1u, 3u, &result) ||
        !InteractionChild(memory, cpu, child, context,
            0x83bfe4u, 0x83c108u, 3u, &result))
        return result;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, (WRAM_FIELD_CLAIMED_ACTOR_IDS + 1u));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, CLAIMED_ACTOR)));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_PRIMARY_TIMER));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE4DE));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpOraValue(cpu, 8u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpLoadA(cpu, 0x11u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E070A));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_PRIMARY_TIMER));
    if (!InteractionChild(memory, cpu, child, context,
            0x83c013u, 0x83d416u, 3u, &result) ||
        !InteractionChild(memory, cpu, child, context,
            0x83c017u, 0x83c7c7u, 2u, &result))
        return result;
    cpu->carry = 1u;
    return ExecutionReturned(0x83c01bu);
}

static Lufia2ExecutionResult FacingCellFailure(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, 0x1du);
    Lufia2ExecutionResult result;
    if (!InteractionChild(memory, cpu, child, context,
            0x83c3a4u, 0x848766u, 3u, &result))
        return result;
    OpLoadA(cpu, 0x1du);
    if (!InteractionChild(memory, cpu, child, context,
            0x83c3aau, 0x83fc1cu, 2u, &result))
        return result;
    return ExecutionReturned(0x83c3dbu);
}

Lufia2ExecutionResult Lufia2FieldHandleFacingCellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!InteractionContext(cpu, 0x83u) || !child)
        return ExecutionHandoff(cpu, 0x83c375u);
    OpLda(memory, cpu, WRAM_UNK_7FD0FE);
    if (!cpu->zero)
        return ExecutionReturned(0x83c3dbu);
    Lufia2ExecutionResult result;
    if (!InteractionChild(memory, cpu, child, context,
            0x83c37bu, 0x83bf33u, 2u, &result))
        return result;
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0x0eu);
    if (!InteractionChild(memory, cpu, child, context,
            0x83c384u, 0x80e722u, 3u, &result))
        return result;
    OpLdx(cpu, 0x1cu);
    OpLdy(cpu, 4u);
    if (!InteractionChild(memory, cpu, child, context,
            0x83c38eu, 0x80e7fau, 3u, &result) ||
        !InteractionChild(memory, cpu, child, context,
            0x83c392u, 0x83c40eu, 2u, &result))
        return result;
    if (!cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LEADER_CELL_CATEGORY));
        OpCmpValue(cpu, 2u);
        if (cpu->zero)
            return ExecutionReturned(0x83c3dbu);
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            return ExecutionReturned(0x83c3dbu);
        return FacingCellFailure(memory, cpu, child, context);
    }
    if (!InteractionChild(memory, cpu, child, context,
            0x83c3b0u, 0x83c3dcu, 2u, &result))
        return result;
    if (!cpu->zero) {
        OpLoadA(cpu, 0u);
        if (!InteractionChild(memory, cpu, child, context,
                0x83c3b7u, 0x83f4b4u, 2u, &result))
            return result;
    } else {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LEADER_TILE_HEIGHT));
        OpCmp(memory, cpu, OpAbs(cpu, WRAM_FIELD_FACING_TILE_HEIGHT));
        if (!cpu->carry)
            return FacingCellFailure(memory, cpu, child, context);
        if (!InteractionChild(memory, cpu, child, context,
                0x83c3c4u, 0x83bf33u, 2u, &result) ||
            !InteractionChild(memory, cpu, child, context,
                0x83c3c7u, 0x83fb2eu, 3u, &result))
            return result;
        ExchangeAccumulatorBytes(cpu);
        OpBitValue(cpu, 2u);
        if (!cpu->zero)
            return FacingCellFailure(memory, cpu, child, context);
        OpLoadA(cpu, 0x13u);
        if (!InteractionChild(memory, cpu, child, context,
                0x83c3d2u, 0x848766u, 3u, &result))
            return result;
    }
    OpLoadA(cpu, 2u);
    if (!InteractionChild(memory, cpu, child, context,
            0x83c3d8u, 0x83fc1cu, 2u, &result))
        return result;
    return ExecutionReturned(0x83c3dbu);
}

static Lufia2ExecutionResult LeaderPendingAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 2u);
    if (cpu->zero) {
        OpLoadA(cpu, 0x10u);
        OpSta(memory, cpu, WRAM_ACTOR_SCENE_MOTION_OFFSET);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1471));
        OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
        OpOraValue(cpu, 2u);
        OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    }
    cpu->carry = 0u;
    return ExecutionReturned(0x83bf32u);
}

static uint8_t LeaderActorInteraction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 1u);
    if (!cpu->zero)
        return 0u;
    if (!InteractionChild(memory, cpu, child, context,
            0x83bebdu, 0x83ba06u, 2u, result))
        return 1u;
    if (!cpu->carry)
        return 0u;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PROBE_HEIGHT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpBitValue(cpu, 4u);
    if (!cpu->zero)
        return 0u;
    if (!InteractionChild(memory, cpu, child, context,
            0x83becbu, 0x83bb08u, 2u, result))
        return 1u;
    if (cpu->carry)
        return 0u;
    cpu->carry = 1u;
    *result = ExecutionReturned(0x83bf13u);
    return 1u;
}

Lufia2ExecutionResult Lufia2FieldHandleLeaderAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!InteractionContext(cpu, 0x83u) || !child)
        return ExecutionHandoff(cpu, 0x83be90u);
    Lufia2ExecutionResult result;
    if (!InteractionChild(memory, cpu, child, context,
            0x83be90u, 0x83bf33u, 2u, &result))
        return result;
    OpLoadA(cpu, 0xa0u);
    if (!InteractionChild(memory, cpu, child, context,
            0x83be95u, 0x83867bu, 2u, &result))
        return result;
    if (cpu->zero)
        return ExecutionReturned(0x83bf13u);
    if (!InteractionChild(memory, cpu, child, context,
            0x83be9au, 0x80e817u, 3u, &result))
        return result;
    if (cpu->carry) {
        if (!InteractionChild(memory, cpu, child, context,
                0x83bea0u, 0x83c32du, 2u, &result))
            return result;
        if (cpu->decimal)
            return ExecutionHandoff(cpu, 0x83bea3u);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x38u);
        if (!InteractionChild(memory, cpu, child, context,
                0x83bea6u, 0x83d3a1u, 3u, &result))
            return result;
        OpLoadA(cpu, 0x20u);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, ACTION_BUTTONS)));
        if (!cpu->zero)
            return ExecutionReturned(0x83bf13u);
    }
    if (!InteractionChild(memory, cpu, child, context,
            0x83beb0u, 0x8ec05fu, 3u, &result))
        return result;
    if (cpu->carry)
        return ExecutionReturned(0x83bf13u);
    if (LeaderActorInteraction(memory, cpu, child, context, &result))
        return result;
    if (!InteractionChild(memory, cpu, child, context,
            0x83bed3u, 0x83bf33u, 2u, &result))
        return result;
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    if (!cpu->zero)
        return ExecutionReturned(0x83bf13u);
    if (!InteractionChild(memory, cpu, child, context,
            0x83bedcu, 0x83fb8bu, 2u, &result))
        return result;
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAST_PENDING_OBJECT), cpu->x);
    OpBitValue(cpu, 8u);
    if (!cpu->zero)
        return LeaderPendingAction(memory, cpu);
    OpBitValue(cpu, 4u);
    if (!cpu->zero) {
        if (!InteractionChild(memory, cpu, child, context,
                0x83beeau, 0x83bfb4u, 2u, &result))
            return result;
        cpu->carry = 1u;
        return ExecutionReturned(0x83bf13u);
    }
    OpLoadA(cpu, 0x20u);
    OpBit(memory, cpu, OpDp(cpu, ACTION_BUTTONS));
    cpu->carry = 0u;
    if (cpu->zero)
        return ExecutionReturned(0x83bf13u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    OpBitValue(cpu, 0x40u);
    cpu->carry = 0u;
    if (!cpu->zero)
        return ExecutionReturned(0x83bf13u);
    if (!InteractionChild(memory, cpu, child, context,
            0x83beffu, 0x83c40eu, 2u, &result))
        return result;
    if (cpu->carry) {
        if (!InteractionChild(memory, cpu, child, context,
                0x83bf04u, 0x83c3dcu, 2u, &result))
            return result;
        if (!cpu->zero) {
            if (!InteractionChild(memory, cpu, child, context,
                    0x83bf09u, 0x83c375u, 2u, &result))
                return result;
            cpu->carry = 1u;
            return ExecutionReturned(0x83bf13u);
        }
    }
    if (!InteractionChild(memory, cpu, child, context,
            0x83bf0fu, 0x83c24fu, 2u, &result))
        return result;
    cpu->carry = 1u;
    return ExecutionReturned(0x83bf13u);
}
