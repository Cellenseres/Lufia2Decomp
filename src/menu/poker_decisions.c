#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/poker.h"
#include "system/wram.h"

enum {
    OPPONENT_SEAT = 0x11u,
    OPPONENT_ACTION = 4u,
    OPPONENT_SEATS = WRAM_CARD_GAME_HAND_WINS & 0xffffu,
    OPPONENT_RANKS = WRAM_CARD_GAME_HAND_RANKS & 0xffffu,
    OPPONENT_ORDER = WRAM_POKER_RANK_ORDER & 0xffffu,
    OPPONENT_INDICES = (WRAM_POKER_OPPONENT_INDICES - 2u) & 0xffffu,
    OPPONENT_VALUES = (WRAM_POKER_OPPONENT_VALUES - 2u) & 0xffffu,
    OPPONENT_STAKE = WRAM_POKER_ROUND_STAKE & 0xffffu,
    OPPONENT_ACCEPTED = WRAM_CARD_GAME_ACCEPTED_STAKE & 0xffffu,
    OPPONENT_BASE_STAKE = WRAM_CARD_GAME_STAKE & 0xffffu,
    OPPONENT_RAISED = WRAM_POKER_ACTION_STATE & 0xffffu,
    OPPONENT_ROUNDS = WRAM_POKER_ROUNDS_REMAINING & 0xffffu,
    OPPONENT_STATUS = WRAM_POKER_ROUND_STATUS & 0xffffu,
    OPPONENT_ORIGIN = WRAM_POKER_STAKE_SPRITE_ORIGIN & 0xffffu,
    OPPONENT_WINDOW = WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu,
    OPPONENT_WORK_LIMIT = 65536u
};

typedef struct OpponentTurn {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} OpponentTurn;

static bool OpponentReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

static bool OpponentCall(OpponentTurn *turn, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = turn->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = (target >> 16) == 0x84u ? 2u : 3u;
    if (turn->calls++ == OPPONENT_WORK_LIMIT) {
        turn->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(turn->memory, cpu, turn->child, turn->context,
                            site, target, frame, 0x84u)) {
        turn->result = ExecutionReturned(site);
        turn->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (cpu->program_bank != 0x84u || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal ||
        cpu->accumulator_is_8_bit != byte_accumulator) {
        turn->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static void FindOpponentRank(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_ORDER));
        OpCmp(memory, cpu, OpDp(cpu, OPPONENT_SEAT));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 8u);
    } while (!cpu->zero);
    OpCpx(cpu, 0u);
}

static void SelectOpponentSeat(const Lufia2Memory *memory,
                               Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, OPPONENT_SEAT));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
}

static bool EstimateOpponentBudget(OpponentTurn *turn, uint32_t random_site) {
    const Lufia2Memory *memory = turn->memory;
    Lufia2CpuState *cpu = turn->cpu;
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpAbs(cpu, OPPONENT_VALUES));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0u)));
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpAbs(cpu, OPPONENT_VALUES));
    if (!OpponentCall(turn, random_site, 0x808299u))
        return false;
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_RANKS));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0u));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, OPPONENT_VALUES));
        OpSta(memory, cpu, OpDp(cpu, 0u));
    }
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpCmp(memory, cpu, OpAbs(cpu, OPPONENT_STAKE));
    return true;
}

static void WriteOpponentAction(const Lufia2Memory *memory,
                                Lufia2CpuState *cpu, uint16_t action) {
    OpLoadA(cpu, action);
    OpSta(memory, cpu, OpDp(cpu, OPPONENT_ACTION));
}

static void WriteOpponentRaise(const Lufia2Memory *memory,
                               Lufia2CpuState *cpu) {
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, OPPONENT_STAKE)));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    WriteOpponentAction(memory, cpu, 2u);
}

