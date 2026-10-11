#include "lufia2/field.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/joypad.h"
#include "system/wram.h"

enum {
    PLAYER_DIRECTION = 0x22,
    PLAYER_STEP_OFFSET = 0x23,
    ACTION_BUTTONS = 0x46,
    PROBE_HEIGHT = 0x56,
    CLAIM_DIRECTION = 0x65,
    ACTOR_SLOT = 0xa7,
    FACING_ACTIONS = 0x83c1a5,
    STEP_HEIGHT = 0x54
};

static uint8_t PlayerActionContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t PlayerActionChild(
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

Lufia2ExecutionResult Lufia2FieldReleaseClaimedActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PlayerActionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83bf3eu);
    Lufia2ExecutionResult result;
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bf3eu, 0x83c0efu, 2u, &result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83bf41u, 0x83f988u, 2u, &result))
        return result;
    OpSta(memory, cpu, OpDp(cpu, PROBE_HEIGHT));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bf4bu, 0x83d87du, 2u, &result))
        return result;
    if (!cpu->negative && !cpu->zero)
        return ExecutionReturned(0x83bfb3u);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bf52u, 0x83bf33u, 2u, &result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83bf55u, 0x83c33du, 2u, &result))
        return result;
    if (cpu->carry)
        return ExecutionReturned(0x83bfb3u);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bf5au, 0x83f988u, 2u, &result))
        return result;
    OpCmp(memory, cpu, OpDp(cpu, PROBE_HEIGHT));
    if (!cpu->zero)
        return ExecutionReturned(0x83bfb3u);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bf61u, 0x83fb71u, 3u, &result))
        return result;
    const uint8_t blocked_cells[] = {2u, 10u, 14u, 3u};
    for (unsigned i = 0u; i < sizeof blocked_cells; ++i) {
        OpCmpValue(cpu, blocked_cells[i]);
        if (cpu->zero)
            return ExecutionReturned(0x83bfb3u);
    }
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, FACING_ACTIONS));
    OpSta(memory, cpu, OpDp(cpu, CLAIM_DIRECTION));
    cpu->carry = 0u;
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x83bf81u);
    OpAdcValue(cpu, 0x30u);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bf83u, 0x83d350u, 3u, &result))
        return result;
    OpLda(memory, cpu, WRAM_FIELD_CLAIMED_ACTOR_IDS);
    OpSta(memory, cpu, OpDp(cpu, ACTOR_SLOT));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bf8du, 0x83ab4fu, 3u, &result))
        return result;
    OpLda(memory, cpu, OpDp(cpu, CLAIM_DIRECTION));
    cpu->carry = 0u;
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x83bf94u);
    OpAdcValue(cpu, 0x34u);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bf96u, 0x83d350u, 3u, &result))
        return result;
    OpStz(memory, cpu, OpDp(cpu, ACTOR_SLOT));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bf9cu, 0x83ab4fu, 3u, &result))
        return result;
    OpLoadA(cpu, 0x42u);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bfa2u, 0x848766u, 3u, &result))
        return result;
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpAndValue(cpu, 0xfeu);
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bfb0u, 0x83c7c7u, 2u, &result))
        return result;
    return ExecutionReturned(0x83bfb3u);
}

