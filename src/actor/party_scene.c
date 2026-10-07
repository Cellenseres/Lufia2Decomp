#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    PARTY_ACTOR_SLOT = 0xa7u,
    PARTY_WORD_OFFSET = 0xa9u,
    PARTY_RECORD_OFFSET = 0xabu,
    PARTY_SPRITE = 0x54u,
    PARTY_SPRITE_BANK = 0x55u,
    PARTY_IDS = WRAM_SCENE_PARTY_IDS,
    PARTY_HIDDEN_SPRITE = WRAM_UNK_7FD0FE,
    PARTY_SINGLE_ACTOR = WRAM_WINDOW_MODE,
    PARTY_WALK_SCRIPTS = 0x8482d0u
};

static Lufia2ExecutionResult PartySceneUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2ActorPreparePartyOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8482d5u);
    OpStz(memory, cpu, OpDp(cpu, PARTY_ACTOR_SLOT + 1u));
    OpLda(memory, cpu, OpDp(cpu, PARTY_ACTOR_SLOT));
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, PARTY_WORD_OFFSET));
    OpStz(memory, cpu, OpDp(cpu, PARTY_WORD_OFFSET + 1u));
    OpAdc(memory, cpu, OpDp(cpu, PARTY_ACTOR_SLOT));
    OpSta(memory, cpu, OpDp(cpu, PARTY_RECORD_OFFSET));
    OpStz(memory, cpu, OpDp(cpu, PARTY_RECORD_OFFSET + 1u));
    OpStz(memory, cpu, OpDp(cpu, PARTY_RECORD_OFFSET + 2u));
    return ExecutionReturned(0x8482e6u);
}

Lufia2ExecutionResult Lufia2ActorClearSceneOccupancy(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83fa12u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83fa1bu, 0x83f9b6u, 2u, 0x83u))
        return PartySceneUnwound(0x83fa1bu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7e4000u));
    OpAndValue(cpu, 0xfeu);
    OpSta(memory, cpu, OpLongX(cpu, 0x7e4000u));
    PushIndex(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpCmpValue(cpu, 2u);
    OpPullX(memory, cpu);
    if (cpu->carry) {
        OpLda(memory, cpu, OpLongX(cpu, 0x7e4001u));
        OpAndValue(cpu, 0xfeu);
        OpSta(memory, cpu, OpLongX(cpu, 0x7e4001u));
    }
    return ExecutionReturned(0x83fa3eu);
}

Lufia2ExecutionResult Lufia2ActorReleaseSceneSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83aaafu);
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpOraValue(cpu, 4u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE25E));
    OpSta(memory, cpu, OpDp(cpu, PARTY_SPRITE));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE2A6));
    OpSta(memory, cpu, OpDp(cpu, PARTY_SPRITE_BANK));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x83abf4u));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83aad6u, 0x83abccu, 3u, 0x83u))
        return PartySceneUnwound(0x83aad6u);
    SetIndexWidth(cpu, 0u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83aadcu, 0x83fa12u, 3u, 0x83u))
        return PartySceneUnwound(0x83aadcu);
    return ExecutionReturned(0x83aae0u);
}

Lufia2ExecutionResult Lufia2ActorUploadSceneSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83aae1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83aae1u, 0x83aae5u, 2u, 0x83u))
        return PartySceneUnwound(0x83aae1u);
    return ExecutionReturned(0x83aae4u);
}

static uint32_t PartyInitializeSceneActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_ACTOR_ID));
    OpSta(memory, cpu, OpDp(cpu, PARTY_SPRITE));
    if (cpu->index_is_8_bit)
        Write8(memory, DirectAddress(cpu, PARTY_ACTOR_SLOT), (uint8_t)cpu->y);
    else
        Write16Direct(memory, cpu, PARTY_ACTOR_SLOT, cpu->y);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x848231u, 0x8482d5u, 3u, 0x84u))
        return 0x848231u;
    LoadA16(cpu, cpu->direct_page);
    OpTya(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X + 1u));
    OpLda(memory, cpu, PARTY_HIDDEN_SPRITE);
    if (!cpu->zero) {
        OpLoadA(cpu, 0xf8u);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X));
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X + 1u));
        OpLda(memory, cpu, PARTY_HIDDEN_SPRITE);
    } else {
        OpLda(memory, cpu, OpDp(cpu, PARTY_SPRITE));
    }
    const struct { uint32_t site, target; } presentation[] = {
        {0x84825cu, 0x83a9bau}, {0x848260u, 0x83d416u},
        {0x848264u, 0x83aa30u}
    };
    for (unsigned step = 0u; step < 3u; ++step)
        if (!CallChildWithFrame(memory, cpu, child, context,
                presentation[step].site, presentation[step].target, 3u, 0x84u))
            return presentation[step].site;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FACING));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE4DE));
    OpStz(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    const uint16_t leader_fields[] = {
        WRAM_UNK_7E066A, WRAM_ACTOR_TILE_X, WRAM_ACTOR_TILE_Y
    };
    for (unsigned field = 0u; field < 3u; ++field) {
        OpLda(memory, cpu, OpAbs(cpu, leader_fields[field]));
        OpSta(memory, cpu, OpAbsX(cpu, leader_fields[field]));
    }
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x84828bu, 0x83a746u, 3u, 0x84u))
        return 0x84828bu;
    SetIndexWidth(cpu, 0u);
    OpLoadA(cpu, 0x20u);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_ACTOR_SLOT)));
    if (!cpu->zero)
        OpLoadA(cpu, 0x22u);
    OpAndValue(cpu, 0xfbu);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_ACTOR_STATE));
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_ACTOR_FLAGS));
    OpTyx(cpu);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE316));
    OpLda(memory, cpu, OpLongX(cpu, PARTY_WALK_SCRIPTS));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E070A));
    OpTya(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8482b5u, 0x83d416u, 3u, 0x84u))
        return 0x8482b5u;
    return 0u;
}

Lufia2ExecutionResult Lufia2FieldRebuildPartyActors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x84u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x848204u);
    OpLdy(cpu, 5u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E05D2));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            if (cpu->index_is_8_bit)
                Write8(memory, DirectAddress(cpu, PARTY_ACTOR_SLOT), (uint8_t)cpu->y);
            else
                Write16Direct(memory, cpu, PARTY_ACTOR_SLOT, cpu->y);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x848210u, 0x83aaafu, 3u, 0x84u))
                return PartySceneUnwound(0x848210u);
            OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTY_ACTOR_SLOT)));
        }
        OpDey(cpu);
    } while (!cpu->negative);
    OpLdy(cpu, 0u);
    for (;;) {
        LoadA16(cpu, cpu->direct_page);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_FOLLOW_SLOTS));
        OpLda(memory, cpu, OpAbsY(cpu, PARTY_IDS));
        if (!cpu->negative) {
            uint32_t site = PartyInitializeSceneActor(memory, cpu, child, context);
            if (site)
                return PartySceneUnwound(site);
        }
        OpLda(memory, cpu, OpAbs(cpu, PARTY_SINGLE_ACTOR));
        OpBitValue(cpu, 1u);
        if (!cpu->zero)
            break;
        OpLda(memory, cpu, PARTY_HIDDEN_SPRITE);
        if (!cpu->zero)
            break;
        OpIny(cpu);
        OpCpy(cpu, 5u);
        if (cpu->zero)
            break;
    }
    return ExecutionReturned(0x8482cfu);
}