Lufia2ExecutionResult Lufia2PokerChooseOpponentAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OpponentReady(cpu))
        return ExecutionHandoff(cpu, 0x84bdc2u);
    OpponentTurn turn = {memory, cpu, child, context,
                         ExecutionReturned(0x84bedfu), 0u};
    if (!OpponentCall(&turn, 0x84bdc2u, 0x84bbf1u))
        return turn.result;
    FindOpponentRank(memory, cpu);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, OPPONENT_RANKS));
        OpAndValue(cpu, 0xff00u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, OPPONENT_ROUNDS));
            cpu->carry = 0u;
            RolA16(cpu);
            RolA16(cpu);
            OpSta(memory, cpu, OpDp(cpu, 0u));
            OpLda(memory, cpu, OpAbs(cpu, OPPONENT_RANKS));
            OpAndValue(cpu, 0xffu);
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 0u;
            RorA16(cpu);
            RorA16(cpu);
            RorA16(cpu);
            if (!OpponentCall(&turn, 0x84bdf9u, 0x808299u))
                return turn.result;
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, 0u));
            OpCmp(memory, cpu, OpAbs(cpu, OPPONENT_VALUES));
            if (!cpu->negative) {
                WriteOpponentAction(memory, cpu, 0u);
                return turn.result;
            }
        }
        OpLda(memory, cpu, OpAbs(cpu, OPPONENT_RAISED));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_RANKS));
            if (cpu->zero) {
                OpLoadA(cpu, 1u);
                WriteOpponentRaise(memory, cpu);
                return turn.result;
            }
        } else {
            OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_RANKS));
            if (!EstimateOpponentBudget(&turn, 0x84be32u))
                return turn.result;
            if (cpu->negative) {
                OpLoadA(cpu, 5u);
                cpu->carry = 1u;
                OpSbcValue(cpu, OpReadM(memory, cpu,
                                      OpAbs(cpu, OPPONENT_ROUNDS)));
                cpu->carry = 0u;
                for (unsigned shift = 0u; shift < 4u; ++shift)
                    RolA16(cpu);
                OpSta(memory, cpu, OpDp(cpu, 0u));
                OpLoadA(cpu, 100u);
                if (!OpponentCall(&turn, 0x84be60u, 0x808299u))
                    return turn.result;
                cpu->carry = 0u;
                OpAdc(memory, cpu, OpDp(cpu, 0u));
                OpCmp(memory, cpu, OpAbs(cpu, OPPONENT_VALUES));
                if (!cpu->negative) {
                    WriteOpponentAction(memory, cpu, 0u);
                    return turn.result;
                }
            }
            WriteOpponentAction(memory, cpu, 1u);
            return turn.result;
        }
    }
    OpLda(memory, cpu, OpAbs(cpu, OPPONENT_STAKE));
    OpCmpValue(cpu, 99u);
    if (cpu->zero) {
        WriteOpponentAction(memory, cpu, 1u);
        return turn.result;
    }
    OpLda(memory, cpu, OpAbs(cpu, OPPONENT_VALUES));
    if (!OpponentCall(&turn, 0x84be83u, 0x808299u))
        return turn.result;
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, OPPONENT_ROUNDS)));
    OpCpy(cpu, 2u);
    bool fixed_increment = false;
    if (!cpu->negative) {
        OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_RANKS));
        OpAndValue(cpu, 0xffu);
        OpCmpValue(cpu, 4u);
        if (!cpu->negative) {
            OpLoadA(cpu, 1u);
            fixed_increment = true;
        }
    }
    if (!fixed_increment) {
        do {
            OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_RANKS));
            OpAndValue(cpu, 0xffu);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, 0u));
            OpSta(memory, cpu, OpDp(cpu, 0u));
            OpDey(cpu);
        } while (!cpu->zero);
    }
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_RANKS));
    OpAndValue(cpu, 0xff00u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0u));
        RorA16(cpu);
        RorA16(cpu);
        OpAndValue(cpu, 0xffu);
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0u));
    }
    OpLda(memory, cpu, OpDp(cpu, 0u));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, OPPONENT_STAKE));
    OpCmpValue(cpu, 99u);
    if (!cpu->negative)
        OpLoadA(cpu, 99u);
    WriteOpponentRaise(memory, cpu);
    return turn.result;
}

Lufia2ExecutionResult Lufia2PokerAcceptOpponentStake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!OpponentReady(cpu))
        return ExecutionHandoff(cpu, 0x84bd83u);
    SelectOpponentSeat(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, OPPONENT_STAKE));
    OpSta(memory, cpu, OpAbsX(cpu, OPPONENT_STAKE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, OPPONENT_BASE_STAKE));
    OpSta(memory, cpu, OpAbsX(cpu, OPPONENT_ACCEPTED));
    return ExecutionReturned(0x84bd95u);
}

