#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/poker.h"
#include "system/wram.h"

enum {
    POKER_RANKS = WRAM_CARD_GAME_HAND_RANKS & 0xffffu,
    POKER_VALUES = WRAM_POKER_RANK_VALUES & 0xffffu,
    POKER_ROWS = WRAM_POKER_RANK_DISPLAY_ROWS & 0xffffu,
    POKER_VISIBLE_COUNTS = WRAM_POKER_VISIBLE_CARD_COUNTS & 0xffffu,
    POKER_ORDER = WRAM_POKER_RANK_ORDER & 0xffffu,
    POKER_SEATS = WRAM_CARD_GAME_HAND_WINS & 0xffffu,
    POKER_HANDS = WRAM_CARD_GAME_HANDS & 0xffffu,
    POKER_COUNTS = WRAM_CARD_GAME_HAND_COUNTS & 0xffffu,
    POKER_WORK = WRAM_CARD_GAME_RANK_WORK & 0xffffu,
    POKER_STATUS = WRAM_POKER_ROUND_STATUS & 0xffffu,
    POKER_WINDOW = WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu,
    POKER_RESULT_LIMIT = 65536u
};

typedef struct PokerResults {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
    unsigned steps;
} PokerResults;

static bool ResultReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

static bool ResultCall(PokerResults *results, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = results->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = (target >> 16) == 0x84u ? 2u : 3u;
    if (results->calls++ == POKER_RESULT_LIMIT) {
        results->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(results->memory, cpu, results->child,
                            results->context, site, target, frame, 0x84u)) {
        results->result = ExecutionReturned(site);
        results->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (cpu->program_bank != 0x84u || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal ||
        cpu->accumulator_is_8_bit != byte_accumulator) {
        results->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static bool ResultStep(PokerResults *results, uint32_t pc) {
    if (results->steps++ != POKER_RESULT_LIMIT)
        return true;
    results->result = ExecutionHandoff(results->cpu, pc);
    return false;
}

static void SwapRankWords(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t field) {
    OpLda(memory, cpu, OpAbsY(cpu, field));
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpLda(memory, cpu, OpAbsX(cpu, field));
    OpSta(memory, cpu, OpAbsY(cpu, field));
    OpLda(memory, cpu, OpDp(cpu, 4u));
    OpSta(memory, cpu, OpAbsX(cpu, field));
}

Lufia2ExecutionResult Lufia2PokerPrepareHandRanks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84c3b4u);
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    RolA16(cpu);
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 6u));
    OpLdy(cpu, 8u);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, POKER_SEATS));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    do {
        OpLda(memory, cpu, OpDp(cpu, 0u));
        if (!cpu->zero)
            OpLda(memory, cpu, OpAbsX(cpu, POKER_HANDS));
        OpSta(memory, cpu, OpAbsY(cpu, POKER_WORK));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 6u)));
    OpLda(memory, cpu, OpDp(cpu, 0x11u));
    bool hide_card = false;
    if (!cpu->zero) {
        OpCmp(memory, cpu, OpDp(cpu, 0x24u));
        hide_card = !cpu->zero;
    }
    if (hide_card) {
        OpStz(memory, cpu, OpAbs(cpu, POKER_WORK + 8u));
        OpLda(memory, cpu, OpAbsX(cpu, POKER_COUNTS));
        OpDecA(cpu);
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, POKER_COUNTS));
    }
    OpSta(memory, cpu, OpAbsX(cpu, POKER_VISIBLE_COUNTS));
    OpSta(memory, cpu, OpDp(cpu, 0x1fu));
    OpLdy(cpu, 0u);
    do {
        OpTya(cpu);
        OpIncA(cpu);
        OpIncA(cpu);
        OpTax(cpu);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, POKER_WORK));
            OpAndValue(cpu, 0xffu);
            OpSta(memory, cpu, OpDp(cpu, 2u));
            OpLda(memory, cpu, OpAbsY(cpu, POKER_WORK));
            OpAndValue(cpu, 0xffu);
            OpCmp(memory, cpu, OpDp(cpu, 2u));
            if (cpu->negative) {
                OpLda(memory, cpu, OpAbsY(cpu, POKER_WORK));
                OpSta(memory, cpu, OpDp(cpu, 0u));
                OpLda(memory, cpu, OpAbsX(cpu, POKER_WORK));
                OpSta(memory, cpu, OpAbsY(cpu, POKER_WORK));
                OpLda(memory, cpu, OpDp(cpu, 0u));
                OpSta(memory, cpu, OpAbsX(cpu, POKER_WORK));
            }
            OpInx(cpu);
            OpInx(cpu);
            OpCpx(cpu, 10u);
        } while (!cpu->zero);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 8u);
    } while (!cpu->zero);
    return ExecutionReturned(0x84c429u);
}

