#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/card_game.h"
#include "system/wram.h"

enum {
    CARD_DECK = WRAM_CARD_GAME_DECK & 0xffffu,
    CARD_HANDS = WRAM_CARD_GAME_HANDS & 0xffffu,
    CARD_COUNTS = WRAM_CARD_GAME_HAND_COUNTS & 0xffffu,
    CARD_TOTALS = WRAM_CARD_GAME_HAND_TOTALS & 0xffffu,
    CARD_RANKS = WRAM_CARD_GAME_HAND_RANKS & 0xffffu,
    CARD_WINS = WRAM_CARD_GAME_HAND_WINS & 0xffffu,
    CARD_REPLAY_CURSOR = WRAM_CARD_GAME_REPLAY_CURSOR & 0xffffu,
    CARD_WINDOW_DEPTH = WRAM_SPECIAL_MENU_WINDOW_DEPTH & 0xffffu,
    CARD_WINDOW_STATE = WRAM_SPECIAL_MENU_WINDOW_STATE & 0xffffu,
    CARD_WINDOW_LEFT = (WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu) + 0u,
    CARD_WINDOW_TOP = (WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu) + 2u,
    CARD_WINDOW_WIDTH = (WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu) + 4u,
    CARD_WINDOW_HEIGHT = (WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu) + 6u,
    CARD_WINDOW_STYLE = WRAM_SPECIAL_MENU_WINDOW_STYLE & 0xffffu,
    CARD_CURSOR_DEPTH = WRAM_SPECIAL_MENU_CURSOR_DEPTH & 0xffffu,
    CARD_CURSOR_XY = (WRAM_SPECIAL_MENU_CURSOR_DEFINITION & 0xffffu) + 12u,
    CARD_CURSOR_STEP = (WRAM_SPECIAL_MENU_CURSOR_DEFINITION & 0xffffu) + 14u,
    CARD_CURSOR_SELECTION = WRAM_SPECIAL_MENU_CURSOR_SELECTIONS & 0xffffu,
    CARD_CURSOR_OAM = (WRAM_SPECIAL_MENU_CURSOR_OAM & 0xffffu) + 0u,
    CARD_CURSOR_OAM_START = (WRAM_SPECIAL_MENU_CURSOR_OAM & 0xffffu) + 2u,
    CARD_BALANCE = WRAM_MENU_EXCHANGE_BALANCE & 0xffffu,
    CARD_CALL_LIMIT = 65536u
};

typedef struct CardGameSession {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} CardGameSession;