Lufia2ExecutionResult Lufia2PokerLocateStakeSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!OpponentReady(cpu))
        return ExecutionHandoff(cpu, 0x84bee4u);
    OpLda(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 2u);
    cpu->carry = 0u;
    for (unsigned shift = 0u; shift < 3u; ++shift)
        RolA16(cpu);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, OPPONENT_ORIGIN));
    OpLda(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW + 2u));
    cpu->carry = 0u;
    OpAdcValue(cpu, 2u);
    cpu->carry = 0u;
    for (unsigned shift = 0u; shift < 3u; ++shift)
        RolA16(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpAndValue(cpu, 0xff00u);
    OpOra(memory, cpu, OpAbs(cpu, OPPONENT_ORIGIN));
    OpSta(memory, cpu, OpAbs(cpu, OPPONENT_ORIGIN));
    return ExecutionReturned(0x84bf0au);
}

Lufia2ExecutionResult Lufia2PokerDrawOpponentStake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OpponentReady(cpu))
        return ExecutionHandoff(cpu, 0x84bd96u);
    OpponentTurn turn = {memory, cpu, child, context,
                         ExecutionReturned(0x84bdc1u), 0u};
    OpLda(memory, cpu, OpDp(cpu, 0x13u));
    if (!OpponentCall(&turn, 0x84bd98u, 0x84c31bu))
        return turn.result;
    OpLda(memory, cpu, OpDp(cpu, 0x13u));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 2u)));
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpStz(memory, cpu, OpDp(cpu, 0x2du));
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, OPPONENT_ORIGIN)));
    OpLoadA(cpu, 0x3600u);
    if (!OpponentCall(&turn, 0x84bdacu, 0x84c2ceu))
        return turn.result;
    OpLda(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpAbs(cpu, OPPONENT_ORIGIN));
    cpu->carry = 0u;
    OpAdcValue(cpu, 16u);
    OpTay(cpu);
    OpLoadA(cpu, 0x3400u);
    if (!OpponentCall(&turn, 0x84bdbeu, 0x84c2ceu))
        return turn.result;
    return turn.result;
}

Lufia2ExecutionResult Lufia2PokerMatchOpponentStake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OpponentReady(cpu))
        return ExecutionHandoff(cpu, 0x84bf67u);
    OpponentTurn turn = {memory, cpu, child, context,
                         ExecutionReturned(0x84c027u), 0u};
    if (!OpponentCall(&turn, 0x84bf67u, 0x84bbf1u))
        return turn.result;
    FindOpponentRank(memory, cpu);
    bool match_stake = cpu->zero != 0u;
    if (!match_stake) {
        OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_RANKS));
        if (cpu->zero) {
            OpLoadA(cpu, 100u);
            if (!OpponentCall(&turn, 0x84bf88u, 0x808299u))
                return turn.result;
            OpCmp(memory, cpu, OpAbs(cpu, OPPONENT_VALUES));
            match_stake = cpu->negative != 0u;
        }
        if (!match_stake) {
            if (!EstimateOpponentBudget(&turn, 0x84bfabu))
                return turn.result;
            match_stake = !cpu->negative;
        }
    }
    SelectOpponentSeat(memory, cpu);
    if (match_stake) {
        OpLda(memory, cpu, OpAbs(cpu, OPPONENT_STAKE));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, OPPONENT_BASE_STAKE));
        OpSta(memory, cpu, OpAbsX(cpu, OPPONENT_ACCEPTED));
        OpLoadA(cpu, 1u);
    } else {
        OpStz(memory, cpu, OpAbsX(cpu, OPPONENT_SEATS));
        OpStepMem(memory, cpu, OpAbs(cpu, OPPONENT_STATUS), 1);
        OpLoadA(cpu, 0u);
    }
    OpSta(memory, cpu, OpDp(cpu, OPPONENT_ACTION));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW + 6u));
    if (!OpponentCall(&turn, 0x84bff3u, 0x84a2b3u) ||
        !OpponentCall(&turn, 0x84bff6u, 0x84bbdau))
        return turn.result;
    OpLdy(cpu, 0xfc2bu);
    if (!OpponentCall(&turn, 0x84bffcu, 0x808878u) ||
        !OpponentCall(&turn, 0x84c000u, 0x84c578u) ||
        !OpponentCall(&turn, 0x84c003u, 0x84ba1fu))
        return turn.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpRepWidths(cpu, 0x20u);
    if (!OpponentCall(&turn, 0x84c00eu, 0x84bee4u))
        return turn.result;
    cpu->carry = 0u;
    OpAdcValue(cpu, 0xf830u);
    OpTax(cpu);
    if (!OpponentCall(&turn, 0x84c016u, 0x84c64bu) ||
        !OpponentCall(&turn, 0x84c019u, 0x84a39bu))
        return turn.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!OpponentCall(&turn, 0x84c022u, 0x84a5a8u))
        return turn.result;
    OpRepWidths(cpu, 0x20u);
    return turn.result;
}

