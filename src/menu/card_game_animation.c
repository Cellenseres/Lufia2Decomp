#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/card_game.h"
#include "system/wram.h"

enum {
    CARD_OAM = WRAM_FIELD_OAM_LOW_BUFFER,
    CARD_TILES = WRAM_CARD_GAME_TILE_BUFFER & 0xffffu,
    CARD_DECK = WRAM_CARD_GAME_DECK & 0xffffu,
    CARD_ANIMATION_CALL_LIMIT = 65536u
};

typedef struct CardAnimation {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} CardAnimation;

static bool AnimationReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static bool WordAnimationReady(const Lufia2CpuState *cpu) {
    return AnimationReady(cpu) && !cpu->accumulator_is_8_bit;
}

static bool AnimationCall(CardAnimation *animation, uint32_t site,
                          uint32_t target) {
    Lufia2CpuState *cpu = animation->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    if (animation->calls++ == CARD_ANIMATION_CALL_LIMIT) {
        animation->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(animation->memory, cpu, animation->child,
                            animation->context, site, target, 2u, 0x84u)) {
        animation->result = ExecutionReturned(site);
        animation->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!AnimationReady(cpu) ||
        cpu->accumulator_is_8_bit != byte_accumulator) {
        animation->result = ExecutionHandoff(cpu, site + 3u);
        return false;
    }
    return true;
}

static void SelectCardSprites(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint8_t scratch) {
    OpLda(memory, cpu, OpDp(cpu, 2u));
    cpu->carry = 0u;
    ExchangeAccumulatorBytes(cpu);
    RorA16(cpu);
    RorA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, scratch));
    RorA16(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, scratch));
    OpTax(cpu);
}

Lufia2ExecutionResult Lufia2CardGameAnimateOpeningCard(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84afb8u);
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84affbu), 0u};
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x13u)));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
    OpIny(cpu);
    OpIny(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
    OpIny(cpu);
    OpIny(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x11u));
    OpStz(memory, cpu, OpDp(cpu, 0x28u));
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 2u)));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
    OpIny(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x13u), cpu->y);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    if (!AnimationCall(&animation, 0x84afd3u, 0x84b2bcu) ||
        !AnimationCall(&animation, 0x84afd6u, 0x84b3c9u))
        return animation.result;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    if (!AnimationCall(&animation, 0x84afddu, 0x84a5a8u))
        return animation.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x11u)));
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    OpLoadA(cpu, 0x20u);
    if (!AnimationCall(&animation, 0x84afe6u, 0x84a607u))
        return animation.result;
    do {
        if (!AnimationCall(&animation, 0x84afe9u, 0x84b43bu))
            return animation.result;
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, 0x72u));
        if (!AnimationCall(&animation, 0x84aff0u, 0x84a5a8u))
            return animation.result;
        OpStepMem(memory, cpu, OpDp(cpu, 6u), -1);
    } while (!cpu->zero);
    OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84affbu);
}

Lufia2ExecutionResult Lufia2CardGameShowOpeningCard(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84affcu);
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84b026u), 0u};
    if (!AnimationCall(&animation, 0x84affcu, 0x84b22bu))
        return animation.result;
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x15u)));
    OpLda(memory, cpu, OpAbsY(cpu, CARD_DECK));
    OpIny(cpu);
    OpIny(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x15u), cpu->y);
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    OpStz(memory, cpu, OpDp(cpu, 0x28u));
    OpStepMem(memory, cpu, OpDp(cpu, 0x28u), 1);
    if (!AnimationCall(&animation, 0x84b00eu, 0x84b2bcu) ||
        !AnimationCall(&animation, 0x84b011u, 0x84b3c9u))
        return animation.result;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    if (!AnimationCall(&animation, 0x84b019u, 0x84a5a8u))
        return animation.result;
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0xd048u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
    return ExecutionReturned(0x84b026u);
}

Lufia2ExecutionResult Lufia2CardGameRevealCard(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b11eu);
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84b13cu), 0u};
    if (!AnimationCall(&animation, 0x84b11eu, 0x84b295u) ||
        !AnimationCall(&animation, 0x84b121u, 0x84b274u))
        return animation.result;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0x28u));
    if (!AnimationCall(&animation, 0x84b129u, 0x84b2bcu) ||
        !AnimationCall(&animation, 0x84b12cu, 0x84b387u))
        return animation.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!AnimationCall(&animation, 0x84b137u, 0x84a5a8u))
        return animation.result;
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84b13cu);
}