Lufia2ExecutionResult Lufia2PokerCountCardMatches(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84c55bu);
    PushY(memory, cpu);
    OpPushX(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, 4u));
    OpTax(cpu);
    OpStz(memory, cpu, OpDp(cpu, 6u));
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, POKER_WORK));
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x0eu)));
        OpCmp(memory, cpu, OpDp(cpu, 0u));
        if (cpu->zero)
            OpStepMem(memory, cpu, OpDp(cpu, 6u), 1);
        OpIny(cpu);
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    OpPullX(memory, cpu);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x84c577u);
}

static bool MeasureRepeatedRanks(PokerResults *results) {
    const Lufia2Memory *memory = results->memory;
    Lufia2CpuState *cpu = results->cpu;
    do {
        OpLda(memory, cpu, OpAbsY(cpu, POKER_WORK));
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x0eu)));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        if (!ResultCall(results, 0x84c458u, 0x84c55bu))
            return false;
        OpLda(memory, cpu, OpDp(cpu, 6u));
        OpDecA(cpu);
        bool repeated = !cpu->zero;
        if (repeated) {
            OpLda(memory, cpu, OpDp(cpu, 0u));
            OpCmp(memory, cpu, OpDp(cpu, 0x0bu));
            repeated = !cpu->zero;
        }
        if (repeated) {
            OpSta(memory, cpu, OpDp(cpu, 0x0bu));
            OpCmpValue(cpu, 1u);
            if (cpu->zero)
                OpLoadA(cpu, 14u);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpAbsX(cpu, POKER_VALUES));
            OpSta(memory, cpu, OpAbsX(cpu, POKER_VALUES));
            OpStepMem(memory, cpu, OpAbsX(cpu, POKER_RANKS), 1);
            OpLda(memory, cpu, OpDp(cpu, 6u));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpAbsX(cpu, POKER_ROWS));
            OpSta(memory, cpu, OpAbsX(cpu, POKER_ROWS));
        }
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
    } while (!cpu->zero);
    return true;
}

