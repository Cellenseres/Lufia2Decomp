#include "lufia2/actor.h"
#include "actor/actor_internal.h"
#include "lufia2/field.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/joypad.h"
#include "system/wram.h"

enum {
    PROBE_ACTOR_SLOT = 0x56,
    PLAYER_DIRECTION = 0x22,
    PLAYER_STEP_DIRECTION = 0x7fd09d,
    TURN_ACTION_BASE = 0x18,
    FIELD_CONTROL_REQUEST = 8,
    ANY_INDEX_WIDTH = 2
};

typedef struct PlayerControl {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
} PlayerControl;

static uint8_t PlayerControlChild(
    const PlayerControl *control, uint32_t site, uint32_t target,
    uint8_t frame, uint8_t index_width, Lufia2ExecutionResult *result) {
    Lufia2CpuState *cpu = control->cpu;
    if (!CallChildWithFrame(control->memory, cpu, control->child,
            control->context, site, target, frame, 0x83u)) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit ||
        (index_width != ANY_INDEX_WIDTH &&
            cpu->index_is_8_bit != index_width)) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t TryPlayerInteraction(
    const PlayerControl *control, uint8_t *handled,
    Lufia2ExecutionResult *result) {
    const Lufia2Memory *memory = control->memory;
    Lufia2CpuState *cpu = control->cpu;
    OpLoadA(cpu, JOY_LOW_A | JOY_LOW_L);
    const uint8_t entry_index_width = cpu->index_is_8_bit;
    if (!PlayerControlChild(control, 0x83c1bbu, 0x83867bu,
            2u, entry_index_width, result))
        return 0u;
    if (!cpu->zero) {
        if (!PlayerControlChild(control, 0x83c1d0u, 0x83ba06u, 2u, 0u, result))
            return 0u;
        if (cpu->carry) {
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PROBE_ACTOR_SLOT)));
            if (!PlayerControlChild(control, 0x83c1d7u, 0x83bb08u,
                    2u, ANY_INDEX_WIDTH, result))
                return 0u;
            *handled = 1u;
            *result = ExecutionReturned(0x83c1e2u);
            return 1u;
        }
        if (!PlayerControlChild(control, 0x83c1dcu, 0x8ec05fu, 3u, 0u, result))
            return 0u;
        *handled = cpu->carry;
        *result = ExecutionReturned(0x83c1e2u);
        return 1u;
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
    if (!cpu->zero) {
        OpLoadA(cpu, JOY_HIGH_SELECT);
        if (!PlayerControlChild(control, 0x83c1c7u, 0x838674u,
                2u, entry_index_width, result))
            return 0u;
        if (!cpu->zero) {
            if (!PlayerControlChild(control, 0x83c1ccu, 0x8383e0u,
                    2u, ANY_INDEX_WIDTH, result))
                return 0u;
            *handled = 1u;
            *result = ExecutionReturned(0x83c1cfu);
        }
    }
    return 1u;
}

static uint8_t TurnPlayerAtObstacle(
    const PlayerControl *control, Lufia2ExecutionResult *result) {
    const Lufia2Memory *memory = control->memory;
    Lufia2CpuState *cpu = control->cpu;
    OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
    OpSta(memory, cpu, PLAYER_STEP_DIRECTION);
    cpu->carry = 0u;
    if (cpu->decimal) {
        *result = ExecutionHandoff(cpu, 0x83c20du);
        return 0u;
    }
    OpAdcValue(cpu, TURN_ACTION_BASE);
    if (!PlayerControlChild(control, 0x83c20fu, 0x83d350u, 3u, 1u, result))
        return 0u;
    if (!PlayerControlChild(control, 0x83c213u, 0x8eb63bu,
            3u, ANY_INDEX_WIDTH, result))
        return 0u;
    return 1u;
}

