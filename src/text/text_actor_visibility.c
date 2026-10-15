#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/battle.h"
#include "lufia2/field.h"
#include "lufia2/system.h"
#include "system/wram.h"
#include "text/text_internal.h"

enum {
    REPLACE_ACTOR_SPRITE = 0xa8f8u,
    REVEAL_ENCOUNTER_ACTOR = 0xaca7u,
    POPULATE_ENCOUNTER_ACTORS = 0xab51u,
    PLACE_ENCOUNTER_ACTOR = 0xbc55u,
    ENCOUNTER_ACTOR_LIST = 0x7ef600u,
    CAVE_ENCOUNTERS = 0x7ff966u,
    ROM_ENCOUNTER_SPRITES = 0x94ddf6u,
    DP_ENTITY_ID = 0x54u,
    CAVE_FLOOR_THEME = 0x7fe697u,
    CAVE_ENCOUNTER_THEME = 0x7ff8a0u,
    ROM_ENCOUNTER_THEMES = 0x80abcdu
};

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
    case 0x818351u:
        result =
            Lufia2FieldGenerateEncounter(memory, cpu, RunActorVisibilityChild, context);
        break;
    case 0x81839fu:
        result = Lufia2BattleGenerateEncounterRecord(memory, cpu,
                                                     RunActorVisibilityChild, context);
        break;
    case 0x8184fcu:
        result = Lufia2BattleAddEncounterEnemy(memory, cpu, RunActorVisibilityChild,
                                               context);
        break;
    case 0x808299u:
        Lufia2RandomScale(memory, cpu);
        result = ExecutionReturned(0x8082c6u);
        break;
    case 0x81fbc6u:
        result = Lufia2CharacterSpriteWord(memory, cpu);
        break;
    case 0x81fb8eu:
        result = Lufia2SpriteCoordinatesPacked(memory, cpu);
        break;
    case 0x81fb79u:
        result = Lufia2CharacterSpriteByte(memory, cpu);
        break;
    case 0x80c01du:
        result = Lufia2SceneReadActorAttributes(memory, cpu, RunActorVisibilityChild,
                                                context);
        break;
    case 0x80bfaau:
        result = Lufia2FieldFindHeaderRecord(memory, cpu);
        break;
    case 0x80c1a7u:
        result = Lufia2SceneApplyActorCell(memory, cpu);
        break;
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

static uint32_t PopulateEncounterActors(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS));
    OpBitValue(cpu, 1u);
    if (cpu->zero)
        return LUFIA2_TEXT_COMMAND_NEXT;
    OpLoadA(cpu, 0x91u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WINDOW_MODE));
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, ENCOUNTER_ACTOR_LIST));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero)
            break;
        uint32_t handoff = CallActorVisibilityChild(memory, cpu, 0x8482d5u, 0xab70u);
        if (handoff)
            return handoff;
        OpPushX(memory, cpu);
        OpLda(memory, cpu, OpLongX(cpu, ENCOUNTER_ACTOR_LIST));
        OpAdcValue(cpu, 0x4fu);
        OpSta(memory, cpu, OpDp(cpu, DP_ENTITY_ID));
        LoadA16(cpu, cpu->direct_page);
        OpLda(memory, cpu, OpLongX(cpu, ENCOUNTER_ACTOR_LIST));
        OpDecA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, CAVE_ENCOUNTERS));
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ROM_ENCOUNTER_SPRITES));
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7E05D2));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
        OpLda(memory, cpu, OpDp(cpu, DP_ENTITY_ID));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_ID));
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x4fu);
        PushAccumulator8(memory, cpu);
        handoff = CallActorVisibilityChild(memory, cpu, 0x80c01du, 0xaba0u);
        if (handoff)
            return handoff;
        handoff = CallActorVisibilityChild(memory, cpu, 0x80c1a7u, 0xaba4u);
        if (handoff)
            return handoff;
        LoadA8(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
        handoff = CallActorVisibilityChild(memory, cpu, 0x818351u, 0xabaau);
        if (handoff)
            return handoff;
        OpPullX(memory, cpu);
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT), 1);
    }
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, CAVE_FLOOR_THEME);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_ENCOUNTER_THEMES));
    OpSta(memory, cpu, CAVE_ENCOUNTER_THEME);
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    return LUFIA2_TEXT_COMMAND_YIELD;
}

static uint32_t PlaceEncounterActor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_RELOAD_FLAGS));
    OpBitValue(cpu, 0x10u);
    bool place = cpu->zero;
    if (!place) {
        OpBitValue(cpu, 8u);
        place = !cpu->zero;
        Lufia2TextNextByte(memory, cpu, place ? 0xbc74u : 0xbc62u);
        OpSta(memory, cpu, OpDp(cpu, DP_ENTITY_ID));
        if (place)
            Lufia2TextPrevByte(memory, cpu, 0xbc79u);
        else
            Lufia2TextNextByte(memory, cpu, 0xbc67u);
        OpLda(memory, cpu, OpDp(cpu, DP_ENTITY_ID));
        const uint32_t handoff =
            CallActorVisibilityChild(memory, cpu, 0x80bf92u, place ? 0xbc7fu : 0xbc6du);
        if (handoff)
            return handoff;
        if (!place && cpu->carry)
            return LUFIA2_TEXT_COMMAND_NEXT;
    }
    if (place && !Lufia2TextPlaceActor(memory, cpu, 0xbc82u))
        return 0x80bfbcu;
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x50u);
    PushY(memory, cpu);
    const uint32_t handoff = CallActorVisibilityChild(memory, cpu, 0x818351u, 0xbc91u);
    if (handoff)
        return handoff;
    OpPullY(memory, cpu);
    OpStepMem(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT), 1);
    return LUFIA2_TEXT_COMMAND_NEXT;
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
    case POPULATE_ENCOUNTER_ACTORS:
        return PopulateEncounterActors(memory, cpu);
    case PLACE_ENCOUNTER_ACTOR:
        return PlaceEncounterActor(memory, cpu);
    default:
        return 0u;
    }
}