static void SetOpponentWindow(const Lufia2Memory *memory,
                               Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpLongX(cpu, 0x84b7d5u));
    OpAndValue(cpu, 0xffu);
    cpu->carry = 1u;
    OpSbcValue(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW));
    OpLda(memory, cpu, OpLongX(cpu, 0x84b7d5u));
    ExchangeAccumulatorBytes(cpu);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW + 2u));
}

static void LoadOpponentProfile(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_INDICES));
    OpSta(memory, cpu, OpAbs(cpu, OPPONENT_INDICES));
    OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_VALUES));
    OpSta(memory, cpu, OpAbs(cpu, OPPONENT_VALUES));
}

Lufia2ExecutionResult Lufia2PokerTakeOpponentTurn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OpponentReady(cpu))
        return ExecutionHandoff(cpu, 0x84bc78u);
    OpponentTurn turn = {memory, cpu, child, context,
                         ExecutionReturned(0x84bd82u), 0u};
    SelectOpponentSeat(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_SEATS));
    if (!cpu->zero) {
        SetOpponentWindow(memory, cpu);
        OpLoadA(cpu, 10u);
        OpSta(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW + 4u));
        OpLoadA(cpu, 5u);
        OpSta(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW + 6u));
        OpLda(memory, cpu, OpDp(cpu, OPPONENT_SEAT));
        OpDecA(cpu);
        cpu->carry = 0u;
        RolA16(cpu);
        LoadOpponentProfile(memory, cpu);
        if (!OpponentCall(&turn, 0x84bcbbu, 0x84bdc2u))
            return turn.result;
        OpLda(memory, cpu, OpDp(cpu, OPPONENT_ACTION));
        bool raise_stake = false;
        if (!cpu->zero) {
            OpStepMem(memory, cpu, OpAbs(cpu, OPPONENT_RAISED), 1);
            OpCmpValue(cpu, 1u);
            if (cpu->zero) {
                if (!OpponentCall(&turn, 0x84bd34u, 0x84bd83u))
                    return turn.result;
            } else {
                raise_stake = true;
            }
        } else {
            SelectOpponentSeat(memory, cpu);
            OpStz(memory, cpu, OpAbsX(cpu, OPPONENT_SEATS));
            OpStepMem(memory, cpu, OpAbs(cpu, OPPONENT_STATUS), 1);
        }
        if (raise_stake) {
            OpStepMem(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW + 6u), 1);
            if (!OpponentCall(&turn, 0x84bccdu, 0x84bee0u) ||
                !OpponentCall(&turn, 0x84bcd0u, 0x84bee4u))
                return turn.result;
            OpLda(memory, cpu, OpDp(cpu, 0u));
            OpSta(memory, cpu, OpDp(cpu, 0x22u));
            OpLda(memory, cpu, OpAbs(cpu, OPPONENT_STAKE));
            OpSta(memory, cpu, OpDp(cpu, 0x13u));
            do {
                OpStepMem(memory, cpu, OpDp(cpu, 0x13u), 1);
                if (!OpponentCall(&turn, 0x84bcdeu, 0x84bd96u))
                    return turn.result;
                OpLoadA(cpu, 0x21u);
                if (!OpponentCall(&turn, 0x84bce4u, 0x84a607u) ||
                    !OpponentCall(&turn, 0x84bce7u, 0x84bbdau))
                    return turn.result;
                OpLdy(cpu, 0xfc0bu);
                if (!OpponentCall(&turn, 0x84bcedu, 0x808878u) ||
                    !OpponentCall(&turn, 0x84bcf1u, 0x84bbdau))
                    return turn.result;
                OpLdy(cpu, 0xfc18u);
                if (!OpponentCall(&turn, 0x84bcf7u, 0x808878u))
                    return turn.result;
                OpSepWidths(cpu, 0x20u);
                OpLoadA(cpu, 0x80u);
                OpSta(memory, cpu, OpDp(cpu, 0x72u));
                OpLoadA(cpu, 0x88u);
                OpSta(memory, cpu, OpDp(cpu, 0x74u));
                if (!OpponentCall(&turn, 0x84bd05u, 0x84a5a8u))
                    return turn.result;
                OpRepWidths(cpu, 0x20u);
                OpStepMem(memory, cpu, OpDp(cpu, 0x22u), -1);
            } while (!cpu->zero);
            OpLda(memory, cpu, OpDp(cpu, 0x13u));
            OpSta(memory, cpu, OpAbs(cpu, OPPONENT_STAKE));
            if (!OpponentCall(&turn, 0x84bd13u, 0x84bd83u) ||
                !OpponentCall(&turn, 0x84bd16u, 0x84c578u))
                return turn.result;
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, 8u);
            OpSta(memory, cpu, OpDp(cpu, 0x74u));
            OpRepWidths(cpu, 0x20u);
            if (!OpponentCall(&turn, 0x84bd21u, 0x84bee4u))
                return turn.result;
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x1030u);
            OpTax(cpu);
            if (!OpponentCall(&turn, 0x84bd29u, 0x84c64bu) ||
                !OpponentCall(&turn, 0x84bd2cu, 0x84a39bu) ||
                !OpponentCall(&turn, 0x84bd2fu, 0x84c22cu))
                return turn.result;
        } else {
            OpLoadA(cpu, 3u);
            OpSta(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW + 6u));
            if (!OpponentCall(&turn, 0x84bd4au, 0x84a2b3u) ||
                !OpponentCall(&turn, 0x84bd4du, 0x84bbdau))
                return turn.result;
            OpLdy(cpu, 0xfc2bu);
            if (!OpponentCall(&turn, 0x84bd53u, 0x808878u) ||
                !OpponentCall(&turn, 0x84bd57u, 0x84c578u) ||
                !OpponentCall(&turn, 0x84bd5au, 0x84ba1fu))
                return turn.result;
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, 0x88u);
            OpSta(memory, cpu, OpDp(cpu, 0x74u));
            OpRepWidths(cpu, 0x20u);
            if (!OpponentCall(&turn, 0x84bd65u, 0x84bee4u))
                return turn.result;
            cpu->carry = 0u;
            OpAdcValue(cpu, 0xf830u);
            OpTax(cpu);
            if (!OpponentCall(&turn, 0x84bd6du, 0x84c64bu) ||
                !OpponentCall(&turn, 0x84bd70u, 0x84a39bu))
                return turn.result;
        }
    }
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!OpponentCall(&turn, 0x84bd7du, 0x84a5a8u))
        return turn.result;
    OpRepWidths(cpu, 0x20u);
    return turn.result;
}