static bool CardControlReady(const Lufia2CpuState *cpu, bool byte_accumulator) {
    return cpu->program_bank == 0x84u &&
           cpu->accumulator_is_8_bit == byte_accumulator &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

static bool CardCall(CardGameSession *game, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = game->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = (target >> 16) == 0x84u ? 2u : 3u;
    if (game->calls++ == CARD_CALL_LIMIT) {
        game->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(game->memory, cpu, game->child, game->context,
                            site, target, frame, 0x84u)) {
        game->result = ExecutionReturned(site);
        game->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!CardControlReady(cpu, byte_accumulator)) {
        game->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static bool OpenCardRound(CardGameSession *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpRepWidths(cpu, 0x20u);
    if (!CardCall(game, 0x84a71du, 0x84b4d9u))
        return false;
    OpStz(memory, cpu, OpAbs(cpu, CARD_WINDOW_DEPTH));
    OpStz(memory, cpu, OpAbs(cpu, CARD_WINDOW_STYLE));
    OpStz(memory, cpu, OpAbs(cpu, CARD_CURSOR_DEPTH));
    if (!CardCall(game, 0x84a729u, 0x84af4bu) ||
        !CardCall(game, 0x84a72cu, 0x84ac66u))
        return false;
    OpSepWidths(cpu, 0x20u);
    if (!CardCall(game, 0x84a731u, 0x84aee1u))
        return false;
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!CardCall(game, 0x84a738u, 0x84a5a8u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, 0x17u));
    static const uint32_t sites[] = {
        0x84a73fu, 0x84a744u, 0x84a749u, 0x84a74eu};
    for (unsigned hand = 0u; hand < 4u; ++hand) {
        if (!CardCall(game, sites[hand], 0x84b18eu))
            return false;
        if (hand != 3u)
            OpStepMem(memory, cpu, OpDp(cpu, 0x17u), 1);
    }
    if (!CardCall(game, 0x84a751u, 0x84adf3u))
        return false;
    OpStz(memory, cpu, OpDp(cpu, 0x17u));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    if (!CardCall(game, 0x84a75bu, 0x84ae5eu))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpCmpValue(cpu, 21u);
    return true;
}

static bool PlayFirstCardHand(CardGameSession *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x0au));
    OpLdx(cpu, 0xb7e5u);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0xb068u);
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_CURSOR_XY), cpu->x);
    if (!CardCall(game, 0x84a77bu, 0x84a02au))
        return false;
    for (;;) {
        if (!CardCall(game, 0x84a77eu, 0x849f1au))
            return false;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            if (!CardCall(game, 0x84a789u, 0x84a607u) ||
                !CardCall(game, 0x84a78cu, 0x84a0dau) ||
                !CardCall(game, 0x84a78fu, 0x84b18eu) ||
                !CardCall(game, 0x84a792u, 0x84adf3u))
                return false;
            OpLda(memory, cpu, OpAbs(cpu, CARD_COUNTS));
            OpCmpValue(cpu, 5u);
            if (cpu->zero)
                return true;
            OpStz(memory, cpu, OpDp(cpu, 2u));
            if (!CardCall(game, 0x84a79fu, 0x84ae5eu))
                return false;
            OpLoadA(cpu, 21u);
            OpCmp(memory, cpu, OpDp(cpu, 0u));
            if (cpu->negative)
                return true;
            OpLda(memory, cpu, OpAbs(cpu, CARD_CURSOR_XY));
            cpu->carry = 0u;
            OpAdcValue(cpu, 32u);
            OpSta(memory, cpu, OpAbs(cpu, CARD_CURSOR_XY));
            if (!CardCall(game, 0x84a7b3u, 0x84a02au))
                return false;
        } else {
            OpCmpValue(cpu, 2u);
            if (cpu->zero) {
                OpLoadA(cpu, 1u);
                return CardCall(game, 0x84a7c0u, 0x84a607u) &&
                       CardCall(game, 0x84a7c3u, 0x84a0dau);
            }
        }
    }
}

static void ForecastCardTotal(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x15u)));
    OpLda(memory, cpu, OpAbsY(cpu, CARD_DECK));
    OpAndValue(cpu, 0x00ffu);
    OpCmpValue(cpu, 10u);
    if (!cpu->negative)
        OpLoadA(cpu, 10u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLoadA(cpu, 21u);
    OpCmp(memory, cpu, OpDp(cpu, 2u));
}

static bool PlayMiddleCardHands(CardGameSession *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpStepMem(memory, cpu, OpDp(cpu, 0x17u), 1);
    do {
        for (;;) {
            OpLda(memory, cpu, OpDp(cpu, 0x17u));
            OpSta(memory, cpu, OpDp(cpu, 2u));
            if (!CardCall(game, 0x84a7ccu, 0x84ae5eu))
                return false;
            OpLda(memory, cpu, OpDp(cpu, 0u));
            OpCmpValue(cpu, 21u);
            if (!cpu->negative)
                break;
            ForecastCardTotal(memory, cpu);
            if (cpu->negative) {
                OpLoadA(cpu, 100u);
                if (!CardCall(game, 0x84a7f7u, 0x808299u))
                    return false;
                OpCmpValue(cpu, 70u);
                if (cpu->negative)
                    break;
            }
            if (!CardCall(game, 0x84a800u, 0x84b18eu))
                return false;
            OpLda(memory, cpu, OpDp(cpu, 0x17u));
            cpu->carry = 0u;
            RolA16(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpAbsX(cpu, CARD_COUNTS));
            OpCmpValue(cpu, 5u);
            if (cpu->zero)
                break;
        }
        OpStepMem(memory, cpu, OpDp(cpu, 0x17u), 1);
        OpLoadA(cpu, 2u);
        OpCmp(memory, cpu, OpDp(cpu, 0x17u));
    } while (!cpu->negative);
    return true;
}

