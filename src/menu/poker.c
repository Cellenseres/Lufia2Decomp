#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/poker.h"
#include "system/wram.h"

enum {
    POKER_STAKE = WRAM_CARD_GAME_STAKE & 0xffffu,
    POKER_STAKES = WRAM_CARD_GAME_ACCEPTED_STAKE & 0xffffu,
    POKER_SEATS = WRAM_CARD_GAME_HAND_WINS & 0xffffu,
    POKER_HANDS = WRAM_CARD_GAME_HANDS & 0xffffu,
    POKER_COUNTS = WRAM_CARD_GAME_HAND_COUNTS & 0xffffu,
    POKER_CARRYOVER = WRAM_POKER_CARRYOVER & 0xffffu,
    POKER_ROUNDS = WRAM_POKER_ROUNDS_REMAINING & 0xffffu,
    POKER_ROUND_STATUS = WRAM_POKER_ROUND_STATUS & 0xffffu,
    POKER_ROUND_STAKE = WRAM_POKER_ROUND_STAKE & 0xffffu,
    POKER_ACTION_STATE = WRAM_POKER_ACTION_STATE & 0xffffu,
    POKER_MARKER_TIMER = WRAM_POKER_MARKER_TIMER & 0xffffu,
    POKER_OPPONENTS = WRAM_POKER_OPPONENT_INDICES & 0xffffu,
    POKER_OPPONENT_VALUES = WRAM_POKER_OPPONENT_VALUES & 0xffffu,
    POKER_BALANCE = WRAM_MENU_EXCHANGE_BALANCE & 0xffffu,
    POKER_WINDOW = WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu,
    POKER_WINDOW_STYLE = WRAM_SPECIAL_MENU_WINDOW_STYLE & 0xffffu,
    POKER_WINDOW_HEIGHT = POKER_WINDOW + 6u,
    POKER_WINDOW_STATE = WRAM_SPECIAL_MENU_WINDOW_STATE & 0xffffu,
    POKER_CURSOR_DEPTH = WRAM_SPECIAL_MENU_CURSOR_DEPTH & 0xffffu,
    POKER_REPLAY_CURSOR = WRAM_CARD_GAME_REPLAY_CURSOR & 0xffffu,
    POKER_CALL_LIMIT = 65536u
};

typedef struct PokerGame {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} PokerGame;

static bool PokerReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static bool PokerCall(PokerGame *game, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = game->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = (target >> 16) == 0x84u ? 2u : 3u;
    if (game->calls++ == POKER_CALL_LIMIT) {
        game->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(game->memory, cpu, game->child, game->context,
                            site, target, frame, 0x84u)) {
        game->result = ExecutionReturned(site);
        game->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!PokerReady(cpu) || cpu->accumulator_is_8_bit != byte_accumulator) {
        game->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static bool ChargePokerEntry(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, POKER_BALANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, 10u);
    if (!cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, POKER_BALANCE + 2u));
        OpDecA(cpu);
        if (cpu->negative)
            return false;
        OpSta(memory, cpu, OpAbs(cpu, POKER_BALANCE + 2u));
        OpLda(memory, cpu, OpAbs(cpu, POKER_BALANCE));
        cpu->carry = 1u;
        OpSbcValue(cpu, 10u);
    }
    OpSta(memory, cpu, OpAbs(cpu, POKER_BALANCE));
    return true;
}

static void SetPokerWindow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t top) {
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW));
    OpLoadA(cpu, top);
    OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW + 2u));
    OpLoadA(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW + 4u));
}

static bool DealPokerOpeningHands(PokerGame *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpStz(memory, cpu, OpDp(cpu, 0x17u));
    OpStz(memory, cpu, OpDp(cpu, 0x30u));
    do {
        if (!PokerCall(game, 0x84b89eu, 0x84c5afu))
            return false;
        OpStepMem(memory, cpu, OpDp(cpu, 0x17u), 1);
        OpLoadA(cpu, 4u);
        OpCmp(memory, cpu, OpDp(cpu, 0x17u));
    } while (!cpu->zero);
    OpStepMem(memory, cpu, OpDp(cpu, 0x30u), 1);
    OpStz(memory, cpu, OpDp(cpu, 0x17u));
    do {
        if (!PokerCall(game, 0x84b8aeu, 0x84c5afu))
            return false;
        OpStepMem(memory, cpu, OpDp(cpu, 0x17u), 1);
        OpLoadA(cpu, 4u);
        OpCmp(memory, cpu, OpDp(cpu, 0x17u));
    } while (!cpu->zero);
    return true;
}