Lufia2ExecutionResult Lufia2FieldBeginObjectPush(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PlayerActionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83c01cu);
    Lufia2ExecutionResult result;
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83c01cu, 0x83c0efu, 2u, &result))
        return result;
    OpLda(memory, cpu, OpDp(cpu, PLAYER_STEP_OFFSET));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83c021u, 0x83fb12u, 3u, &result))
        return result;
    OpLda(memory, cpu, OpDp(cpu, PLAYER_STEP_OFFSET));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83c027u, 0x83c079u, 3u, &result))
        return result;
    if (!cpu->carry)
        return ExecutionReturned(0x83c078u);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83c030u, 0x83c0efu, 2u, &result))
        return result;
    OpLda(memory, cpu, OpDp(cpu, PLAYER_STEP_OFFSET));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83c035u, 0x83fb12u, 3u, &result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83c039u, 0x83fb9fu, 2u, &result))
        return result;
    OpTxa(cpu);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_PUSH_OBJECT);
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, WRAM_UNK_7FE4DE);
    OpSta(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, WRAM_ACTOR_SCENE_MOTION_OFFSET);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83c04fu, 0x80dcdau, 3u, &result))
        return result;
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, WRAM_FIELD_CLAIMED_ACTOR_IDS + 1u);
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpOraValue(cpu, 2u);
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpStz(memory, cpu, OpDp(cpu, ACTOR_SLOT));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83c065u, 0x83ab4fu, 3u, &result))
        return result;
    OpLda(memory, cpu, WRAM_FIELD_SELECTED_DIRECTION);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83c06du, 0x83d350u, 3u, &result))
        return result;
    OpLoadA(cpu, 0x1au);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83c073u, 0x848766u, 3u, &result))
        return result;
    cpu->carry = 1u;
    return ExecutionReturned(0x83c078u);
}

static Lufia2ExecutionResult RestoreLeaderRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t start) {
    OpStz(memory, cpu, OpDp(cpu, ACTOR_SLOT));
    Lufia2ExecutionResult result;
    if (!PlayerActionChild(memory, cpu, child, context,
            start + 2u, 0x83ab4fu, 3u, &result))
        return result;
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_UNK_7FD0AE);
    return ExecutionReturned(start + 12u);
}

static Lufia2ExecutionResult ContinuePlayerMovement(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09A8));
    OpBitValue(cpu, 8u);
    if (cpu->zero) {
        Lufia2ExecutionResult result;
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bcb8u, 0x83bcc8u, 2u, &result))
            return result;
    }
    return RestoreLeaderRecord(memory, cpu, child, context, 0x83bcbbu);
}

static uint8_t TryPlayerItemSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    if (!cpu->zero)
        return 0u;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    OpBitValue(cpu, 0x40u);
    if (!cpu->zero)
        return 0u;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SCENE_TEXT_CONTROL));
    OpBitValue(cpu, 0x20u);
    if (!cpu->zero)
        return 0u;
    OpLoadA(cpu, JOY_HIGH_SELECT);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bc56u, 0x838674u, 2u, result))
        return 1u;
    if (cpu->zero)
        return 0u;
    OpLda(memory, cpu, WRAM_UNK_7FD0FE);
    if (!cpu->zero)
        return 0u;
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bc61u, 0x83c0efu, 2u, result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83bc64u, 0x83fb71u, 3u, result))
        return 1u;
    OpCmpValue(cpu, 2u);
    if (cpu->zero)
        return 0u;
    if (PlayerActionChild(memory, cpu, child, context,
            0x83bc6cu, 0x83c460u, 2u, result))
        *result = RestoreLeaderRecord(memory, cpu, child, context, 0x83bdefu);
    return 1u;
}