static bool PlayLastCardHand(CardGameSession *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    for (;;) {
        OpLda(memory, cpu, OpDp(cpu, 0x17u));
        OpSta(memory, cpu, OpDp(cpu, 2u));
        if (!CardCall(game, 0x84a81du, 0x84ae5eu))
            return false;
        OpLda(memory, cpu, OpDp(cpu, 0u));
        OpCmpValue(cpu, 17u);
        if (!cpu->negative) {
            OpLoadA(cpu, 100u);
            if (!CardCall(game, 0x84a82au, 0x808299u))
                return false;
            OpCmpValue(cpu, 0u);
            if (!cpu->zero) {
                OpLoadA(cpu, 10u);
                OpCmpValue(cpu, 0u);
                if (!cpu->zero)
                    return true;
            }
        } else {
            OpCmpValue(cpu, 21u);
            if (!cpu->negative)
                return true;
        }
        for (;;) {
            ForecastCardTotal(memory, cpu);
            if (!cpu->negative)
                break;
            OpLda(memory, cpu, OpDp(cpu, 0u));
            OpCmpValue(cpu, 17u);
            if (cpu->negative) {
                OpLoadA(cpu, 100u);
                OpCmpValue(cpu, 10u);
                if (cpu->negative)
                    break;
            }
            OpStepMem(memory, cpu, OpDp(cpu, 0x15u), 1);
            OpStepMem(memory, cpu, OpDp(cpu, 0x15u), 1);
            OpLda(memory, cpu, OpDp(cpu, 0x15u));
            OpCmpValue(cpu, 60u);
            if (!cpu->negative)
                return true;
        }
        if (!CardCall(game, 0x84a87au, 0x84b18eu))
            return false;
        OpLda(memory, cpu, OpDp(cpu, 0x17u));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, CARD_COUNTS));
        OpCmpValue(cpu, 5u);
        if (cpu->zero)
            return true;
    }
}

static bool EvaluateCardHands(CardGameSession *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpStz(memory, cpu, OpDp(cpu, 2u));
    do {
        if (!CardCall(game, 0x84a891u, 0x84ae5eu))
            return false;
        OpLda(memory, cpu, OpDp(cpu, 2u));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0u));
        OpSta(memory, cpu, OpAbsX(cpu, CARD_TOTALS));
        OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->negative);
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    if (!CardCall(game, 0x84a8a9u, 0x84ab61u))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpAbs(cpu, CARD_RANKS));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, 0x0bu));
    OpStz(memory, cpu, OpDp(cpu, 2u));
    do {
        if (!CardCall(game, 0x84a8b8u, 0x84ab61u))
            return false;
        OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
        OpLda(memory, cpu, OpDp(cpu, 2u));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0u));
        OpSta(memory, cpu, OpAbsX(cpu, CARD_RANKS));
        OpStepMem(memory, cpu, OpDp(cpu, 0x0bu), -1);
    } while (!cpu->negative);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 2u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, CARD_WINS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpLda(memory, cpu, OpAbs(cpu, CARD_RANKS));
    for (unsigned hand = 0u; hand < 3u; ++hand) {
        OpCmp(memory, cpu, OpAbs(cpu, (uint16_t)(CARD_RANKS + 2u + hand * 2u)));
        if (cpu->negative)
            OpStepMem(memory, cpu, OpAbs(cpu, (uint16_t)(CARD_WINS + hand * 2u)), 1);
    }
    OpLda(memory, cpu, OpAbs(cpu, CARD_RANKS));
    if (!cpu->zero)
        return true;
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    if (!CardCall(game, 0x84a8feu, 0x84ae5eu))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpAbs(cpu, CARD_RANKS));
    OpLoadA(cpu, 21u);
    OpCmp(memory, cpu, OpDp(cpu, 0u));
    if (cpu->negative)
        OpStz(memory, cpu, OpAbs(cpu, CARD_RANKS));
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, 2u), cpu->x);
    OpLdy(cpu, 2u);
    OpWriteX(memory, cpu, OpDp(cpu, 4u), cpu->y);
    do {
        if (!CardCall(game, 0x84a91au, 0x84ae5eu))
            return false;
        OpLda(memory, cpu, OpDp(cpu, 2u));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        OpLoadA(cpu, 21u);
        OpCmp(memory, cpu, OpDp(cpu, 0u));
        if (!cpu->negative) {
            OpLda(memory, cpu, OpAbs(cpu, CARD_RANKS));
            OpCmp(memory, cpu, OpDp(cpu, 0u));
            if (cpu->negative)
                OpStepMem(memory, cpu, OpAbsX(cpu, CARD_WINS), 1);
        }
        OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->negative);
    return true;
}