static Lufia2ExecutionResult ContinueReposition(CardAnimation *animation,
                                                 bool completed_frame) {
    const Lufia2Memory *memory = animation->memory;
    Lufia2CpuState *cpu = animation->cpu;
    for (;;) {
        if (completed_frame) {
            OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
            if (cpu->negative)
                break;
        }
        if (!AnimationCall(animation, 0x84b157u, 0x84b43bu))
            return animation->result;
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, 0x72u));
        if (!AnimationCall(animation, 0x84b15eu, 0x84a5a8u))
            return animation->result;
        completed_frame = true;
    }
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x0bu)));
    AnimationCall(animation, 0x84b169u, 0x84b16du);
    return animation->result;
}

Lufia2ExecutionResult Lufia2CardGameAnimateReposition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b13du);
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84b16cu), 0u};
    OpLda(memory, cpu, OpDp(cpu, 2u));
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    RorA16(cpu);
    RorA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    RorA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 6u));
    OpSta(memory, cpu, OpDp(cpu, 0x0bu));
    OpTax(cpu);
    if (!AnimationCall(&animation, 0x84b14bu, 0x84b16du))
        return animation.result;
    OpStz(memory, cpu, OpDp(cpu, 2u));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    if (!AnimationCall(&animation, 0x84b154u, 0x84a607u))
        return animation.result;
    return ContinueReposition(&animation, false);
}

Lufia2ExecutionResult Lufia2CardGameResumeReposition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !AnimationReady(cpu) || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84b161u);
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84b16cu), 0u};
    return ContinueReposition(&animation, true);
}

Lufia2ExecutionResult Lufia2CardGameSwapSpriteBlocks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b16du);
    OpLdy(cpu, 0u);
    OpLoadA(cpu, 47u);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, CARD_OAM));
        OpSta(memory, cpu, OpDp(cpu, 2u));
        OpLda(memory, cpu, OpAbsX(cpu, CARD_OAM));
        OpSta(memory, cpu, OpAbsY(cpu, CARD_OAM));
        OpLda(memory, cpu, OpDp(cpu, 2u));
        OpSta(memory, cpu, OpAbsX(cpu, CARD_OAM));
        OpIny(cpu);
        OpIny(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 6u), -1);
    } while (!cpu->negative);
    return ExecutionReturned(0x84b18du);
}

static Lufia2ExecutionResult ContinueCardTurn(CardAnimation *animation,
                                               uint32_t landing) {
    const Lufia2Memory *memory = animation->memory;
    Lufia2CpuState *cpu = animation->cpu;
    if (landing == 0x84b237u || landing == 0x84b241u) {
        for (;;) {
            if (landing == 0x84b241u) {
                OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
                if (cpu->zero)
                    break;
            }
            if (!AnimationCall(animation, 0x84b237u, 0x84b43bu))
                return animation->result;
            OpLoadA(cpu, 0x80u);
            OpSta(memory, cpu, OpDp(cpu, 0x72u));
            if (!AnimationCall(animation, 0x84b23eu, 0x84a5a8u))
                return animation->result;
            landing = 0x84b241u;
        }
        OpLoadA(cpu, 0x20u);
        if (!AnimationCall(animation, 0x84b247u, 0x84a607u))
            return animation->result;
        OpLdx(cpu, 0xb663u);
        OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
        if (!AnimationCall(animation, 0x84b24fu, 0x84b472u))
            return animation->result;
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, 0x72u));
        if (!AnimationCall(animation, 0x84b256u, 0x84a5a8u))
            return animation->result;
        landing = 0x84b259u;
    }
    if (landing == 0x84b259u) {
        if (!AnimationCall(animation, 0x84b259u, 0x84a5a8u))
            return animation->result;
        landing = 0x84b25cu;
    }
    if (landing == 0x84b25cu) {
        if (!AnimationCall(animation, 0x84b25cu, 0x84a5a8u))
            return animation->result;
        landing = 0x84b25fu;
    }
    if (landing == 0x84b25fu) {
        OpLdx(cpu, 0xb693u);
        OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
        if (!AnimationCall(animation, 0x84b264u, 0x84b472u))
            return animation->result;
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, 0x72u));
        if (!AnimationCall(animation, 0x84b26bu, 0x84a5a8u))
            return animation->result;
        landing = 0x84b26eu;
    }
    if (landing == 0x84b26eu &&
        !AnimationCall(animation, 0x84b26eu, 0x84a5a8u))
        return animation->result;
    OpRepWidths(cpu, 0x20u);
    return animation->result;
}