static bool MeasureStraight(PokerResults *results) {
    const Lufia2Memory *memory = results->memory;
    Lufia2CpuState *cpu = results->cpu;
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpLoadA(cpu, 5u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x1fu)));
    OpSta(memory, cpu, OpDp(cpu, 0x0bu));
    OpStz(memory, cpu, OpDp(cpu, 2u));
    OpStz(memory, cpu, OpDp(cpu, 6u));
    OpLdy(cpu, 0u);
    OpLda(memory, cpu, OpAbsY(cpu, POKER_WORK));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x0eu)));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpCmpValue(cpu, 13u);
    if (cpu->zero)
        OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
    do {
        if (!ResultStep(results, 0x84c4c3u))
            return false;
        OpIny(cpu);
        OpIny(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, POKER_WORK));
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x0eu)));
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
        OpCmp(memory, cpu, OpDp(cpu, 0u));
        bool adjacent = cpu->zero != 0u;
        while (!adjacent) {
            if (!ResultStep(results, 0x84c4d4u))
                return false;
            OpLda(memory, cpu, OpDp(cpu, 0x0bu));
            if (cpu->zero)
                break;
            OpStepMem(memory, cpu, OpDp(cpu, 0x0bu), -1);
            OpLda(memory, cpu, OpAbsY(cpu, POKER_WORK));
            OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x0eu)));
            OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
            OpCmp(memory, cpu, OpDp(cpu, 0u));
            adjacent = cpu->zero != 0u;
        }
        if (adjacent)
            OpStepMem(memory, cpu, OpDp(cpu, 6u), 1);
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, 2u));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 6u));
        OpIncA(cpu);
        OpIncA(cpu);
        OpCmp(memory, cpu, OpDp(cpu, 0x1fu));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, POKER_WORK));
            OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x0eu)));
            OpCmp(memory, cpu, OpDp(cpu, 2u));
            if (cpu->zero) {
                OpStepMem(memory, cpu, OpDp(cpu, 6u), 1);
                OpLda(memory, cpu, OpAbs(cpu, POKER_WORK));
                OpAndValue(cpu, 0xff00u);
                OpOraValue(cpu, 14u);
                OpSta(memory, cpu, OpAbs(cpu, POKER_WORK));
            }
        }
    }
    OpLda(memory, cpu, OpDp(cpu, 6u));
    OpIncA(cpu);
    OpCmp(memory, cpu, OpDp(cpu, 0x1fu));
    if (cpu->zero) {
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpAbsX(cpu, POKER_RANKS));
        OpLda(memory, cpu, OpAbs(cpu, POKER_WORK));
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, POKER_VALUES));
    }
    return true;
}

static bool MeasureFlush(PokerResults *results) {
    const Lufia2Memory *memory = results->memory;
    Lufia2CpuState *cpu = results->cpu;
    OpLda(memory, cpu, OpDp(cpu, 0x1fu));
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLoadA(cpu, 0xff00u);
    OpSta(memory, cpu, OpDp(cpu, 0x0eu));
    OpLdy(cpu, 0u);
    OpLda(memory, cpu, OpAbsY(cpu, POKER_WORK));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x0eu)));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    if (!ResultCall(results, 0x84c539u, 0x84c55bu))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 6u));
    OpCmp(memory, cpu, OpDp(cpu, 2u));
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, POKER_RANKS));
        OpOraValue(cpu, 8u);
        OpSta(memory, cpu, OpAbsX(cpu, POKER_RANKS));
        OpLda(memory, cpu, OpAbs(cpu, POKER_VALUES));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, POKER_WORK));
            OpAndValue(cpu, 0xffu);
            OpSta(memory, cpu, OpAbs(cpu, POKER_VALUES));
        }
    }
    return true;
}

Lufia2ExecutionResult Lufia2PokerMeasureHand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84c42au);
    PokerResults results = {memory, cpu, child, context,
                            ExecutionReturned(0x84c55au), 0u, 0u};
    PushAccumulator16(memory, cpu);
    if (!ResultCall(&results, 0x84c42bu, 0x84c3b4u))
        return results.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 6u)));
    OpStz(memory, cpu, OpAbsX(cpu, POKER_VALUES));
    OpStz(memory, cpu, OpAbsX(cpu, POKER_ROWS));
    OpStz(memory, cpu, OpAbsX(cpu, POKER_RANKS));
    OpLda(memory, cpu, OpAbsX(cpu, POKER_SEATS));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x1fu));
        OpSta(memory, cpu, OpDp(cpu, 4u));
        OpSta(memory, cpu, OpDp(cpu, 2u));
        OpStz(memory, cpu, OpDp(cpu, 0x0bu));
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, 0x0eu));
        OpLdy(cpu, 0u);
        if (!MeasureRepeatedRanks(&results))
            return results.result;
        OpLda(memory, cpu, OpAbsX(cpu, POKER_RANKS));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, POKER_ROWS));
            OpCmpValue(cpu, 5u);
            if (cpu->zero) {
                OpLoadA(cpu, 3u);
                OpSta(memory, cpu, OpAbsX(cpu, POKER_RANKS));
            }
        } else {
            OpLda(memory, cpu, OpDp(cpu, 0x1fu));
            OpDecA(cpu);
            if (!cpu->zero &&
                (!MeasureStraight(&results) || !MeasureFlush(&results)))
                return results.result;
        }
    }
    PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x84c55au);
}