static bool RevealCardHands(CardGameSession *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
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
        OpLda(memory, cpu, OpAbsX(cpu, CARD_COUNTS));
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
            OpLda(memory, cpu, OpAbsX(cpu, CARD_HANDS));
            OpSta(memory, cpu, OpDp(cpu, 0x22u));
            if (!CardCall(game, 0x84a965u, 0x84b2bcu))
                return false;
            OpLoadA(cpu, 32u);
            if (!CardCall(game, 0x84a96bu, 0x84a607u) ||
                !CardCall(game, 0x84a96eu, 0x84b387u))
                return false;
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
        if (!CardCall(game, 0x84a989u, 0x84a5a8u))
            return false;
        OpRepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
    } while (!cpu->zero);
    return true;
}

static bool ShowCardResults(CardGameSession *game) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x0au));
    OpLdx(cpu, 0xb7abu);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    if (!CardCall(game, 0x84a9a4u, 0x84ab61u))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpSta(memory, cpu, OpAbs(cpu, CARD_RANKS));
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 6u));
    OpTay(cpu);
    OpLoadA(cpu, 0x070eu);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLda(memory, cpu, OpAbs(cpu, CARD_TOTALS + 6u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    if (!CardCall(game, 0x84a9beu, 0x84aab5u))
        return false;
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    do {
        OpLda(memory, cpu, OpDp(cpu, 4u));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, CARD_TOTALS));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLda(memory, cpu, OpAbsX(cpu, CARD_RANKS + 2u));
        cpu->carry = 0u;
        RolA16(cpu);
        OpSta(memory, cpu, OpDp(cpu, 6u));
        RolA16(cpu);
        OpAdc(memory, cpu, OpDp(cpu, 6u));
        OpTay(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x84b7d5u));
        OpSta(memory, cpu, OpDp(cpu, 2u));
        if (!CardCall(game, 0x84a9e1u, 0x84aab5u))
            return false;
        OpStepMem(memory, cpu, OpDp(cpu, 4u), 1);
        OpLoadA(cpu, 3u);
        OpCmp(memory, cpu, OpDp(cpu, 4u));
    } while (!cpu->zero);
    return true;
}

static void SetCardWindow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t height) {
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_LEFT));
    OpLoadA(cpu, 11u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_TOP));
    OpLoadA(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_WIDTH));
    OpLoadA(cpu, height);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_HEIGHT));
}

