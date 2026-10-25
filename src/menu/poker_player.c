#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/poker.h"
#include "menu/arithmetic_wait.h"
#include "system/wram.h"

enum {
    PLAYER_VALUE = 0u,
    PLAYER_DIGITS = 2u,
    PLAYER_RAISE = 0x0bu,
    PLAYER_TOTAL = 0x0eu,
    PLAYER_OAM_CURSOR = 0x2du,
    PLAYER_EXIT_FLAGS = 0x72u,
    PLAYER_TEXT_DELAY = 0x74u,
    PLAYER_BALANCE = WRAM_MENU_EXCHANGE_BALANCE & 0xffffu,
    PLAYER_STAKE = WRAM_POKER_ROUND_STAKE & 0xffffu,
    PLAYER_ACCEPTED = WRAM_CARD_GAME_ACCEPTED_STAKE & 0xffffu,
    PLAYER_HAND = WRAM_CARD_GAME_HAND_WINS & 0xffffu,
    ROUND_STATUS = WRAM_POKER_ROUND_STATUS & 0xffffu,
    TABLE_STAKE = WRAM_CARD_GAME_STAKE & 0xffffu,
    RAISE_LIMIT = WRAM_CARD_GAME_STAKE_LIMIT & 0xffffu,
    PENDING_RAISE = WRAM_CARD_GAME_RESERVED_STAKE & 0xffffu,
    MULTIPLY_LEFT = WRAM_SPECIAL_MENU_MULTIPLY_LEFT & 0xffffu,
    MULTIPLY_RIGHT = WRAM_SPECIAL_MENU_MULTIPLY_RIGHT & 0xffffu,
    MULTIPLY_PRODUCT = WRAM_SPECIAL_MENU_MULTIPLY_RESULT & 0xffffu,
    PLAYER_WINDOW = WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu,
    PLAYER_CALL_LIMIT = 4096u
};

typedef struct PlayerTurn {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} PlayerTurn;

static bool PlayerReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static bool PlayerCall(PlayerTurn *turn, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = turn->cpu;
    const uint8_t width = cpu->accumulator_is_8_bit;
    const uint8_t frame = target >> 16 == 0x84u ? 2u : 3u;
    if (turn->calls++ == PLAYER_CALL_LIMIT) {
        turn->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(turn->memory, cpu, turn->child, turn->context,
                            site, target, frame, 0x84u)) {
        turn->result = ExecutionReturned(site);
        turn->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    const bool normalizes_width = site == 0x84c078u || site == 0x84c0c1u ||
        site == 0x84c16cu || site == 0x84c187u || site == 0x84c1a2u || site == 0x84c1e2u;
    if (!PlayerReady(cpu) || (!normalizes_width && cpu->accumulator_is_8_bit != width)) {
        turn->result = ExecutionHandoff(cpu, (uint32_t)cpu->program_bank << 16 |
            (uint16_t)(site + frame + 1u));
        return false;
    }
    return true;
}

static bool ChargePlayerStake(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, PLAYER_STAKE));
    OpOraValue(cpu, 0x0a00u);
    OpSta(memory, cpu, SNES_WRMPYA);
    WaitForArithmetic(memory, cpu, 0x84c032u, 4u);
    OpLda(memory, cpu, SNES_RDMPYL);
    OpSta(memory, cpu, OpDp(cpu, PLAYER_VALUE));
    OpLda(memory, cpu, OpAbs(cpu, PLAYER_BALANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PLAYER_VALUE)));
    if (!cpu->carry) {
        OpStepMem(memory, cpu, OpAbs(cpu, PLAYER_BALANCE + 2u), -1);
        if (cpu->negative)
            return false;
        OpLda(memory, cpu, OpAbs(cpu, PLAYER_BALANCE));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PLAYER_VALUE)));
    }
    OpSta(memory, cpu, OpAbs(cpu, PLAYER_BALANCE));
    return true;
}

static bool OpenPlayerWindow(PlayerTurn *turn, uint16_t top, uint16_t height,
                              uint32_t site) {
    Lufia2CpuState *cpu = turn->cpu;
    OpLoadA(cpu, 10u);
    OpSta(turn->memory, cpu, OpAbs(cpu, PLAYER_WINDOW));
    OpLoadA(cpu, top);
    OpSta(turn->memory, cpu, OpAbs(cpu, PLAYER_WINDOW + 2u));
    OpLoadA(cpu, 12u);
    OpSta(turn->memory, cpu, OpAbs(cpu, PLAYER_WINDOW + 4u));
    OpLoadA(cpu, height);
    OpSta(turn->memory, cpu, OpAbs(cpu, PLAYER_WINDOW + 6u));
    return PlayerCall(turn, site, 0x84a2b3u);
}