static bool PlayPokerRounds(PokerGame *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    do {
        OpStz(memory, cpu, OpAbs(cpu, POKER_ACTION_STATE));
        OpStz(memory, cpu, OpAbs(cpu, POKER_ROUND_STAKE));
        if (!PokerCall(game, 0x84b8c0u, 0x84c578u) ||
            !PokerCall(game, 0x84b8c3u, 0x84c26fu))
            return false;
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x88u);
        OpSta(memory, cpu, OpDp(cpu, 0x74u));
        if (!PokerCall(game, 0x84b8ccu, 0x84a5a8u))
            return false;
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, 0x11u));
        do {
            if (!PokerCall(game, 0x84b8d6u, 0x84bc78u))
                return false;
            OpStepMem(memory, cpu, OpDp(cpu, 0x11u), 1);
            OpLoadA(cpu, 4u);
            OpCmp(memory, cpu, OpDp(cpu, 0x11u));
        } while (!cpu->zero);
        OpLda(memory, cpu, OpAbs(cpu, POKER_SEATS));
        if (!cpu->zero && !PokerCall(game, 0x84b8e7u, 0x84c028u))
            return false;
        if (!PokerCall(game, 0x84b8eau, 0x84bf0bu))
            return false;
        OpLda(memory, cpu, OpAbs(cpu, POKER_ROUND_STAKE));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, POKER_STAKE));
        OpSta(memory, cpu, OpAbs(cpu, POKER_STAKE));
        OpLda(memory, cpu, OpAbs(cpu, POKER_ROUND_STATUS));
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            return true;
        OpStz(memory, cpu, OpDp(cpu, 0x17u));
        do {
            OpLda(memory, cpu, OpDp(cpu, 0x17u));
            cpu->carry = 0u;
            RolA16(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpAbsX(cpu, POKER_SEATS));
            if (!cpu->zero && !PokerCall(game, 0x84b90bu, 0x84c5afu))
                return false;
            OpStepMem(memory, cpu, OpDp(cpu, 0x17u), 1);
            OpLoadA(cpu, 4u);
            OpCmp(memory, cpu, OpDp(cpu, 0x17u));
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpAbs(cpu, POKER_ROUNDS), -1);
    } while (!cpu->zero);
    return PokerCall(game, 0x84b91cu, 0x84c73au) &&
           PokerCall(game, 0x84b91fu, 0x84c794u);
}

static bool ShowPokerTie(PokerGame *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW_HEIGHT));
    if (!PokerCall(game, 0x84b944u, 0x84a2b3u))
        return false;
    OpLdx(cpu, 0x3316u);
    OpLdy(cpu, 0xfd14u);
    if (!PokerCall(game, 0x84b94du, 0x808878u))
        return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x70a4u);
    if (!PokerCall(game, 0x84b95cu, 0x84c64bu))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, POKER_STAKE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, POKER_CARRYOVER));
    OpCmpValue(cpu, 60000u);
    if (cpu->carry)
        OpLoadA(cpu, 60000u);
    OpSta(memory, cpu, OpAbs(cpu, POKER_CARRYOVER));
    return true;
}

static bool AwardPokerPot(PokerGame *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpLda(memory, cpu, OpAbs(cpu, POKER_SEATS));
    if (cpu->zero) {
        OpLoadA(cpu, 0x29u);
        return PokerCall(game, 0x84b97bu, 0x84a607u);
    }
    OpLoadA(cpu, 0x28u);
    if (!PokerCall(game, 0x84b983u, 0x84a607u) ||
        !PokerCall(game, 0x84b986u, 0x84c794u))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, POKER_CARRYOVER));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, POKER_STAKE));
    OpCmpValue(cpu, 9999u);
    if (!cpu->negative)
        OpLoadA(cpu, 9999u);
    OpSta(memory, cpu, OpAbs(cpu, 0x6008u));
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, 0x600au));
    if (!PokerCall(game, 0x84b9a1u, 0x84a5dfu))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, 0x600cu));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    return PokerCall(game, 0x84b9a9u, 0x84c7a6u);
}