static void AddRankWeight(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           unsigned increments) {
    for (unsigned step = 0u; step < increments; ++step)
        OpStepMem(memory, cpu, OpDp(cpu, 0u), 1);
}

static bool SelectRankWeight(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, POKER_SEATS));
    if (cpu->zero)
        return false;
    OpLda(memory, cpu, OpAbsX(cpu, POKER_RANKS));
    if (cpu->zero)
        return false;
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, POKER_ROWS));
        OpCmpValue(cpu, 3u);
        AddRankWeight(memory, cpu, cpu->zero ? 3u : cpu->negative ? 1u : 7u);
        return true;
    }
    OpLda(memory, cpu, OpAbsX(cpu, POKER_RANKS));
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        AddRankWeight(memory, cpu, 2u);
        return true;
    }
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        AddRankWeight(memory, cpu, 6u);
        return true;
    }
    OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
    OpCmpValue(cpu, 4u);
    if (cpu->zero) {
        AddRankWeight(memory, cpu, 4u);
        return true;
    }
    OpCmpValue(cpu, 8u);
    if (cpu->zero) {
        AddRankWeight(memory, cpu, 5u);
        return true;
    }
    OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
    OpLda(memory, cpu, OpAbsX(cpu, POKER_VALUES));
    OpAndValue(cpu, 0xffu);
    OpCmpValue(cpu, 14u);
    if (cpu->zero) {
        OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), 1);
    }
    AddRankWeight(memory, cpu, 8u);
    return true;
}

static void MarkCompleteRank(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, POKER_VISIBLE_COUNTS));
    OpCmpValue(cpu, 5u);
    bool complete = cpu->zero != 0u;
    if (!complete) {
        OpLoadA(cpu, 1u);
        OpCmp(memory, cpu, OpDp(cpu, 0u));
        complete = cpu->zero != 0u;
        if (!complete) {
            OpLda(memory, cpu, OpAbsX(cpu, POKER_VISIBLE_COUNTS));
            OpCmpValue(cpu, 3u);
            if (cpu->negative)
                return;
            OpLoadA(cpu, 3u);
            OpCmp(memory, cpu, OpDp(cpu, 0u));
            complete = !cpu->negative;
            if (!complete) {
                OpLda(memory, cpu, OpAbsX(cpu, POKER_VISIBLE_COUNTS));
                OpCmpValue(cpu, 4u);
                if (cpu->negative)
                    return;
                OpLda(memory, cpu, OpDp(cpu, 0u));
                OpCmpValue(cpu, 2u);
                complete = cpu->zero != 0u;
                if (!complete) {
                    OpCmpValue(cpu, 7u);
                    complete = cpu->zero != 0u;
                }
            }
        }
    }
    if (complete) {
        OpLoadA(cpu, 0x100u);
        OpOra(memory, cpu, OpDp(cpu, 0u));
        OpSta(memory, cpu, OpDp(cpu, 0u));
    }
}

Lufia2ExecutionResult Lufia2PokerClassifyHand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84bb2fu);
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpStz(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    if (SelectRankWeight(memory, cpu))
        MarkCompleteRank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpAbsX(cpu, POKER_RANKS));
    OpLda(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpAbsX(cpu, POKER_ROWS));
    return ExecutionReturned(0x84bb8cu);
}

