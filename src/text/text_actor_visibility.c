#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum { REPLACE_ACTOR_SPRITE = 0xa8f8u, REVEAL_ENCOUNTER_ACTOR = 0xaca7u };

typedef struct ActorVisibilityService {
    const Lufia2Memory *memory;
    uint32_t handoff;
} ActorVisibilityService;

static uint8_t RunActorVisibilityChild(void *context, Lufia2CpuState *cpu,
                                       uint32_t target, uint32_t site, uint8_t frame) {
    ActorVisibilityService *service = context;
    const Lufia2Memory *memory = service->memory;
    const uint8_t caller_bank = cpu->program_bank;
    cpu->program_bank = (uint8_t)(target >> 16);
    Lufia2ExecutionResult result;
    switch (target) {
    case 0x80bf6fu:
        result = Lufia2FieldFindWindowActor(memory, cpu);
        break;
    case 0x80bf92u:
        result =
            Lufia2FieldSelectActorById(memory, cpu, RunActorVisibilityChild, context);
        break;
    case 0x8482d5u:
        result = Lufia2ActorPreparePartyOffsets(memory, cpu);
        break;
    case 0x83aaafu:
        result = Lufia2ActorReleaseSceneSprite(memory, cpu, RunActorVisibilityChild,
                                               context);
        break;
    case 0x83abccu:
        result = Lufia2SpriteReleaseAllocation(memory, cpu);
        break;
    case 0x83fa12u:
        result = Lufia2ActorClearSceneOccupancy(memory, cpu, RunActorVisibilityChild,
                                                context);
        break;
    case 0x83f9b6u:
        result = Lufia2MapPackedAttributeCell(memory, cpu);
        break;
    case 0x83a9bau:
        result = Lufia2ActorLoadSprite(memory, cpu);
        break;
    case 0x83d416u:
        Lufia2ActorLoadPrimaryScript(memory, cpu);
        result = ExecutionReturned(0x83d436u);
        break;
    case 0x83aa30u:
        result = Lufia2ActorSetSpriteHeightOffset(memory, cpu, RunActorVisibilityChild,
                                                  context);
        break;
    case 0x83fafau:
        result = Lufia2ActorExtendSignedByte(memory, cpu);
        break;
    case 0x8ebaf6u:
        result = Lufia2FieldRevealActor(memory, cpu, RunActorVisibilityChild, context);
        break;
    case 0x8ebae7u:
        result = Lufia2FieldMarkEncounterActor(memory, cpu);
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

static uint32_t CallActorVisibilityChild(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu, uint32_t target,
                                         uint16_t back) {
    SimulateJslFrame(memory, cpu, 0x80u, back);
    ActorVisibilityService service = {memory, 0u};
    RunActorVisibilityChild(&service, cpu, target, 0x800000u | (uint16_t)(back - 3u),
                            3u);
    return service.handoff;
}

static uint32_t ReplaceActorSprite(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xa8fau);
    uint32_t handoff = CallActorVisibilityChild(memory, cpu, 0x80bf6fu, 0xa8feu);
    if (handoff)
        return handoff;
    Lufia2TextNextByte(memory, cpu, 0xa901u);
    PushY(memory, cpu);
    PushAccumulator8(memory, cpu);
    handoff = CallActorVisibilityChild(memory, cpu, 0x83aaafu, 0xa907u);
    if (handoff)
        return handoff;
    LoadA8(cpu, Pull8(memory, cpu));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
    handoff = CallActorVisibilityChild(memory, cpu, 0x83a9bau, 0xa911u);
    if (handoff)
        return handoff;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpBitValue(cpu, 8u);
    if (cpu->zero) {
        handoff = CallActorVisibilityChild(memory, cpu, 0x83d416u, 0xa91eu);
        if (handoff)
            return handoff;
    }
    handoff = CallActorVisibilityChild(memory, cpu, 0x83aa30u, 0xa922u);
    if (handoff)
        return handoff;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, 0xfbu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, WRAM_ACTOR_SCENE_MOTION_OFFSET);
    OpPullY(memory, cpu);
    return LUFIA2_TEXT_COMMAND_NEXT;
}

static uint32_t RevealEncounterActor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2TextNextByte(memory, cpu, 0xaca9u);
    uint32_t handoff = CallActorVisibilityChild(memory, cpu, 0x80bf92u, 0xacadu);
    if (handoff)
        return handoff;
    if (cpu->carry)
        return LUFIA2_TEXT_COMMAND_NEXT;
    handoff = CallActorVisibilityChild(memory, cpu, 0x8ebaf6u, 0xacb3u);
    if (handoff)
        return handoff;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    handoff = CallActorVisibilityChild(memory, cpu, 0x8ebae7u, 0xacb9u);
    return handoff ? handoff : LUFIA2_TEXT_COMMAND_NEXT;
}

uint32_t Lufia2TextExecuteActorVisibility(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu, uint16_t handler) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return 0u;
    switch (handler) {
    case REPLACE_ACTOR_SPRITE:
        return ReplaceActorSprite(memory, cpu);
    case REVEAL_ENCOUNTER_ACTOR:
        return RevealEncounterActor(memory, cpu);
    default:
        return 0u;
    }
}