static bool ChoosePokerReplay(PokerGame *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW_HEIGHT));
    if (!PokerCall(game, 0x84b9b2u, 0x84a2b3u))
        return false;
    OpLdx(cpu, 0x3316u);
    OpLdy(cpu, 0xfa2du);
    if (!PokerCall(game, 0x84b9bbu, 0x808878u))
        return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!PokerCall(game, 0x84b9c5u, 0x84a5a8u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x7868u);
    OpWriteX(memory, cpu, OpAbs(cpu, POKER_REPLAY_CURSOR), cpu->x);
    OpLdx(cpu, 0x28u);
    OpWriteX(memory, cpu, OpAbs(cpu, POKER_REPLAY_CURSOR + 2u), cpu->x);
    if (!PokerCall(game, 0x84b9d6u, 0x84ab23u))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpStz(memory, cpu, OpAbs(cpu, POKER_CARRYOVER));
    return true;
}

static bool ShowPokerEmptyBalance(PokerGame *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    SetPokerWindow(memory, cpu, 10u);
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, POKER_WINDOW_HEIGHT));
    if (!PokerCall(game, 0x84b9feu, 0x84a2b3u))
        return false;
    OpLdx(cpu, 0x32d6u);
    OpLdy(cpu, 0xfa0au);
    if (!PokerCall(game, 0x84ba07u, 0x808878u))
        return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x80a0u);
    return PokerCall(game, 0x84ba16u, 0x84c64bu);
}

Lufia2ExecutionResult Lufia2PokerRun(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PokerReady(cpu) || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84b827u);
    PokerGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84ba1eu), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    if (!PokerCall(&game, 0x84b82fu, 0x84b57fu))
        return game.result;
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    Push8(memory, cpu, A8(cpu));
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, POKER_WINDOW_STATE), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, POKER_CARRYOVER));
    OpLoadA(cpu, 0u);
    for (unsigned opponent = 0u; opponent < 3u; ++opponent) {
        if (opponent)
            OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu,
            (uint16_t)(POKER_OPPONENTS + opponent * 2u)));
    }
    for (unsigned opponent = 0u; opponent < 3u; ++opponent) {
        OpLoadA(cpu, (uint16_t)(50u - opponent * 10u));
        OpSta(memory, cpu, OpAbs(cpu,
            (uint16_t)(POKER_OPPONENT_VALUES + opponent * 2u)));
    }
    for (;;) {
        OpRepWidths(cpu, 0x20u);
        if (!PokerCall(&game, 0x84b864u, 0x84b4d9u) ||
            !PokerCall(&game, 0x84b867u, 0x84c69cu))
            return game.result;
        if (!ChargePokerEntry(memory, cpu)) {
            if (!ShowPokerEmptyBalance(&game))
                return game.result;
            break;
        }
        if (!PokerCall(&game, 0x84b889u, 0x84c819u) ||
            !PokerCall(&game, 0x84b88cu, 0x84af11u))
            return game.result;
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x88u);
        OpSta(memory, cpu, OpDp(cpu, 0x74u));
        if (!PokerCall(&game, 0x84b895u, 0x84a5a8u))
            return game.result;
        OpRepWidths(cpu, 0x20u);
        if (!DealPokerOpeningHands(&game) || !PlayPokerRounds(&game) ||
            !PokerCall(&game, 0x84b922u, 0x84ba53u))
            return game.result;
        SetPokerWindow(memory, cpu, 11u);
        if (!PokerCall(&game, 0x84b937u, 0x84ba40u))
            return game.result;
        OpLda(memory, cpu, OpDp(cpu, 0u));
        if (cpu->zero) {
            if (!ShowPokerTie(&game))
                return game.result;
        } else {
            if (!AwardPokerPot(&game) || !ChoosePokerReplay(&game))
                return game.result;
            if (!cpu->zero)
                break;
        }
        if (!PokerCall(&game, 0x84b9e0u, 0x84a39bu))
            return game.result;
    }
    if (!PokerCall(&game, 0x84ba19u, 0x84a5cau))
        return game.result;
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84ba1eu);
}

Lufia2ExecutionResult Lufia2PokerSumStakes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!PokerReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84ba40u);
    OpLda(memory, cpu, OpAbs(cpu, POKER_STAKES));
    for (unsigned seat = 1u; seat < 4u; ++seat) {
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, (uint16_t)(POKER_STAKES + seat * 2u)));
    }
    OpSta(memory, cpu, OpAbs(cpu, POKER_STAKE));
    return ExecutionReturned(0x84ba52u);
}