Lufia2ExecutionResult Lufia2FieldCoordinatePlayerAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PlayerActionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83bc28u);
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 1u);
    Lufia2ExecutionResult result;
    if (!cpu->zero) {
        OpLoadA(cpu, JOY_LOW_A | JOY_LOW_L);
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bc32u, 0x83867bu, 2u, &result))
            return result;
        if (cpu->zero)
            return ContinuePlayerMovement(memory, cpu, child, context);
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bc3au, 0x83bf3eu, 2u, &result))
            return result;
        return RestoreLeaderRecord(memory, cpu, child, context, 0x83bdefu);
    }
    if (TryPlayerItemSelection(memory, cpu, child, context, &result))
        return result;
    OpLoadA(cpu, JOY_LOW_A | JOY_LOW_L);
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, ACTION_BUTTONS)));
    if (!cpu->zero) {
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bc78u, 0x83be90u, 2u, &result))
            return result;
        return cpu->carry ?
            RestoreLeaderRecord(memory, cpu, child, context, 0x83bcbbu) :
            ContinuePlayerMovement(memory, cpu, child, context);
    }
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bc7fu, 0x83c180u, 2u, &result))
        return result;
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 0x7cu);
    if (!cpu->zero)
        return RestoreLeaderRecord(memory, cpu, child, context, 0x83bcbbu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    OpBitValue(cpu, 0x40u);
    if (!cpu->zero)
        return ContinuePlayerMovement(memory, cpu, child, context);
    OpLda(memory, cpu, WRAM_UNK_7FD0AE);
    OpAndValue(cpu, 0x80u);
    uint8_t cell_action = !cpu->zero;
    if (!cell_action) {
        OpLoadA(cpu, JOY_HIGH_B);
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bc9bu, 0x838674u, 2u, &result))
            return result;
        cell_action = !cpu->zero;
    }
    if (cell_action) {
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bca0u, 0x83c375u, 2u, &result))
            return result;
    } else {
        OpLoadA(cpu, JOY_HIGH_Y);
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bca7u, 0x838674u, 2u, &result))
            return result;
        if (cpu->zero)
            return ContinuePlayerMovement(memory, cpu, child, context);
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bcacu, 0x83c24fu, 2u, &result))
            return result;
    }
    return RestoreLeaderRecord(memory, cpu, child, context, 0x83bcbbu);
}

static Lufia2ExecutionResult FinishPlayerMovement(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bdecu, 0x83c7c7u, 2u, &result))
        return result;
    return RestoreLeaderRecord(memory, cpu, child, context, 0x83bdefu);
}

static Lufia2ExecutionResult BlockPlayerMovement(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_FACING_CHANGE_MARKER));
    cpu->carry = 0u;
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x83bd8du);
    OpAdcValue(cpu, 4u);
    Lufia2ExecutionResult result;
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bd8fu, 0x83d350u, 3u, &result))
        return result;
    return FinishPlayerMovement(memory, cpu, child, context);
}

static Lufia2ExecutionResult CommitPlayerStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, WRAM_UNK_7FE4DE);
    Lufia2ExecutionResult result;
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bd9cu, 0x83fc69u, 2u, &result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83bd9fu, 0x83c0efu, 2u, &result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83bda2u, 0x83fb71u, 3u, &result))
        return result;
    OpCmpValue(cpu, 5u);
    uint8_t cell_action = cpu->zero;
    if (cell_action) {
        OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
        OpCmpValue(cpu, 1u);
        cell_action = cpu->zero;
    }
    if (cell_action)
        OpLoadA(cpu, 8u);
    else
        OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bdb6u, 0x83d350u, 3u, &result))
        return result;
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 1u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_FACING_CHANGE_MARKER));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bdc4u, 0x83c180u, 2u, &result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83bdc7u, 0x83c0fau, 2u, &result))
        return result;
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        OpLda(memory, cpu, WRAM_FIELD_CLAIMED_ACTOR_IDS);
        OpSta(memory, cpu, OpDp(cpu, ACTOR_SLOT));
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bdd8u, 0x83ab4fu, 3u, &result))
            return result;
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
        OpLda(memory, cpu, WRAM_UNK_7FE4DE);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE4DE));
        OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bde8u, 0x83d350u, 3u, &result))
            return result;
    }
    return FinishPlayerMovement(memory, cpu, child, context);
}