static uint8_t ProbePlayerStep(
    const PlayerControl *control, uint8_t *walk,
    Lufia2ExecutionResult *result) {
    const Lufia2Memory *memory = control->memory;
    Lufia2CpuState *cpu = control->cpu;
    OpRepWidths(cpu, 0x10u);
    if (!PlayerControlChild(control, 0x83c1efu, 0x83c0efu, 2u, 0u, result) ||
        !PlayerControlChild(control, 0x83c1f2u, 0x83c246u, 2u, 0u, result) ||
        !PlayerControlChild(control, 0x83c1f5u, 0x83fbbdu,
            2u, ANY_INDEX_WIDTH, result))
        return 0u;
    OpSepWidths(cpu, 0x10u);
    uint8_t turn;
    if (cpu->carry) {
        OpRepWidths(cpu, 0x10u);
        if (!PlayerControlChild(control, 0x83c1feu, 0x8ebbafu,
                3u, ANY_INDEX_WIDTH, result))
            return 0u;
        OpSepWidths(cpu, 0x10u);
        *walk = 0u;
        turn = !cpu->carry;
    } else {
        OpRepWidths(cpu, 0x10u);
        if (!PlayerControlChild(control, 0x83c21bu, 0x83c0efu, 2u, 0u, result) ||
            !PlayerControlChild(control, 0x83c21eu, 0x83c246u, 2u, 0u, result) ||
            !PlayerControlChild(control, 0x83c221u, 0x83fb12u, 3u, 0u, result) ||
            !PlayerControlChild(control, 0x83c225u, 0x83f9adu, 2u, 0u, result))
            return 0u;
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
        OpSepWidths(cpu, 0x10u);
        OpBitValue(cpu, 1u);
        turn = !cpu->zero;
    }
    if (turn) {
        if (!TurnPlayerAtObstacle(control, result))
            return 0u;
        *walk = 0u;
    }
    return 1u;
}

static uint8_t StartPlayerWalk(
    const PlayerControl *control, Lufia2ExecutionResult *result) {
    const Lufia2Memory *memory = control->memory;
    Lufia2CpuState *cpu = control->cpu;
    if (!PlayerControlChild(control, 0x83c232u, 0x83fc69u,
            2u, ANY_INDEX_WIDTH, result))
        return 0u;
    OpLda(memory, cpu, OpDp(cpu, PLAYER_DIRECTION));
    if (!PlayerControlChild(control, 0x83c237u, 0x83d350u,
            3u, ANY_INDEX_WIDTH, result) ||
        !PlayerControlChild(control, 0x83c23bu, 0x83c0fau,
            2u, ANY_INDEX_WIDTH, result))
        return 0u;
    OpLoadA(cpu, FIELD_CONTROL_REQUEST);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS), 1u);
    return 1u;
}

Lufia2ExecutionResult Lufia2PlayerSlotStandardUpdateWithServices(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x83c1b4u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    if (cpu->negative)
        return ExecutionReturned(0x83c1e2u);
    const PlayerControl control = {memory, cpu, child, context};
    Lufia2ExecutionResult result;
    uint8_t handled = 0u;
    if (!TryPlayerInteraction(&control, &handled, &result) || handled)
        return result;
    if (!PlayerControlChild(&control, 0x83c1e3u, 0x83c161u,
            2u, cpu->index_is_8_bit, &result))
        return result;
    if (!cpu->carry)
        return ExecutionReturned(0x83c1e2u);
    if (!PlayerControlChild(&control, 0x83c1e8u, 0x83fc56u,
            2u, cpu->index_is_8_bit, &result))
        return result;
    uint8_t walk = 1u;
    if (!cpu->carry && !ProbePlayerStep(&control, &walk, &result))
        return result;
    if (walk && !StartPlayerWalk(&control, &result))
        return result;
    OpSepWidths(cpu, 0x10u);
    return ExecutionReturned(0x83c245u);
}