Lufia2ExecutionResult Lufia2CardGameTurnCardSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b22bu);
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84b273u), 0u};
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 2u);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    OpLdx(cpu, 4u);
    OpWriteX(memory, cpu, OpDp(cpu, 4u), cpu->x);
    return ContinueCardTurn(&animation, 0x84b237u);
}

Lufia2ExecutionResult Lufia2CardGameResumeCardTurn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t landing) {
    if (!child || !AnimationReady(cpu) || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, landing);
    switch (landing) {
    case 0x84b241u: case 0x84b259u: case 0x84b25cu:
    case 0x84b25fu: case 0x84b26eu: case 0x84b271u:
        break;
    default:
        return ExecutionHandoff(cpu, landing);
    }
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84b273u), 0u};
    return ContinueCardTurn(&animation, landing);
}

Lufia2ExecutionResult Lufia2CardGameHideCardSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b274u);
    SelectCardSprites(memory, cpu, 4u);
    OpLdy(cpu, 23u);
    do {
        OpLoadA(cpu, 0xf000u);
        OpSta(memory, cpu, OpAbsX(cpu, CARD_OAM));
        OpInx(cpu);
        OpInx(cpu);
        OpStz(memory, cpu, OpAbsX(cpu, CARD_OAM));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x84b294u);
}

Lufia2ExecutionResult Lufia2CardGameLocateCardTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b295u);
    SelectCardSprites(memory, cpu, 4u);
    OpLda(memory, cpu, OpAbsX(cpu, CARD_OAM));
    OpSepWidths(cpu, 0x20u);
    RorA8(cpu);
    RorA8(cpu);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpStz(memory, cpu, OpDp(cpu, 5u));
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0u;
    RolA16(cpu);
    RolA16(cpu);
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 4u));
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    return ExecutionReturned(0x84b2bbu);
}

static void FillCardFaceBlock(const Lufia2Memory *memory,
                              Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x2au));
    OpSta(memory, cpu, OpAbsX(cpu, CARD_TILES));
    OpIncA(cpu);
    OpInx(cpu);
    OpInx(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_TILES));
    OpTay(cpu);
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 6u);
    OpTax(cpu);
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 15u);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_TILES));
    OpIncA(cpu);
    OpInx(cpu);
    OpInx(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_TILES));
}

static void FillCardRankTiles(const Lufia2Memory *memory,
                              Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    OpAndValue(cpu, 0xffu);
    RolA16(cpu);
    OpAdcValue(cpu, 0xffffu);
    OpLdx(cpu, 0u);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_TILES));
    OpIncA(cpu);
    OpTay(cpu);
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 8u);
    OpTax(cpu);
    OpTya(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_TILES));
    OpTxa(cpu);
    OpAdcValue(cpu, 30u);
    OpTax(cpu);
    OpTya(cpu);
    OpOraValue(cpu, 0xc000u);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_TILES));
    OpDecA(cpu);
    OpTay(cpu);
    OpTxa(cpu);
    OpAdcValue(cpu, 8u);
    OpTax(cpu);
    OpTya(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_TILES));
}

Lufia2ExecutionResult Lufia2CardGameBuildCardTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!AnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b2bcu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x2cu));
    OpLdx(cpu, 0xb603u);
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, 0x28u), -1);
    if (cpu->zero)
        OpLdx(cpu, 0xb633u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x2au), cpu->x);
    OpLoadA(cpu, 0u);
    OpTax(cpu);
    OpTay(cpu);
    OpLoadA(cpu, 23u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x2au));
        OpSta(memory, cpu, OpAbsX(cpu, CARD_TILES));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->negative);
    OpStepMem(memory, cpu, OpDp(cpu, 0x28u), 1);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x23u));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        RolA16(cpu);
        OpAdcValue(cpu, 0xb6c3u);
        OpSta(memory, cpu, OpDp(cpu, 0x2au));
        OpLdy(cpu, 0u);
        OpLdx(cpu, 10u);
        FillCardFaceBlock(memory, cpu);
        FillCardRankTiles(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, 0x22u));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        RolA16(cpu);
        OpAdcValue(cpu, 0xb6c9u);
        OpSta(memory, cpu, OpDp(cpu, 0x2au));
        OpLdy(cpu, 0u);
        OpLdx(cpu, 26u);
        FillCardFaceBlock(memory, cpu);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84b386u);
}