Lufia2ExecutionResult Lufia2PokerRankHands(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84bbf1u);
    PokerResults results = {memory, cpu, child, context,
                            ExecutionReturned(0x84bc77u), 0u, 0u};
    OpStz(memory, cpu, OpDp(cpu, 0x24u));
    do {
        if (!ResultCall(&results, 0x84bbf3u, 0x84c42au) ||
            !ResultCall(&results, 0x84bbf6u, 0x84bb2fu))
            return results.result;
        OpStepMem(memory, cpu, OpDp(cpu, 0x24u), 1);
        OpLoadA(cpu, 4u);
        OpCmp(memory, cpu, OpDp(cpu, 0x24u));
    } while (!cpu->zero);
    OpLoadA(cpu, 0u);
    OpTax(cpu);
    OpLdy(cpu, 4u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, POKER_ORDER));
        OpInx(cpu);
        OpInx(cpu);
        OpIncA(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpStz(memory, cpu, OpDp(cpu, 0u));
    do {
        OpLda(memory, cpu, OpDp(cpu, 0u));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTay(cpu);
        OpTyx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpLoadA(cpu, 3u);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0u)));
        OpSta(memory, cpu, OpDp(cpu, 2u));
        do {
            OpLda(memory, cpu, OpAbsY(cpu, POKER_RANKS));
            OpCmp(memory, cpu, OpAbsX(cpu, POKER_RANKS));
            bool swap = cpu->negative != 0u;
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsY(cpu, POKER_VALUES));
                OpCmp(memory, cpu, OpAbsX(cpu, POKER_VALUES));
                swap = cpu->negative != 0u;
            }
            if (swap) {
                SwapRankWords(memory, cpu, POKER_RANKS);
                SwapRankWords(memory, cpu, POKER_VALUES);
                SwapRankWords(memory, cpu, POKER_ORDER);
            }
            OpInx(cpu);
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), 1);
        OpLda(memory, cpu, OpDp(cpu, 0u));
        OpCmpValue(cpu, 3u);
    } while (!cpu->zero);
    return ExecutionReturned(0x84bc77u);
}

Lufia2ExecutionResult Lufia2PokerDrawSeatResult(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84c6e8u);
    OpCmpValue(cpu, 1u);
    if (cpu->zero)
        return ExecutionReturned(0x84c739u);
    OpLda(memory, cpu, OpLongX(cpu, 0x84b7ddu));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, POKER_SEATS));
    if (cpu->zero) {
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x324au);
    } else {
        OpLoadA(cpu, 0x322au);
    }
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER));
    OpIny(cpu);
    OpIny(cpu);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    for (unsigned row = 1u; row < 3u; ++row) {
        OpLda(memory, cpu, OpLongX(cpu, 0x84b7ddu));
        cpu->carry = 0u;
        OpAdcValue(cpu, (uint16_t)(row * 16u));
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER));
        OpIny(cpu);
        OpIny(cpu);
        OpLda(memory, cpu, OpDp(cpu, 2u));
        OpIncA(cpu);
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_LOW_BUFFER));
        OpIny(cpu);
        OpIny(cpu);
        if (row == 1u)
            OpSta(memory, cpu, OpDp(cpu, 2u));
    }
    return ExecutionReturned(0x84c739u);
}

Lufia2ExecutionResult Lufia2PokerRefreshSeatSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84ba1fu);
    PokerResults results = {memory, cpu, child, context,
                            ExecutionReturned(0x84ba3fu), 0u, 0u};
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0u);
    OpTax(cpu);
    OpLdy(cpu, 96u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, POKER_SEATS));
        if (!ResultCall(&results, 0x84ba2eu, 0x84c6e8u))
            return results.result;
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84ba3fu);
}

Lufia2ExecutionResult Lufia2PokerLocateResultText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84bbdau);
    OpLda(memory, cpu, OpAbs(cpu, POKER_WINDOW + 2u));
    OpIncA(cpu);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    RorA16(cpu);
    RorA16(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, POKER_WINDOW));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, POKER_WINDOW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x3000u);
    OpIncA(cpu);
    OpIncA(cpu);
    OpTax(cpu);
    return ExecutionReturned(0x84bbf0u);
}

