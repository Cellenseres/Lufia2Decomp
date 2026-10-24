#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/card_game.h"
#include "system/wram.h"

enum {
    CARD_DECK = WRAM_CARD_GAME_DECK & 0xffffu,
    CARD_HANDS = WRAM_CARD_GAME_HANDS & 0xffffu,
    CARD_HAND_COUNTS = WRAM_CARD_GAME_HAND_COUNTS & 0xffffu,
    CARD_HAND_TOTALS = WRAM_CARD_GAME_HAND_TOTALS & 0xffffu,
    CARD_SCORE_WORK = WRAM_CARD_GAME_SCORE_WORK & 0xffffu,
    CARD_RANK_WORK = WRAM_CARD_GAME_RANK_WORK & 0xffffu,
    CARD_GAME_ROUND = WRAM_CARD_GAME_ROUND & 0xffffu,
    CARD_HAND_INDEX = 0x02u,
    CARD_DECK_POSITION = 0x15u,
    CARD_ACTIVE_HAND = 0x17u
};

static bool CardGameReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2CardGameBuildDeck(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CardGameReady(cpu))
        return ExecutionHandoff(cpu, 0x84b4a7u);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0u);
    OpTax(cpu);
    do {
        OpIncA(cpu);
        OpLdy(cpu, 12u);
        do {
            OpSta(memory, cpu, OpAbsX(cpu, CARD_DECK));
            OpInx(cpu);
            OpInx(cpu);
            OpIncA(cpu);
            OpDey(cpu);
        } while (!cpu->negative);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x0100u);
        OpAndValue(cpu, 0xff00u);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->negative);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_GAME_ROUND));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x84b4d8u);
}

static bool DrawShuffleIndex(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    Lufia2ExecutionResult *result) {
    OpLoadA(cpu, 52u);
    if (!CallChildWithFrame(memory, cpu, child, context, site, 0x808299u, 3u, 0x84u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!CardGameReady(cpu)) {
        *result = ExecutionHandoff(cpu, site + 4u);
        return false;
    }
    cpu->carry = 0u;
    RolA16(cpu);
    return true;
}

Lufia2ExecutionResult Lufia2CardGameShuffleDeck(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardGameReady(cpu))
        return ExecutionHandoff(cpu, 0x84b4d9u);
    Lufia2ExecutionResult result = ExecutionReturned(0x84b522u);
    OpLoadA(cpu, 0x0200u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    do {
        if (!DrawShuffleIndex(memory, cpu, child, context, 0x84b4e1u, &result))
            return result;
        OpTax(cpu);
        if (!DrawShuffleIndex(memory, cpu, child, context, 0x84b4ebu, &result))
            return result;
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, CARD_DECK));
        OpSta(memory, cpu, OpDp(cpu, 0x02u));
        OpLda(memory, cpu, OpAbsY(cpu, CARD_DECK));
        OpSta(memory, cpu, OpAbsX(cpu, CARD_DECK));
        OpLda(memory, cpu, OpDp(cpu, 0x02u));
        OpSta(memory, cpu, OpAbsY(cpu, CARD_DECK));
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->negative);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 3u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, CARD_HAND_COUNTS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 39u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, CARD_HANDS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    return result;
}

Lufia2ExecutionResult Lufia2CardGameDealCard(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CardGameReady(cpu))
        return ExecutionHandoff(cpu, 0x84af1cu);
    OpLda(memory, cpu, OpDp(cpu, CARD_ACTIVE_HAND));
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    OpTax(cpu);
    RolA16(cpu);
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x22u));
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    OpLda(memory, cpu, OpAbsX(cpu, CARD_HAND_COUNTS));
    cpu->carry = 0u;
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x22u));
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    OpStepMem(memory, cpu, OpAbsX(cpu, CARD_HAND_COUNTS), 1);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, CARD_DECK_POSITION)));
    OpLda(memory, cpu, OpAbsY(cpu, CARD_DECK));
    OpIny(cpu);
    OpIny(cpu);
    OpCpy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x0104u)));
    if (!cpu->zero)
        OpWriteX(memory, cpu, OpDp(cpu, CARD_DECK_POSITION), cpu->y);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x22u)));
    OpSta(memory, cpu, OpAbsY(cpu, CARD_HANDS));
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    return ExecutionReturned(0x84af4au);
}