static bool ResolveCardRound(CardGameSession *game, bool *replay) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    if (!CardCall(game, 0x84a9edu, 0x84ab01u))
        return false;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    if (!CardCall(game, 0x84a9fau, 0x84a5a8u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, CARD_WINS));
    if (cpu->zero) {
        OpLoadA(cpu, 41u);
        if (!CardCall(game, 0x84aa07u, 0x84a607u))
            return false;
    } else {
        OpLoadA(cpu, 40u);
        if (!CardCall(game, 0x84aa0fu, 0x84a607u))
            return false;
        do {
            if (!CardCall(game, 0x84aa12u, 0x849f1au))
                return false;
            OpCmpValue(cpu, 1u);
        } while (!cpu->zero);
        OpLoadA(cpu, 2u);
        if (!CardCall(game, 0x84aa1du, 0x84a607u) ||
            !CardCall(game, 0x84aa20u, 0x84ad9du))
            return false;
    }
    SetCardWindow(memory, cpu, 6u);
    if (!CardCall(game, 0x84aa3bu, 0x84a2b3u))
        return false;
    OpLdx(cpu, 0x3316u);
    OpLdy(cpu, 0xfa2du);
    if (!CardCall(game, 0x84aa44u, 0x808878u))
        return false;
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!CardCall(game, 0x84aa4du, 0x84a5a8u))
        return false;
    OpLdx(cpu, 0x7868u);
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_REPLAY_CURSOR), cpu->x);
    OpLdx(cpu, 40u);
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_REPLAY_CURSOR + 2u), cpu->x);
    if (!CardCall(game, 0x84aa5cu, 0x84ab23u))
        return false;
    OpLda(memory, cpu, OpDp(cpu, 0u));
    if (!cpu->zero)
        return true;
    if (!CardCall(game, 0x84aa63u, 0x84a39bu))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, CARD_BALANCE));
    OpOra(memory, cpu, OpAbs(cpu, CARD_BALANCE + 2u));
    OpSepWidths(cpu, 0x20u);
    if (cpu->zero) {
        OpLoadA(cpu, 10u);
        OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_TOP));
        OpLoadA(cpu, 7u);
        OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_HEIGHT));
        if (!CardCall(game, 0x84aa7au, 0x84a2b3u))
            return false;
        OpLdx(cpu, 0x32d6u);
        OpLdy(cpu, 0xfa0au);
        if (!CardCall(game, 0x84aa83u, 0x808878u))
            return false;
        OpLoadA(cpu, 0x88u);
        OpSta(memory, cpu, OpDp(cpu, 0x74u));
        if (!CardCall(game, 0x84aa8bu, 0x84a5a8u))
            return false;
        do {
            if (!CardCall(game, 0x84aa8eu, 0x849f1au))
                return false;
            OpCmpValue(cpu, 0u);
        } while (cpu->zero);
        OpLoadA(cpu, 2u);
        return CardCall(game, 0x84aa9au, 0x84a607u);
    }
    if (!CardCall(game, 0x84aa9fu, 0x84a4fdu) ||
        !CardCall(game, 0x84aaa2u, 0x84a4ddu))
        return false;
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    if (!CardCall(game, 0x84aaa9u, 0x84a5a8u))
        return false;
    *replay = true;
    return true;
}

Lufia2ExecutionResult Lufia2CardGameRun(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardControlReady(cpu, true))
        return ExecutionHandoff(cpu, 0x84a705u);
    CardGameSession game = {memory, cpu, child, context,
                            ExecutionReturned(0x84aab4u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    if (!CardCall(&game, 0x84a70du, 0x84b57fu))
        return game.result;
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_WINDOW_STATE), cpu->x);
    bool replay;
    do {
        replay = false;
        if (!OpenCardRound(&game))
            return game.result;
        if (!cpu->zero &&
            (!PlayFirstCardHand(&game) || !PlayMiddleCardHands(&game) ||
             !PlayLastCardHand(&game)))
            return game.result;
        if (!EvaluateCardHands(&game) || !RevealCardHands(&game) ||
            !ShowCardResults(&game) || !ResolveCardRound(&game, &replay))
            return game.result;
    } while (replay);
    if (!CardCall(&game, 0x84aaafu, 0x84a5cau))
        return game.result;
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84aab4u);
}