Lufia2ExecutionResult Lufia2PokerDrawStakes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PokerReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84c578u);
    PokerGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84c5aeu), 0u};
    static const uint16_t seats[] = {1u, 2u, 3u, 0u};
    static const uint16_t destinations[] = {0x3386u, 0x33aeu, 0x335au, 0x3574u};
    static const uint32_t sites[] = {0x84c583u, 0x84c58fu, 0x84c59bu, 0x84c5aau};
    for (unsigned field = 0u; field < 4u; ++field) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(POKER_STAKES + seats[field] * 2u)));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLdx(cpu, destinations[field]);
        if (!field)
            OpLdy(cpu, 0xf922u);
        else if (field == 3u)
            OpLdy(cpu, 0xf932u);
        if (!PokerCall(&game, sites[field], 0x808878u))
            return game.result;
    }
    return ExecutionReturned(0x84c5aeu);
}

Lufia2ExecutionResult Lufia2PokerDealCard(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PokerReady(cpu))
        return ExecutionHandoff(cpu, 0x84c5afu);
    PokerGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84c64au), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x1bu));
    OpLdx(cpu, 0xb701u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x19u), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x17u));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpAbsX(cpu, POKER_COUNTS));
    cpu->carry = 0u;
    RolA16(cpu);
    RolA16(cpu);
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 2u));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    OpTay(cpu);
    OpStz(memory, cpu, OpDp(cpu, 2u));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
    OpSta(memory, cpu, OpDp(cpu, 0x11u));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpStz(memory, cpu, OpDp(cpu, 0x28u));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    if (!PokerCall(&game, 0x84c5f1u, 0x84a607u) ||
        !PokerCall(&game, 0x84c5f4u, 0x84b2bcu) ||
        !PokerCall(&game, 0x84c5f7u, 0x84b3c9u))
        return game.result;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    if (!PokerCall(&game, 0x84c5feu, 0x84a5a8u))
        return game.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x11u)));
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    do {
        if (!PokerCall(&game, 0x84c605u, 0x84b43bu))
            return game.result;
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, 0x72u));
        if (!PokerCall(&game, 0x84c60cu, 0x84a5a8u))
            return game.result;
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x20u);
    if (!PokerCall(&game, 0x84c615u, 0x84b295u))
        return game.result;
    OpLda(memory, cpu, OpDp(cpu, 0x24u));
    OpSta(memory, cpu, OpDp(cpu, 0x0bu));
    OpStz(memory, cpu, OpDp(cpu, 0x28u));
    OpLda(memory, cpu, OpDp(cpu, 0x30u));
    bool turn_card = !cpu->zero;
    if (!turn_card) {
        OpLda(memory, cpu, OpDp(cpu, 0x17u));
        turn_card = cpu->zero != 0u;
    }
    if (turn_card) {
        OpStepMem(memory, cpu, OpDp(cpu, 0x28u), 1);
        if (!PokerCall(&game, 0x84c62bu, 0x84b22bu))
            return game.result;
    }
    if (!PokerCall(&game, 0x84c62eu, 0x84b274u))
        return game.result;
    OpLda(memory, cpu, OpDp(cpu, 0x0bu));
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    if (!PokerCall(&game, 0x84c635u, 0x84af1cu) ||
        !PokerCall(&game, 0x84c638u, 0x84b2bcu) ||
        !PokerCall(&game, 0x84c63bu, 0x84b387u))
        return game.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!PokerCall(&game, 0x84c646u, 0x84a5a8u))
        return game.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84c64au);
}

Lufia2ExecutionResult Lufia2PokerWaitMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PokerReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84c64bu);
    PokerGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84c69bu), 0u};
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 508u), cpu->x);
    OpLdx(cpu, 0x3504u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 510u), cpu->x);
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, POKER_MARKER_TIMER));
    do {
        OpStepMem(memory, cpu, OpAbs(cpu, POKER_MARKER_TIMER), -1);
        if (cpu->negative) {
            OpLoadA(cpu, 10u);
            OpSta(memory, cpu, OpAbs(cpu, POKER_MARKER_TIMER));
            OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 510u));
            OpIncA(cpu);
            OpCmpValue(cpu, 0x3508u);
            if (cpu->zero)
                OpLoadA(cpu, 0x3504u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 510u));
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, 0x80u);
            OpSta(memory, cpu, OpDp(cpu, 0x72u));
            OpRepWidths(cpu, 0x20u);
        }
        if (!PokerCall(&game, 0x84c67cu, 0x84a5a8u) ||
            !PokerCall(&game, 0x84c67fu, 0x849f1au))
            return game.result;
        OpCmpValue(cpu, 0u);
    } while (cpu->zero);
    OpLoadA(cpu, 0xf000u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 508u));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 2u);
    if (!PokerCall(&game, 0x84c698u, 0x84a607u))
        return game.result;
    return ExecutionReturned(0x84c69bu);
}