static uint8_t PlayerNativeService(
    void *context, Lufia2CpuState *cpu, uint32_t target,
    uint32_t site, uint8_t frame) {
    const Lufia2Memory *memory = context;
    cpu->program_bank = (uint8_t)(target >> 16);
    Lufia2ExecutionResult result;
    uint8_t return_frame = 2u;
    switch (target) {
    case 0x83867bu:
        result = Lufia2FieldTakeButtons(memory, cpu);
        break;
    case 0x838674u:
        result = Lufia2FieldTakeHighButtons(memory, cpu);
        break;
    case 0x8383e0u:
        result = Lufia2FieldEncounterHandoff(memory, cpu, PlayerNativeService, context);
        break;
    case 0x83ba06u:
        result = Lufia2FieldProbeTalkTarget(memory, cpu, PlayerNativeService, context);
        break;
    case 0x83bb08u:
        result = Lufia2FieldTryActorInteraction(memory, cpu, PlayerNativeService, context);
        break;
    case 0x8ec05fu:
        result = Lufia2FieldTryObjectReward(memory, cpu, PlayerNativeService, context);
        return_frame = 3u;
        break;
    case 0x83c161u:
        result = Lufia2FieldReadPlayerDirection(memory, cpu);
        break;
    case 0x83fc56u:
        result = Lufia2FieldProbeInputAllowed(memory, cpu);
        break;
    case 0x83c0efu:
        result = Lufia2FieldProbeLeaderPosition(memory, cpu);
        break;
    case 0x83c246u:
        result = Lufia2FieldReadFacingStepCode(memory, cpu);
        break;
    case 0x83fbbdu:
        result = Lufia2FieldProbeDirectionBlocked(memory, cpu);
        break;
    case 0x8ebbafu:
        result = Lufia2FieldTryVehicleTransition(memory, cpu, PlayerNativeService, context);
        return_frame = 3u;
        break;
    case 0x83f9adu:
        result = Lufia2FieldObjectAttributeCell(memory, cpu);
        break;
    case 0x83fc69u:
        result = Lufia2FieldUpdateProbeAction(memory, cpu);
        break;
    case 0x83c0fau:
        result = Lufia2FieldAcknowledgeControlChange(memory, cpu);
        break;
    case 0x8eb63bu:
        result = Lufia2FieldTryItemRegion(memory, cpu, PlayerNativeService, context);
        return_frame = 3u;
        break;
    case 0x83f988u:
        result = Lufia2MapProbeTileHeight(memory, cpu);
        break;
    case 0x83ba5cu:
        result = Lufia2FieldProbeTalkDown(memory, cpu, PlayerNativeService, context);
        break;
    case 0x83ba80u:
        result = Lufia2FieldProbeTalkLeft(memory, cpu, PlayerNativeService, context);
        break;
    case 0x83ba96u:
        result = Lufia2FieldProbeTalkUp(memory, cpu, PlayerNativeService, context);
        break;
    case 0x83baacu:
        result = Lufia2FieldProbeTalkRight(memory, cpu, PlayerNativeService, context);
        break;
    case 0x83ba76u:
        result = Lufia2FieldProbeTalkBlocked(memory, cpu, PlayerNativeService, context);
        break;
    case 0x83bac2u:
        result = Lufia2FieldFindActorAtProbe(memory, cpu);
        break;
    case 0x80ad57u:
        result = Lufia2FieldFaceActorTowardActor(memory, cpu, PlayerNativeService, context);
        break;
    case 0x80ad39u:
        result = Lufia2FieldFaceActorGroupTowardActor(memory, cpu, PlayerNativeService, context);
        return_frame = 3u;
        break;
    case 0x80c1dfu:
        result = Lufia2FieldSelectInteractionVariant(memory, cpu, PlayerNativeService, context);
        return_frame = 3u;
        break;
    case 0x80bf92u:
        result = Lufia2FieldSelectActorById(memory, cpu, PlayerNativeService, context);
        return_frame = 3u;
        break;
    case 0x83ab4fu:
        result = Lufia2ActorSetRecordOffsets(memory, cpu);
        return_frame = 3u;
        break;
    case 0x80bfe7u:
        result = Lufia2SceneScriptSelectRecord(memory, cpu);
        return_frame = 3u;
        break;
    case 0x83bb76u:
        result = Lufia2FieldPrepareEventControl(memory, cpu);
        break;
    case 0x8482d5u:
        result = Lufia2ActorPreparePartyOffsets(memory, cpu);
        return_frame = 3u;
        break;

    case 0x83d350u:
        if (Lufia2ActorPrimaryActionCore(memory, cpu) !=
                LUFIA2_ACTOR_PRIMARY_ACTION_RETURN_D3AE)
            return 0u;
        result = ExecutionReturned(0x83d3aeu);
        return_frame = 3u;
        break;
    case 0x83fb12u: {
        const uint32_t return_pc = Lufia2ActorMovementStep(memory, cpu);
        if (!return_pc)
            return 0u;
        result = ExecutionReturned(return_pc);
        return_frame = 3u;
        break;
    }
    default:
        cpu->resume_pc = target;
        return 0u;
    }
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        if (result.flow == LUFIA2_EXECUTION_BOUNDARY)
            cpu->resume_pc = result.pc;
        return 0u;
    }
    const uint8_t low = Pull8(memory, cpu);
    const uint8_t high = Pull8(memory, cpu);
    if (return_frame == 3u)
        cpu->program_bank = Pull8(memory, cpu);
    const uint32_t resume = ((uint32_t)cpu->program_bank << 16) |
        (uint16_t)(((uint16_t)low | (uint16_t)high << 8) + 1u);
    if (resume != site + frame + 1u) {
        cpu->resume_pc = resume;
        return 0u;
    }
    return 1u;
}

Lufia2ExecutionResult Lufia2PlayerSlotStandardUpdate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = Lufia2PlayerSlotStandardUpdateWithServices(
        memory, cpu, PlayerNativeService, (void *)memory);
    if (result.flow == LUFIA2_EXECUTION_CHILD_UNWOUND)
        return ExecutionHandoff(cpu, cpu->resume_pc);
    return result;
}