Lufia2ExecutionResult Lufia2PokerMatchOpponentStakes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !OpponentReady(cpu))
        return ExecutionHandoff(cpu, 0x84bf0bu);
    OpponentTurn turn = {memory, cpu, child, context,
                         ExecutionReturned(0x84bf66u), 0u};
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW + 4u));
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpAbs(cpu, OPPONENT_WINDOW + 6u));
    OpStz(memory, cpu, OpDp(cpu, OPPONENT_SEAT));
    OpStepMem(memory, cpu, OpDp(cpu, OPPONENT_SEAT), 1);
    do {
        SelectOpponentSeat(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_SEATS));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_STAKE));
            OpCmp(memory, cpu, OpAbs(cpu, OPPONENT_STAKE));
            if (!cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_VALUES));
                OpSta(memory, cpu, OpAbs(cpu, OPPONENT_VALUES));
                OpLda(memory, cpu, OpAbsX(cpu, OPPONENT_INDICES));
                OpSta(memory, cpu, OpAbs(cpu, OPPONENT_INDICES));
                SetOpponentWindow(memory, cpu);
                if (!OpponentCall(&turn, 0x84bf52u, 0x84bf67u))
                    return turn.result;
            }
        }
        OpStepMem(memory, cpu, OpDp(cpu, OPPONENT_SEAT), 1);
        OpLoadA(cpu, 4u);
        OpCmp(memory, cpu, OpDp(cpu, OPPONENT_SEAT));
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpRepWidths(cpu, 0x20u);
    return turn.result;
}