static bool ShowPlayerShortfall(PlayerTurn *turn) {
    const Lufia2Memory *memory = turn->memory;
    Lufia2CpuState *cpu = turn->cpu;
    OpStz(memory, cpu, OpAbs(cpu, PLAYER_BALANCE + 2u));
    if (!OpenPlayerWindow(turn, 11u, 7u, 0x84c06fu))
        return false;
    OpLdx(cpu, 0x3316u);
    OpLdy(cpu, 0xfb4eu);
    if (!PlayerCall(turn, 0x84c078u, 0x808878u))
        return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, PLAYER_TEXT_DELAY));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x80a0u);
    return PlayerCall(turn, 0x84c087u, 0x84c64bu);
}

static bool OpenPlayerRaise(PlayerTurn *turn) {
    const Lufia2Memory *memory = turn->memory;
    Lufia2CpuState *cpu = turn->cpu;
    OpLoadA(cpu, 99u);
    OpSta(memory, cpu, OpAbs(cpu, RAISE_LIMIT));
    if (!PlayerCall(turn, 0x84c093u, 0x84c578u))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, PLAYER_STAKE));
    OpSta(memory, cpu, OpDp(cpu, PLAYER_VALUE));
    OpStz(memory, cpu, OpDp(cpu, PLAYER_RAISE));
    OpStz(memory, cpu, OpAbs(cpu, PENDING_RAISE));
    if (!OpenPlayerWindow(turn, 15u, 6u, 0x84c0b8u))
        return false;
    OpLdx(cpu, 0x3416u);
    OpLdy(cpu, 0xf96du);
    return PlayerCall(turn, 0x84c0c1u, 0x808878u);
}

static bool LimitPlayerRaise(PlayerTurn *turn) {
    const Lufia2Memory *memory = turn->memory;
    Lufia2CpuState *cpu = turn->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, PLAYER_RAISE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, PENDING_RAISE)));
    if (!cpu->carry)
        return true;
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLY_LEFT));
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLY_RIGHT));
    if (!PlayerCall(turn, 0x84c0d8u, 0x84a5dfu))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, PLAYER_BALANCE + 2u));
    if (!cpu->zero)
        return true;
    OpLda(memory, cpu, OpAbs(cpu, PLAYER_BALANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, MULTIPLY_PRODUCT)));
    if (cpu->carry)
        return true;
    OpLda(memory, cpu, OpAbs(cpu, PLAYER_BALANCE));
    if (!cpu->zero) {
        OpSta(memory, cpu, SNES_WRDIVL);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 10u);
        OpSta(memory, cpu, SNES_WRDIVB);
        WaitForArithmetic(memory, cpu, 0x84c0fau, 8u);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, SNES_RDDIVL);
    } else {
        OpLda(memory, cpu, OpAbs(cpu, PENDING_RAISE));
    }
    OpSta(memory, cpu, OpDp(cpu, PLAYER_RAISE));
    return true;
}

static bool DrawPlayerRaise(PlayerTurn *turn) {
    const Lufia2Memory *memory = turn->memory;
    Lufia2CpuState *cpu = turn->cpu;
    OpLda(memory, cpu, OpAbs(cpu, PLAYER_STAKE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, PLAYER_RAISE));
    OpCmpValue(cpu, 99u);
    if (!cpu->negative) {
        OpLoadA(cpu, 99u);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, PLAYER_STAKE)));
        OpSta(memory, cpu, OpDp(cpu, PLAYER_RAISE));
        OpLoadA(cpu, 99u);
    }
    OpSta(memory, cpu, OpDp(cpu, PLAYER_TOTAL));
    if (!PlayerCall(turn, 0x84c12au, 0x84c31bu))
        return false;
    OpLda(memory, cpu, OpDp(cpu, PLAYER_TOTAL));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PLAYER_DIGITS)));
    OpSta(memory, cpu, OpDp(cpu, PLAYER_DIGITS));
    OpStz(memory, cpu, OpDp(cpu, PLAYER_OAM_CURSOR));
    OpLda(memory, cpu, OpDp(cpu, PLAYER_VALUE));
    OpLdy(cpu, 0x8870u);
    OpLoadA(cpu, 0x3600u);
    if (!PlayerCall(turn, 0x84c13eu, 0x84c2ceu))
        return false;
    OpLda(memory, cpu, OpDp(cpu, PLAYER_DIGITS));
    OpSta(memory, cpu, OpDp(cpu, PLAYER_VALUE));
    OpLdy(cpu, 0x8880u);
    OpLoadA(cpu, 0x3400u);
    if (!PlayerCall(turn, 0x84c14bu, 0x84c2ceu))
        return false;
    OpLda(memory, cpu, OpDp(cpu, PLAYER_RAISE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, PLAYER_STAKE));
    OpSta(memory, cpu, OpDp(cpu, PLAYER_VALUE));
    OpLdx(cpu, 0x3416u);
    OpLdy(cpu, 0xf96du);
    return PlayerCall(turn, 0x84c15cu, 0x808878u);
}