Lufia2ExecutionResult Lufia2CardGameDrawCardTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b387u);
    OpStepMem(memory, cpu, OpDp(cpu, 0x28u), -1);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x23u));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 4u);
        ExchangeAccumulatorBytes(cpu);
        cpu->carry = 0u;
        RolA16(cpu);
        RolA16(cpu);
    } else {
        OpLoadA(cpu, 0x400u);
    }
    OpSta(memory, cpu, OpDp(cpu, 0x28u));
    OpLdy(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x24u)));
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    do {
        OpLoadA(cpu, 3u);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        do {
            OpLda(memory, cpu, OpAbsY(cpu, CARD_TILES));
            OpOra(memory, cpu, OpDp(cpu, 0x28u));
            OpSta(memory, cpu, OpAbsX(cpu, 0x2000u));
            OpIny(cpu);
            OpIny(cpu);
            OpInx(cpu);
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
        } while (!cpu->negative);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 56u);
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->negative);
    return ExecutionReturned(0x84b3c8u);
}

Lufia2ExecutionResult Lufia2CardGameEmitCardSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !AnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b3c9u);
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84b428u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpStepMem(memory, cpu, OpDp(cpu, 0x28u), -1);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x23u));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 4u);
        ExchangeAccumulatorBytes(cpu);
        cpu->carry = 0u;
        RolA16(cpu);
    } else {
        OpLoadA(cpu, 0x200u);
    }
    OpOraValue(cpu, 0x3000u);
    OpSta(memory, cpu, OpDp(cpu, 0x28u));
    OpLdy(cpu, 0u);
    SelectCardSprites(memory, cpu, 0u);
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    do {
        static const uint32_t sites[] = {
            0x84b3fdu, 0x84b406u, 0x84b40fu, 0x84b418u
        };
        for (unsigned column = 0u; column < 4u; ++column) {
            if (column) {
                OpLda(memory, cpu, OpDp(cpu, 0x24u));
                cpu->carry = 0u;
                OpAdcValue(cpu, (uint16_t)(column * 8u));
            }
            if (!AnimationCall(&animation, sites[column], 0x84b429u))
                return animation.result;
        }
        OpLda(memory, cpu, OpDp(cpu, 0x24u));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x800u);
        OpSta(memory, cpu, OpDp(cpu, 0x24u));
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84b428u);
}

Lufia2ExecutionResult Lufia2CardGameEmitCardSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b429u);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_OAM));
    OpLda(memory, cpu, OpAbsY(cpu, CARD_TILES));
    OpOra(memory, cpu, OpDp(cpu, 0x28u));
    OpIny(cpu);
    OpIny(cpu);
    OpInx(cpu);
    OpInx(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_OAM));
    OpInx(cpu);
    OpInx(cpu);
    return ExecutionReturned(0x84b43au);
}

Lufia2ExecutionResult Lufia2CardGameMoveCardSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !AnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b43bu);
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84b45au), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpTay(cpu);
    SelectCardSprites(memory, cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->y);
    OpLdy(cpu, 23u);
    do {
        if (!AnimationCall(&animation, 0x84b453u, 0x84b45bu))
            return animation.result;
        OpDey(cpu);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84b45au);
}

Lufia2ExecutionResult Lufia2CardGameMoveCardSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b45bu);
    OpLda(memory, cpu, OpAbsX(cpu, CARD_OAM));
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 1u));
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsX(cpu, CARD_OAM));
    for (unsigned byte = 0u; byte < 4u; ++byte)
        OpInx(cpu);
    return ExecutionReturned(0x84b471u);
}

Lufia2ExecutionResult Lufia2CardGameRetileCardSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !AnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b472u);
    CardAnimation animation = {memory, cpu, child, context,
                              ExecutionReturned(0x84b49au), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x0au));
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    SelectCardSprites(memory, cpu, 4u);
    OpInx(cpu);
    OpInx(cpu);
    OpLoadA(cpu, 23u);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    do {
        if (!AnimationCall(&animation, 0x84b492u, 0x84b49bu))
            return animation.result;
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84b49au);
}

Lufia2ExecutionResult Lufia2CardGameRetileCardSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WordAnimationReady(cpu))
        return ExecutionHandoff(cpu, 0x84b49bu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 8u));
    OpSta(memory, cpu, OpAbsX(cpu, CARD_OAM));
    for (unsigned byte = 0u; byte < 4u; ++byte)
        OpInx(cpu);
    OpIny(cpu);
    OpIny(cpu);
    return ExecutionReturned(0x84b4a6u);
}
