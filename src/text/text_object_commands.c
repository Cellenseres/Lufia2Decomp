#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    SET_OBJECT_MOTION = 0xaa43u,
    SET_OBJECT_MOTION_FLAG = 0xaa49u,
    SET_OBJECT_HORIZONTAL = 0xaa83u,
    SET_OBJECT_VERTICAL = 0xaaa1u,
    RESET_MATCHING_OBJECTS = 0xaabfu,
    WAIT_OBJECT_PARAMETER = 0xaae6u,
    WAIT_OBJECT_PRESENT = 0xab1du,
    FACE_ACTOR = 0xad02u,
    FACE_ACTOR_PAIR = 0xad12u,
    FACE_ACTOR_GROUP = 0xad2fu,
    MOVE_ACTOR_TO_SCENE = 0xb021u,
    QUEUE_OBJECT_ANIMATION = 0xb92cu,
    QUEUE_OBJECT_ANIMATION_40 = 0xb93du,
    OBJECT_COUNT = 32u,
    DP_OBJECT_KIND = 0x54u,
    DP_OBJECT_PARAMETER = 0x55u,
    DP_SOURCE_ACTOR = 0x56u,
    DP_TARGET_ACTOR = 0x57u,
    DP_PARAMETER_FLAGS_LOW = 0x58u,
    DP_PARAMETER_FLAGS_HIGH = 0x59u
};

typedef struct TextObjectService {
    const Lufia2Memory *memory;
    uint32_t handoff;
} TextObjectService;

static uint8_t RunObjectChild(void *context, Lufia2CpuState *cpu, uint32_t target,
                              uint32_t site, uint8_t frame) {
    TextObjectService *service = context;
    const Lufia2Memory *memory = service->memory;
    const uint8_t caller_bank = cpu->program_bank;
    cpu->program_bank = (uint8_t)(target >> 16);
    Lufia2ExecutionResult result;
    switch (target) {
    case 0x80bf92u:
        result = Lufia2FieldSelectActorById(memory, cpu, RunObjectChild, context);
        break;
    case 0x80ad57u:
        result = Lufia2FieldFaceActorTowardActor(memory, cpu, RunObjectChild, context);
        break;
    case 0x80ad39u:
        result =
            Lufia2FieldFaceActorGroupTowardActor(memory, cpu, RunObjectChild, context);
        break;
    case 0x8482d5u:
        result = Lufia2ActorPreparePartyOffsets(memory, cpu);
        break;
    case 0x83c6aau:
        result = Lufia2FieldApplyObjectParameters(memory, cpu);
        break;
    case 0x83dffdu:
        Lufia2ObjectLoadSecondaryScript(memory, cpu);
        result = ExecutionReturned(target);
        break;
    case 0x83f559u:
        result =
            Lufia2FieldRequestObjectAnimation(memory, cpu, RunObjectChild, context);
        break;
    default:
        service->handoff = target;
        return 0u;
    }
    if (result.flow != LUFIA2_EXECUTION_RETURNED) {
        if (!service->handoff)
            service->handoff = result.pc;
        return 0u;
    }
    uint16_t back = Pull8(memory, cpu);
    back |= (uint16_t)Pull8(memory, cpu) << 8;
    cpu->program_bank = frame == 3u ? Pull8(memory, cpu) : caller_bank;
    const uint32_t next = ((uint32_t)cpu->program_bank << 16) | (uint16_t)(back + 1u);
    if (next != site + frame + 1u) {
        service->handoff = next;
        return 0u;
    }
    return 1u;
}

static uint32_t CallObjectService(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  uint32_t target, uint16_t site, uint8_t frame) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, 0x80u, (uint16_t)(site + frame));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + frame));
    TextObjectService service = {memory, 0u};
    if (!RunObjectChild(&service, cpu, target, 0x800000u | site, frame))
        return service.handoff;
    return 0u;
}

static void FindObjectKind(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
        OpCmp(memory, cpu, OpDp(cpu, DP_OBJECT_KIND));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpCpx(cpu, OBJECT_COUNT);
    } while (!cpu->carry);
    if (cpu->x == OBJECT_COUNT)
        cpu->carry = 0u;
    SimulateRtsFrame(memory, cpu);
}