static Lufia2ExecutionResult TryMovementRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpDp(cpu, PLAYER_STEP_OFFSET));
    Lufia2ExecutionResult result;
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bd44u, 0x83fb12u, 3u, &result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83bd48u, 0x83fb8bu, 2u, &result))
        return result;
    OpBitValue(cpu, 8u);
    if (!cpu->zero) {
        OpLoadA(cpu, JOY_LOW_A | JOY_LOW_L);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, ACTION_BUTTONS)));
        if (!cpu->zero) {
            if (!PlayerActionChild(memory, cpu, child, context,
                    0x83bd82u, 0x83c01cu, 2u, &result))
                return result;
            if (cpu->carry)
                return FinishPlayerMovement(memory, cpu, child, context);
        }
        return BlockPlayerMovement(memory, cpu, child, context);
    }
    OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
    OpSta(memory, cpu, WRAM_FIELD_LAST_MOVEMENT_DIRECTION);
    OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
    cpu->carry = 0u;
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x83bd58u);
    OpAdcValue(cpu, 0x18u);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bd5au, 0x83d350u, 3u, &result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83bd5eu, 0x8eb63bu, 3u, &result))
        return result;
    if (cpu->carry)
        return FinishPlayerMovement(memory, cpu, child, context);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bd67u, 0x8ebbafu, 3u, &result))
        return result;
    if (cpu->carry)
        return FinishPlayerMovement(memory, cpu, child, context);
    OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_FIELD_FACING_CHANGE_MARKER));
    if (cpu->zero) {
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bd74u, 0x83bdfcu, 2u, &result))
            return result;
        if (cpu->carry)
            return FinishPlayerMovement(memory, cpu, child, context);
    }
    return BlockPlayerMovement(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2FieldHandlePlayerMovement(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!PlayerActionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83bcc8u);
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 0x1cu);
    if (!cpu->zero)
        return ExecutionReturned(0x83bcdeu);
    Lufia2ExecutionResult result;
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bcd0u, 0x83c161u, 2u, &result))
        return result;
    if (!cpu->carry)
        return ExecutionReturned(0x83bcdeu);
    OpLoadA(cpu, JOY_LOW_R);
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, ACTION_BUTTONS)));
    if (!cpu->zero) {
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bcdbu, 0x83be64u, 2u, &result))
            return result;
        return ExecutionReturned(0x83bcdeu);
    }
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bcdfu, 0x83fc56u, 2u, &result))
        return result;
    if (cpu->carry)
        return CommitPlayerStep(memory, cpu, child, context);
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bcefu, 0x83c0efu, 2u, &result) ||
            !PlayerActionChild(memory, cpu, child, context,
                0x83bcf2u, 0x83f988u, 2u, &result))
            return result;
        OpSta(memory, cpu, OpDp(cpu, STEP_HEIGHT));
        OpLda(memory, cpu, OpDp(cpu, PLAYER_STEP_OFFSET));
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bcf9u, 0x83fb12u, 3u, &result) ||
            !PlayerActionChild(memory, cpu, child, context,
                0x83bcfdu, 0x83f988u, 2u, &result))
            return result;
        OpCmp(memory, cpu, OpDp(cpu, STEP_HEIGHT));
        if (!cpu->zero)
            return BlockPlayerMovement(memory, cpu, child, context);
        if (!PlayerActionChild(memory, cpu, child, context,
                0x83bd07u, 0x83fb71u, 3u, &result))
            return result;
        const uint8_t blocked_cells[] = {4u, 2u, 14u};
        for (unsigned i = 0u; i < sizeof blocked_cells; ++i) {
            OpCmpValue(cpu, blocked_cells[i]);
            if (cpu->zero)
                return BlockPlayerMovement(memory, cpu, child, context);
        }
    }
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTOR_SLOT)));
    OpLda(memory, cpu, OpDp(cpu, PLAYER_STEP_OFFSET));
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bd1bu, 0x83d87du, 2u, &result))
        return result;
    if (cpu->zero)
        return CommitPlayerStep(memory, cpu, child, context);
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 1u);
    if (!cpu->zero)
        return BlockPlayerMovement(memory, cpu, child, context);
    if (!PlayerActionChild(memory, cpu, child, context,
            0x83bd28u, 0x83c0efu, 2u, &result) ||
        !PlayerActionChild(memory, cpu, child, context,
            0x83bd2bu, 0x83fb71u, 3u, &result))
        return result;
    OpCmpValue(cpu, 5u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLoadA(cpu, 4u);
            if (!PlayerActionChild(memory, cpu, child, context,
                    0x83bd3bu, 0x83d350u, 3u, &result))
                return result;
            return FinishPlayerMovement(memory, cpu, child, context);
        }
    }
    return TryMovementRegion(memory, cpu, child, context);
}