Lufia2ExecutionResult Lufia2PokerDrawHandResult(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84baf1u);
    PokerResults results = {memory, cpu, child, context,
                            ExecutionReturned(0x84bb2eu), 0u, 0u};
    OpPushX(memory, cpu);
    OpLdy(cpu, 0u);
    OpLda(memory, cpu, OpAbsX(cpu, POKER_RANKS));
    if (!cpu->zero) {
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, 6u));
        OpLda(memory, cpu, OpDp(cpu, 2u));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 1u;
        OpSbcValue(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW));
        OpLda(memory, cpu, OpDp(cpu, 2u));
        ExchangeAccumulatorBytes(cpu);
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW + 2u));
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW + 4u));
        OpLda(memory, cpu, OpAbsX(cpu, POKER_ROWS));
        OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW + 6u));
        if (!ResultCall(&results, 0x84bb20u, 0x84a2b3u) ||
            !ResultCall(&results, 0x84bb23u, 0x84bbdau))
            return results.result;
        OpLdy(cpu, 0xfc4du);
        if (!ResultCall(&results, 0x84bb29u, 0x808878u))
            return results.result;
    }
    OpPullX(memory, cpu);
    return ExecutionReturned(0x84bb2eu);
}

Lufia2ExecutionResult Lufia2PokerResolveShowdown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ResultReady(cpu))
        return ExecutionHandoff(cpu, 0x84ba53u);
    PokerResults results = {memory, cpu, child, context,
                            ExecutionReturned(0x84baf0u), 0u, 0u};
    OpLda(memory, cpu, OpAbs(cpu, POKER_STATUS));
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        OpLdy(cpu, 4u);
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, POKER_SEATS));
            if (!cpu->zero)
                OpStepMem(memory, cpu, OpAbsX(cpu, POKER_SEATS), 1);
            OpInx(cpu);
            OpInx(cpu);
            OpDey(cpu);
        } while (!cpu->zero);
        if (!ResultCall(&results, 0x84ba6eu, 0x84ba1fu))
            return results.result;
    } else {
        OpStz(memory, cpu, OpDp(cpu, 0x11u));
        if (!ResultCall(&results, 0x84ba76u, 0x84bbf1u))
            return results.result;
        OpLda(memory, cpu, OpAbs(cpu, POKER_RANKS));
        bool tied = cpu->zero != 0u;
        if (!tied) {
            OpCmp(memory, cpu, OpAbs(cpu, POKER_RANKS + 2u));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, POKER_VALUES));
                OpCmp(memory, cpu, OpAbs(cpu, POKER_VALUES + 2u));
                if (cpu->zero) {
                    OpLoadA(cpu, 0u);
                    tied = true;
                }
            }
        }
        if (tied) {
            OpSta(memory, cpu, OpDp(cpu, 0u));
            return ExecutionReturned(0x84baf0u);
        }
        OpStz(memory, cpu, OpDp(cpu, 0x24u));
        do {
            if (!ResultCall(&results, 0x84ba92u, 0x84c42au) ||
                !ResultCall(&results, 0x84ba95u, 0x84bb2fu))
                return results.result;
            OpStepMem(memory, cpu, OpDp(cpu, 0x24u), 1);
            OpLoadA(cpu, 4u);
            OpCmp(memory, cpu, OpDp(cpu, 0x24u));
        } while (!cpu->zero);
        OpLda(memory, cpu, OpAbs(cpu, POKER_ORDER));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbsX(cpu, POKER_SEATS));
        OpLdy(cpu, 2u);
        OpLoadA(cpu, 3u);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        do {
            OpLda(memory, cpu, OpAbsY(cpu, POKER_ORDER));
            cpu->carry = 0u;
            RolA16(cpu);
            OpTax(cpu);
            OpStz(memory, cpu, OpAbsX(cpu, POKER_SEATS));
            OpInx(cpu);
            OpInx(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
        } while (!cpu->zero);
        if (!ResultCall(&results, 0x84bac6u, 0x84ba1fu))
            return results.result;
        OpLdx(cpu, 0u);
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        do {
            OpLda(memory, cpu, OpLongX(cpu, 0x84b7d5u));
            OpSta(memory, cpu, OpDp(cpu, 2u));
            if (!ResultCall(&results, 0x84bad7u, 0x84baf1u))
                return results.result;
            OpInx(cpu);
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
        } while (!cpu->zero);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x88u);
        OpSta(memory, cpu, OpDp(cpu, 0x74u));
        if (!ResultCall(&results, 0x84bae6u, 0x84a5a8u))
            return results.result;
        OpRepWidths(cpu, 0x20u);
    }
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    return ExecutionReturned(0x84baf0u);
}