static uint32_t SetObjectParameters(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint16_t handler) {
    const bool motion =
        handler == SET_OBJECT_MOTION || handler == SET_OBJECT_MOTION_FLAG;
    uint16_t base = handler;
    if (motion) {
        SimulateJsrFrame(memory, cpu, (uint16_t)(handler + 2u));
        base = 0xaa67u;
    }
    Lufia2TextNextByte(memory, cpu, (uint16_t)(base + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_OBJECT_KIND));
    Lufia2TextNextByte(memory, cpu, (uint16_t)(base + 7u));
    OpSta(memory, cpu,
          OpDp(cpu, motion                             ? 0x66u
                    : handler == SET_OBJECT_HORIZONTAL ? 0x63u
                                                       : 0x64u));
    Lufia2TextNextByte(memory, cpu, (uint16_t)(base + 12u));
    OpSta(memory, cpu,
          OpDp(cpu, motion                             ? 0x65u
                    : handler == SET_OBJECT_HORIZONTAL ? 0x5au
                                                       : 0x5bu));
    if (motion) {
        OpStz(memory, cpu, OpDp(cpu, DP_PARAMETER_FLAGS_LOW));
        OpLoadA(cpu, 6u);
        OpSta(memory, cpu, OpDp(cpu, DP_PARAMETER_FLAGS_HIGH));
    } else {
        OpLoadA(cpu, handler == SET_OBJECT_HORIZONTAL ? 6u : 0x30u);
        OpSta(memory, cpu, OpDp(cpu, DP_PARAMETER_FLAGS_LOW));
        OpStz(memory, cpu, OpDp(cpu, DP_PARAMETER_FLAGS_HIGH));
    }
    PushY(memory, cpu);
    const uint16_t site = motion ? 0xaa7du : (uint16_t)(handler + 22u);
    const uint32_t handoff = CallObjectService(memory, cpu, 0x83c6aau, site, 3u);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    if (motion)
        SimulateRtsFrame(memory, cpu);
    if (handler == SET_OBJECT_MOTION_FLAG) {
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
            OpCmp(memory, cpu, OpDp(cpu, DP_OBJECT_KIND));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_OBJECT_UPDATES));
                OpOraValue(cpu, 4u);
                OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_RECOVERY_OBJECT_UPDATES));
            }
            OpInx(cpu);
            OpCpx(cpu, OBJECT_COUNT);
        } while (!cpu->zero);
    }
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t ResetMatchingObjects(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xaac1u);
    OpSta(memory, cpu, OpDp(cpu, DP_OBJECT_KIND));
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1559));
        OpCmp(memory, cpu, OpDp(cpu, DP_OBJECT_KIND));
        if (cpu->zero) {
            OpWriteX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT), cpu->x);
            uint32_t handoff = CallObjectService(memory, cpu, 0x8482d5u, 0xaad0u, 3u);
            if (handoff)
                return handoff;
            TransferDirectToA(cpu);
            OpLoadA(cpu, 6u);
            handoff = CallObjectService(memory, cpu, 0x83dffdu, 0xaad7u, 3u);
            if (handoff)
                return handoff;
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
        }
        OpInx(cpu);
        OpCpx(cpu, OBJECT_COUNT);
    } while (!cpu->zero);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t WaitObject(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t handler) {
    Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_OBJECT_KIND));
    if (handler == WAIT_OBJECT_PARAMETER) {
        Lufia2TextNextByte(memory, cpu, 0xaaedu);
        OpSta(memory, cpu, OpDp(cpu, DP_OBJECT_PARAMETER));
    }
    FindObjectKind(memory, cpu, handler == WAIT_OBJECT_PARAMETER ? 0xaaf2u : 0xab24u);
    if (!cpu->carry)
        return LUFIA2_TEXT_COMMAND_NEXT;
    if (handler == WAIT_OBJECT_PARAMETER) {
        OpLda(memory, cpu, OpDp(cpu, DP_OBJECT_PARAMETER));
        OpCmpValue(cpu, 0u);
        uint16_t field = 0x15d9u;
        if (!cpu->zero) {
            OpCmpValue(cpu, 1u);
            field = cpu->zero ? 0x1639u : 0x1699u;
        }
        OpLda(memory, cpu, OpAbsX(cpu, field));
        if (cpu->zero)
            return LUFIA2_TEXT_COMMAND_NEXT;
        Lufia2TextPrevByte(memory, cpu, 0xab10u);
        Lufia2TextPrevByte(memory, cpu, 0xab13u);
        Lufia2TextPrevByte(memory, cpu, 0xab16u);
    } else {
        Lufia2TextPrevByte(memory, cpu, 0xab2cu);
        Lufia2TextPrevByte(memory, cpu, 0xab2fu);
    }
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static uint32_t FaceActors(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t handler) {
    Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
    if (handler == FACE_ACTOR_GROUP) {
        const uint32_t handoff = CallObjectService(memory, cpu, 0x80ad39u, 0xad32u, 3u);
        return handoff ? handoff : LUFIA2_TEXT_COMMAND_NEXT;
    }
    OpSta(memory, cpu, OpDp(cpu, DP_SOURCE_ACTOR));
    Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 7u));
    OpSta(memory, cpu, OpDp(cpu, DP_TARGET_ACTOR));
    uint32_t handoff =
        CallObjectService(memory, cpu, 0x80ad57u, (uint16_t)(handler + 10u), 2u);
    if (handoff)
        return handoff;
    if (handler == FACE_ACTOR_PAIR) {
        OpLda(memory, cpu, OpDp(cpu, DP_SOURCE_ACTOR));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_TARGET_ACTOR));
        OpSta(memory, cpu, OpDp(cpu, DP_SOURCE_ACTOR));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_TARGET_ACTOR));
        handoff = CallObjectService(memory, cpu, 0x80ad57u, 0xad29u, 2u);
    }
    return handoff ? handoff : LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t MoveActorToScene(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xb023u);
    Lufia2EventFindActorId(memory, cpu, 0xb027u);
    Lufia2TextNextByte(memory, cpu, 0xb02au);
    const uint32_t handoff = Lufia2TextReadSceneCell(memory, cpu, 0xb02eu);
    if (handoff)
        return handoff;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, 0xfbu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120A));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_SCRATCH_A));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E120B));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_SCRATCH_A + 1u));
    PushY(memory, cpu);
    OpLoadA(cpu, 0x62u);
    uint32_t action_handoff;
    if (!Lufia2EventActorAction(memory, cpu, 0xb04fu, &action_handoff))
        return action_handoff;
    OpPullY(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t QueueObjectAnimation(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                     uint16_t handler) {
    Lufia2TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
    PushDataBank(memory, cpu);
    PushY(memory, cpu);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, handler == QUEUE_OBJECT_ANIMATION ? 0u : 0x40u);
    const uint32_t handoff =
        CallObjectService(memory, cpu, 0x83f559u, (uint16_t)(handler + 8u), 3u);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    PullDataBank(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

uint32_t Lufia2TextExecuteObjectCommand(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case SET_OBJECT_MOTION:
    case SET_OBJECT_MOTION_FLAG:
    case SET_OBJECT_HORIZONTAL:
    case SET_OBJECT_VERTICAL:
        return SetObjectParameters(memory, cpu, handler);
    case RESET_MATCHING_OBJECTS:
        return ResetMatchingObjects(memory, cpu);
    case WAIT_OBJECT_PARAMETER:
    case WAIT_OBJECT_PRESENT:
        return WaitObject(memory, cpu, handler);
    case FACE_ACTOR:
    case FACE_ACTOR_PAIR:
    case FACE_ACTOR_GROUP:
        return FaceActors(memory, cpu, handler);
    case MOVE_ACTOR_TO_SCENE:
        return MoveActorToScene(memory, cpu);
    case QUEUE_OBJECT_ANIMATION:
    case QUEUE_OBJECT_ANIMATION_40:
        return QueueObjectAnimation(memory, cpu, handler);
    default:
        return 0u;
    }
}