static bool SettlePlayerRaise(PlayerTurn *turn) {
    const Lufia2Memory *memory = turn->memory;
    Lufia2CpuState *cpu = turn->cpu;
    OpLda(memory, cpu, OpAbs(cpu, PENDING_RAISE));
    OpCmp(memory, cpu, OpDp(cpu, PLAYER_RAISE));
    if (!cpu->zero) {
        Push8(memory, cpu, PackStatus(cpu));
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x21u);
        if (!PlayerCall(turn, 0x84c16cu, 0x84a607u))
            return false;
        UnpackStatus(cpu, Pull8(memory, cpu));
        if (!PlayerReady(cpu) || cpu->accumulator_is_8_bit) {
            turn->result = ExecutionHandoff(cpu, 0x84c170u);
            return false;
        }
        if (cpu->negative) {
            if (!PlayerCall(turn, 0x84c172u, 0x84c20au))
                return false;
        } else if (!PlayerCall(turn, 0x84c177u, 0x84c1e8u)) {
            return false;
        }
    }
    return PlayerCall(turn, 0x84c17au, 0x84af11u);
}

static bool ReadPlayerCommand(PlayerTurn *turn) {
    const Lufia2Memory *memory = turn->memory;
    Lufia2CpuState *cpu = turn->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, PLAYER_EXIT_FLAGS));
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, PLAYER_TEXT_DELAY));
    if (!PlayerCall(turn, 0x84c187u, 0x84a5a8u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, PLAYER_RAISE));
    OpSta(memory, cpu, OpAbs(cpu, PENDING_RAISE));
    return PlayerCall(turn, 0x84c191u, 0x84c350u);
}

static void FoldPlayerHand(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpAbs(cpu, PLAYER_HAND));
    OpStepMem(memory, cpu, OpAbs(cpu, ROUND_STATUS), 1);
}

static bool CommitPlayerRaise(PlayerTurn *turn) {
    const Lufia2Memory *memory = turn->memory;
    Lufia2CpuState *cpu = turn->cpu;
    OpLda(memory, cpu, OpDp(cpu, PLAYER_RAISE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, PLAYER_STAKE));
    OpSta(memory, cpu, OpAbs(cpu, PLAYER_STAKE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, TABLE_STAKE));
    OpSta(memory, cpu, OpAbs(cpu, PLAYER_ACCEPTED));
    return PlayerCall(turn, 0x84c1c6u, 0x84c243u);
}

static bool FinishPlayerTurn(PlayerTurn *turn) {
    if (!PlayerCall(turn, 0x84c1c9u, 0x84c22cu) ||
        !PlayerCall(turn, 0x84c1ccu, 0x84c26fu) ||
        !PlayerCall(turn, 0x84c1cfu, 0x84a39bu) ||
        !PlayerCall(turn, 0x84c1d2u, 0x84c578u) ||
        !PlayerCall(turn, 0x84c1d5u, 0x84af11u))
        return false;
    Lufia2CpuState *cpu = turn->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(turn->memory, cpu, OpDp(cpu, PLAYER_TEXT_DELAY));
    OpLoadA(cpu, 0x80u);
    OpSta(turn->memory, cpu, OpDp(cpu, PLAYER_EXIT_FLAGS));
    if (!PlayerCall(turn, 0x84c1e2u, 0x84a5a8u))
        return false;
    OpRepWidths(cpu, 0x20u);
    return true;
}

Lufia2ExecutionResult Lufia2PokerTakePlayerTurn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !PlayerReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84c028u);
    PlayerTurn turn = {memory, cpu, child, context,
                       ExecutionReturned(0x84c1e7u), 0u};
    if (!ChargePlayerStake(memory, cpu)) {
        if (!ShowPlayerShortfall(&turn))
            return turn.result;
        FoldPlayerHand(memory, cpu);
    } else {
        if (!OpenPlayerRaise(&turn))
            return turn.result;
        do {
            if (!LimitPlayerRaise(&turn) || !DrawPlayerRaise(&turn) ||
                !SettlePlayerRaise(&turn) || !ReadPlayerCommand(&turn))
                return turn.result;
            OpCmpValue(cpu, 3u);
        } while (cpu->zero);
        OpSta(memory, cpu, OpDp(cpu, PLAYER_VALUE));
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 2u);
        if (!PlayerCall(&turn, 0x84c1a2u, 0x84a607u))
            return turn.result;
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, PLAYER_VALUE));
        OpCmpValue(cpu, 2u);
        if (cpu->zero)
            FoldPlayerHand(memory, cpu);
        else if (!CommitPlayerRaise(&turn))
            return turn.result;
    }
    FinishPlayerTurn(&turn);
    return turn.result;
}