static void SortScoreCards(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0u);
    do {
        OpTya(cpu);
        OpIncA(cpu);
        OpTax(cpu);
        do {
            OpLda(memory, cpu, OpAbsY(cpu, CARD_SCORE_WORK));
            OpCmp(memory, cpu, OpAbsX(cpu, CARD_SCORE_WORK));
            if (cpu->negative) {
                OpSta(memory, cpu, OpDp(cpu, 0u));
                OpLda(memory, cpu, OpAbsX(cpu, CARD_SCORE_WORK));
                OpSta(memory, cpu, OpAbsY(cpu, CARD_SCORE_WORK));
                OpLda(memory, cpu, OpDp(cpu, 0u));
                OpSta(memory, cpu, OpAbsX(cpu, CARD_SCORE_WORK));
            }
            OpInx(cpu);
            OpCpx(cpu, 5u);
        } while (!cpu->zero);
        OpIny(cpu);
        OpCpy(cpu, 4u);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2CardGameScoreHand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CardGameReady(cpu))
        return ExecutionHandoff(cpu, 0x84ae5eu);
    OpLda(memory, cpu, OpDp(cpu, CARD_HAND_INDEX));
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    RolA16(cpu);
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 4u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CARD_HANDS));
        OpCmpValue(cpu, 10u);
        if (!cpu->negative)
            OpLoadA(cpu, 10u);
        OpSta(memory, cpu, OpAbsY(cpu, CARD_SCORE_WORK));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    SortScoreCards(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, 1u));
    OpStz(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0u);
    OpLdy(cpu, 4u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CARD_SCORE_WORK));
        OpStepMem(memory, cpu, OpAbsX(cpu, CARD_SCORE_WORK), -1);
        if (cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, 0u));
            OpCmpValue(cpu, 11u);
            if (cpu->negative) {
                OpLoadA(cpu, 11u);
            } else {
                OpStepMem(memory, cpu, OpAbsX(cpu, CARD_SCORE_WORK - 1u), -1);
                if (cpu->negative) {
                    OpCmpValue(cpu, 21u);
                    if (cpu->zero) {
                        cpu->carry = 1u;
                        OpSbcValue(cpu, 10u);
                        OpSta(memory, cpu, OpDp(cpu, 0u));
                    }
                }
                OpStepMem(memory, cpu, OpAbsX(cpu, CARD_SCORE_WORK), 1);
                OpLoadA(cpu, 1u);
            }
        }
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, 0u));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84aee0u);
}

static void SortRankCards(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0u);
    do {
        OpTya(cpu);
        OpIncA(cpu);
        OpIncA(cpu);
        OpTax(cpu);
        do {
            OpLda(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
            OpCmp(memory, cpu, OpAbsX(cpu, CARD_RANK_WORK));
            if (cpu->negative) {
                OpSta(memory, cpu, OpDp(cpu, 0u));
                OpLda(memory, cpu, OpAbsX(cpu, CARD_RANK_WORK));
                OpSta(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
                OpLda(memory, cpu, OpDp(cpu, 0u));
                OpSta(memory, cpu, OpAbsX(cpu, CARD_RANK_WORK));
            }
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, 10u);
        } while (!cpu->zero);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 8u);
    } while (!cpu->zero);
}

static unsigned RankThreeCards(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, 4u), cpu->y);
    OpLda(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
    OpAndValue(cpu, 0x00ffu);
    OpCmp(memory, cpu, OpDp(cpu, 0u));
    bool same_rank = cpu->zero != 0u;
    if (same_rank) {
        OpIny(cpu);
        OpIny(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
        OpAndValue(cpu, 0x00ffu);
        OpCmp(memory, cpu, OpDp(cpu, 0u));
        same_rank = cpu->zero != 0u;
    }
    OpLda(memory, cpu, OpAbsX(cpu, CARD_HAND_TOTALS));
    OpCmpValue(cpu, 21u);
    if (same_rank)
        return cpu->zero ? 5u : 3u;
    if (!cpu->zero)
        return 0u;
    OpLdy(cpu, 0u);
    OpLda(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
    OpAndValue(cpu, 0xff00u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    for (unsigned card = 0u; card < 2u; ++card) {
        OpIny(cpu);
        OpIny(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
        OpAndValue(cpu, 0xff00u);
        OpCmp(memory, cpu, OpDp(cpu, 0u));
        if (!cpu->zero)
            return 0u;
    }
    OpLdy(cpu, 0u);
    OpLda(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
    OpDecA(cpu);
    OpIny(cpu);
    OpIny(cpu);
    OpCmp(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
    return cpu->zero ? 4u : 0u;
}

Lufia2ExecutionResult Lufia2CardGameRankHand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CardGameReady(cpu))
        return ExecutionHandoff(cpu, 0x84ab61u);
    OpLda(memory, cpu, OpDp(cpu, CARD_HAND_INDEX));
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    RolA16(cpu);
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 6u));
    OpLdy(cpu, 8u);
    OpTax(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CARD_HANDS));
        OpSta(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    SortRankCards(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 6u)));
    OpStz(memory, cpu, OpDp(cpu, 6u));
    OpLoadA(cpu, 21u);
    OpCmp(memory, cpu, OpAbsX(cpu, CARD_HAND_TOTALS));
    unsigned rank = 0u;
    if (!cpu->negative) {
        OpLda(memory, cpu, OpAbsX(cpu, CARD_HAND_COUNTS));
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, CARD_HAND_TOTALS));
            OpCmpValue(cpu, 21u);
            if (cpu->zero) {
                OpLdy(cpu, 0u);
                OpLda(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
                OpSta(memory, cpu, OpDp(cpu, 0u));
                OpIny(cpu);
                OpIny(cpu);
                OpLda(memory, cpu, OpAbsY(cpu, CARD_RANK_WORK));
                cpu->carry = 0u;
                OpAdc(memory, cpu, OpDp(cpu, 0u));
                OpCmpValue(cpu, 12u);
                rank = cpu->zero ? 6u : 1u;
            }
        } else {
            OpCmpValue(cpu, 3u);
            if (cpu->zero) {
                rank = RankThreeCards(memory, cpu);
            } else {
                OpCmpValue(cpu, 5u);
                if (cpu->zero)
                    rank = 2u;
            }
        }
    }
    for (unsigned step = 0u; step < rank; ++step)
        OpStepMem(memory, cpu, OpDp(cpu, 6u), 1);
    OpLda(memory, cpu, OpDp(cpu, 6u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    return ExecutionReturned(0x84ac65u);
}