Lufia2ExecutionResult Lufia2PokerResetRound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PokerReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84c69cu);
    PokerGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84c6e7u), 0u};
    if (!PokerCall(&game, 0x84c69cu, 0x84a4fdu) ||
        !PokerCall(&game, 0x84c69fu, 0x84a4ddu))
        return game.result;
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, POKER_STAKE));
    for (unsigned seat = 0u; seat < 4u; ++seat)
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(POKER_STAKES + seat * 2u)));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_SPECIAL_MENU_WINDOW_DEPTH & 0xffffu));
    OpStz(memory, cpu, OpAbs(cpu, POKER_WINDOW_STYLE));
    OpStz(memory, cpu, OpAbs(cpu, POKER_CURSOR_DEPTH));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, POKER_ROUNDS));
    OpLdx(cpu, 0u);
    OpLdy(cpu, 4u);
    OpLoadA(cpu, 1u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, POKER_SEATS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpStz(memory, cpu, OpAbs(cpu, POKER_ROUND_STATUS));
    OpLoadA(cpu, 0xaaaau);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_OAM_HIGH_BUFFER + 6u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_OAM_HIGH_BUFFER + 8u));
    OpStz(memory, cpu, OpDp(cpu, 0x15u));
    return ExecutionReturned(0x84c6e7u);
}

Lufia2ExecutionResult Lufia2PokerRevealOpponents(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PokerReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84c73au);
    PokerGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84c793u), 0u};
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    do {
        OpLda(memory, cpu, OpDp(cpu, 2u));
        cpu->carry = 0u;
        RolA16(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x84b79bu));
        OpSta(memory, cpu, OpDp(cpu, 0x24u));
        OpLda(memory, cpu, OpAbsX(cpu, POKER_COUNTS));
        OpDecA(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0x13u));
        OpTxa(cpu);
        cpu->carry = 0u;
        RolA16(cpu);
        RolA16(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, 0u));
        OpTax(cpu);
        OpWriteX(memory, cpu, OpDp(cpu, 0x11u), cpu->x);
        do {
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpDp(cpu, 0x28u));
            OpLda(memory, cpu, OpAbsX(cpu, POKER_HANDS));
            OpSta(memory, cpu, OpDp(cpu, 0x22u));
            if (!PokerCall(&game, 0x84c766u, 0x84b2bcu))
                return game.result;
            OpLoadA(cpu, 0x20u);
            if (!PokerCall(&game, 0x84c76cu, 0x84a607u) ||
                !PokerCall(&game, 0x84c76fu, 0x84b387u))
                return game.result;
            OpLda(memory, cpu, OpDp(cpu, 0x24u));
            cpu->carry = 1u;
            OpSbcValue(cpu, 62u);
            OpSta(memory, cpu, OpDp(cpu, 0x24u));
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x11u)));
            OpInx(cpu);
            OpInx(cpu);
            OpWriteX(memory, cpu, OpDp(cpu, 0x11u), cpu->x);
            OpStepMem(memory, cpu, OpDp(cpu, 0x13u), -1);
        } while (!cpu->negative);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, 0x74u));
        if (!PokerCall(&game, 0x84c78au, 0x84a5a8u))
            return game.result;
        OpRepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x84c793u);
}

Lufia2ExecutionResult Lufia2PokerWaitForInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PokerReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84c794u);
    PokerGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84c7a5u), 0u};
    do {
        if (!PokerCall(&game, 0x84c794u, 0x84a5a8u) ||
            !PokerCall(&game, 0x84c797u, 0x849f1au))
            return game.result;
        OpCmpValue(cpu, 0u);
    } while (cpu->zero);
    OpLoadA(cpu, 2u);
    if (!PokerCall(&game, 0x84c7a2u, 0x84a607u))
        return game.result;
    return ExecutionReturned(0x84c7a5u);
}

Lufia2ExecutionResult Lufia2PokerDrawCarryover(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PokerReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84c819u);
    PokerGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84c825u), 0u};
    OpLda(memory, cpu, OpAbs(cpu, POKER_CARRYOVER));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdy(cpu, 0xfd30u);
    if (!PokerCall(&game, 0x84c821u, 0x808878u))
        return game.result;
    return ExecutionReturned(0x84c825u);
}