Lufia2ExecutionResult Lufia2CardGameShowHandResult(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardControlReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84aab5u);
    CardGameSession game = {memory, cpu, child, context,
                            ExecutionReturned(0x84ab00u), 0u};
    OpLda(memory, cpu, OpDp(cpu, 2u));
    OpAndValue(cpu, 0x00ffu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, DirectLongIndirectY(memory, cpu, 8u)));
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_LEFT));
    OpLda(memory, cpu, OpDp(cpu, 2u));
    ExchangeAccumulatorBytes(cpu);
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_TOP));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 8u));
    cpu->carry = 0u;
    RolA16(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 4u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_WIDTH));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 8u));
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_HEIGHT));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 8u));
    OpSta(memory, cpu, OpDp(cpu, 2u));
    if (!CardCall(&game, 0x84aae1u, 0x84a2b3u))
        return game.result;
    OpLda(memory, cpu, OpAbs(cpu, CARD_WINDOW_TOP));
    OpIncA(cpu);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    RorA16(cpu);
    RorA16(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, CARD_WINDOW_LEFT));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CARD_WINDOW_LEFT));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x3000u);
    OpIncA(cpu);
    OpIncA(cpu);
    OpTax(cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 2u)));
    if (!CardCall(&game, 0x84aafcu, 0x808878u))
        return game.result;
    return ExecutionReturned(0x84ab00u);
}

Lufia2ExecutionResult Lufia2CardGamePrepareResultSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardControlReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84ab01u);
    CardGameSession game = {memory, cpu, child, context,
                            ExecutionReturned(0x84ab22u), 0u};
    OpLoadA(cpu, 0xaaaau);
    OpSta(memory, cpu, OpAbs(cpu, 0x0306u));
    OpSta(memory, cpu, OpAbs(cpu, 0x0308u));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0u);
    OpTax(cpu);
    OpLdy(cpu, 96u);
    do {
        OpLoadA(cpu, 2u);
        if (!CardCall(&game, 0x84ab19u, 0x84c6e8u))
            return game.result;
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x84ab22u);
}

Lufia2ExecutionResult Lufia2CardGameChooseReplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardControlReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84ab23u);
    CardGameSession game = {memory, cpu, child, context,
                            ExecutionReturned(0x84ab60u), 0u};
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x01fcu);
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_CURSOR_OAM), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_CURSOR_OAM_START), cpu->x);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x0au));
    OpLdx(cpu, 0xb807u);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    if (!CardCall(&game, 0x84ab37u, 0x84a000u))
        return game.result;
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, CARD_REPLAY_CURSOR)));
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_CURSOR_XY), cpu->x);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, CARD_REPLAY_CURSOR + 2u)));
    OpWriteX(memory, cpu, OpAbs(cpu, CARD_CURSOR_STEP), cpu->x);
    if (!CardCall(&game, 0x84ab46u, 0x84a02au))
        return game.result;
    OpLoadA(cpu, 0u);
    if (!CardCall(&game, 0x84ab4bu, 0x84a112u))
        return game.result;
    PushAccumulator8(memory, cpu);
    if (!CardCall(&game, 0x84ab4fu, 0x84a0dau))
        return game.result;
    OpLda(memory, cpu, OpAbs(cpu, CARD_CURSOR_SELECTION));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, Pull8(memory, cpu));
    OpCmpValue(cpu, 0u);
    if (!cpu->zero)
        OpSta(memory, cpu, OpDp(cpu, 0u));
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x84ab60u);
}

Lufia2ExecutionResult Lufia2CardGamePreparePayout(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !CardControlReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84ad9du);
    CardGameSession game = {memory, cpu, child, context,
                            ExecutionReturned(0x84adbfu), 0u};
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_LEFT));
    OpLoadA(cpu, 11u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_TOP));
    OpLoadA(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, CARD_WINDOW_WIDTH));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, CARD_RANKS + 2u)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CARD_GAME_STAKE & 0xffffu));
    do {
        cpu->carry = 0u;
        RolA16(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    if (!CardCall(&game, 0x84adbcu, 0x84c7a6u))
        return game.result;
    return ExecutionReturned(0x84adbfu);
}
