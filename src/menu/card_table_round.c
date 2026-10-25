#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    ROUND_VARIANT = WRAM_CARD_TABLE_VARIANT_FLAGS & 0xffffu,
    ROUND_VALUE = WRAM_CARD_TABLE_STATE_7178 & 0xffffu,
    ROUND_NUMBER = WRAM_CARD_TABLE_PAYOUT_INDEX & 0xffffu,
    ROUND_NUMBER_SLOT = WRAM_CARD_TABLE_PAYOUT_STATE & 0xffffu,
    ROUND_CHANGED_DIGITS = WRAM_CARD_TABLE_PAYOUT_VALUE & 0xffffu,
    ROUND_PENDING_NUMBER = WRAM_CARD_TABLE_FIRST_NUMBER_PENDING & 0xffffu,
    ROUND_NUMBER_MOVEMENT = WRAM_CARD_TABLE_FIRST_NUMBER_MOVEMENT & 0xffffu,
    ROUND_COUNTDOWN = WRAM_CARD_TABLE_ROUND_COUNTDOWN & 0xffffu,
    ROUND_TOTAL = WRAM_CARD_TABLE_ROUND_TOTAL & 0xffffu,
    ROUND_SPECIAL_COUNTDOWN = WRAM_CARD_TABLE_SPECIAL_ROUND_COUNTDOWN & 0xffffu,
    ROUND_LAYOUT = (WRAM_POKER_ROUNDS_REMAINING - 1u) & 0xffffu,
    ROUND_CURSORS = WRAM_CARD_TABLE_PREVIOUS_HAND_WORDS & 0xffffu,
    ROUND_MATCH_COUNT = WRAM_CARD_TABLE_SELECTION_ACTIVE & 0xffffu,
    ROUND_MATCH_TYPES = (WRAM_POKER_RANK_ORDER + 5u) & 0xffffu,
    ROUND_MATCH_VALUES = WRAM_CARD_TABLE_MATCH_VALUES & 0xffffu,
    ROUND_MARKER = WRAM_CARD_TABLE_STATE_71AC & 0xffffu,
    ROUND_RANDOM_SELECTION = WRAM_CARD_TABLE_RANDOM_SELECTION & 0xffffu,
    ROUND_COLUMN_FLAGS = (WRAM_POKER_OPPONENT_VALUES + 3u) & 0xffffu,
    ROUND_SAVED_CURSOR = 0x00u,
    ROUND_SYMBOL = 0x02u,
    ROUND_SCRATCH = 0x04u,
    ROUND_SYMBOL_BASE = 0x08u,
    ROUND_SYMBOL_LIMIT = 0x0eu,
    ROUND_ATTEMPTS = 0x1cu,
    ROUND_SYMBOL_MODE = 0x1fu,
    ROUND_MATCH_TYPE = 0x2au,
    ROUND_MATCH_VALUE = 0x2du,
    ROUND_PASSES = 0x30u
};

Lufia2ExecutionResult Lufia2CardTableRefreshRoundNumber(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, false) ||
        cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84dc60u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84dc86u), false};
    OpLda(memory, cpu, OpAbs(cpu, ROUND_VARIANT));
    OpAndValue(cpu, 8u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, ROUND_VALUE));
        OpSta(memory, cpu, OpAbs(cpu, ROUND_NUMBER));
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, OpAbs(cpu, ROUND_NUMBER_SLOT));
        if (!TableServiceCall(&service, 0x84dc74u, 0x84eb36u, false))
            return service.result;
        OpLda(memory, cpu, OpAbs(cpu, ROUND_CHANGED_DIGITS));
        OpSta(memory, cpu, OpAbs(cpu, ROUND_NUMBER_MOVEMENT));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, ROUND_PENDING_NUMBER));
        if (!TableServiceCall(&service, 0x84dc83u, 0x84ebdbu, false))
            return service.result;
    }
    return service.result;
}

static void StoreRoundMarker(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t marker) {
    OpLoadA(cpu, marker);
    OpSta(memory, cpu, OpAbs(cpu, ROUND_MARKER));
}

static bool AdvanceOrdinaryRoundCounters(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpStepMem(memory, cpu, OpAbs(cpu, ROUND_TOTAL), -1);
    OpStepMem(memory, cpu, OpAbs(cpu, ROUND_COUNTDOWN), -1);
    if (cpu->zero) {
        OpStepMem(memory, cpu, OpAbs(cpu, ROUND_SPECIAL_COUNTDOWN), -1);
        if (cpu->zero) {
            OpStz(memory, cpu, OpAbs(cpu, ROUND_VARIANT));
            OpStz(memory, cpu, OpAbs(cpu, ROUND_COUNTDOWN));
            StoreRoundMarker(memory, cpu, 0x22u);
        } else {
            StoreRoundMarker(memory, cpu, 1u);
            OpLoadA(cpu, 8u);
            OpTestBits(memory, cpu, OpAbs(cpu, ROUND_VARIANT), true);
            OpLoadA(cpu, 6u);
            if (!TableServiceCall(service, 0x84dcbcu, 0x808299u, false))
                return false;
            cpu->carry = 0u;
            OpAdcValue(cpu, 4u);
            OpSta(memory, cpu, OpAbs(cpu, ROUND_COUNTDOWN));
            StoreRoundMarker(memory, cpu, 0x23u);
        }
        OpLoadA(cpu, 7u);
        OpSta(memory, cpu, OpDp(cpu, ROUND_SYMBOL_LIMIT));
        OpSta(memory, cpu, OpDp(cpu, ROUND_SYMBOL_MODE));
    }
    return true;
}

static void FindOrdinaryRoundSymbol(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    do {
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_CARD_GAME_TILE_BUFFER & 0xffffu));
        OpAndValue(cpu, 8u);
        if (!cpu->zero) break;
        OpStepMem(memory, cpu, OpDp(cpu, ROUND_SYMBOL), 1);
        OpDey(cpu);
        OpDey(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
}

static bool DrawOrdinaryRound(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpDp(cpu, ROUND_PASSES));
    do {
        if (!TableServiceCall(service, 0x84dcd9u, 0x84de6eu, false))
            return false;
        OpStz(memory, cpu, OpDp(cpu, ROUND_SCRATCH));
        if (!TableServiceCall(service, 0x84dcdeu, 0x84e640u, false))
            return false;
        OpIncA(cpu);
        cpu->carry = 0u;
        RolA16(cpu);
        OpAdc(memory, cpu, OpDp(cpu, ROUND_SYMBOL_BASE));
        OpTay(cpu);
        OpLdx(cpu, 12u);
        OpLda(memory, cpu, OpDp(cpu, ROUND_SYMBOL_LIMIT));
        OpCmpValue(cpu, 7u);
        bool scan = cpu->zero != 0u;
        if (!scan) {
            OpLoadA(cpu, 12u);
            if (!TableServiceCall(service, 0x84dcf4u, 0x808299u, false))
                return false;
            OpCmpValue(cpu, 0u);
            scan = cpu->zero == 0u;
        }
        if (scan) {
            FindOrdinaryRoundSymbol(memory, cpu);
        } else {
            OpLda(memory, cpu, OpAbsY(cpu, WRAM_CARD_GAME_TILE_BUFFER & 0xffffu));
            OpAndValue(cpu, 8u);
            if (!cpu->zero) {
                OpLda(memory, cpu, OpDp(cpu, ROUND_SYMBOL));
                OpCmpValue(cpu, 6u);
                if (!cpu->zero)
                    OpStepMem(memory, cpu, OpDp(cpu, ROUND_SYMBOL), 1);
            }
        }
        if (!TableServiceCall(service, 0x84dd1fu, 0x84e5b5u, false))
            return false;
        OpStepMem(memory, cpu, OpDp(cpu, ROUND_PASSES), -1);
    } while (!cpu->zero);
    return true;
}

static void BeginRoundMatchScan(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, ROUND_MATCH_COUNT));
    cpu->carry = 0u;
    RolA16(cpu);
    OpDecA(cpu);
    OpTay(cpu);
    OpLdx(cpu, 0u);
}

static void ClearRoundCursor(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, ROUND_LAYOUT));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, ROUND_SAVED_CURSOR));
    OpSta(memory, cpu, OpAbsX(cpu, ROUND_CURSORS));
    OpWriteX(memory, cpu, OpDp(cpu, ROUND_SAVED_CURSOR), cpu->x);
}

static bool FindRoundMatch(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    do {
        OpLda(memory, cpu, OpAbsX(cpu, ROUND_MATCH_TYPES));
        OpCmp(memory, cpu, OpDp(cpu, ROUND_MATCH_TYPE));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, ROUND_MATCH_VALUES));
            OpSta(memory, cpu, OpDp(cpu, ROUND_MATCH_VALUE));
            cpu->carry = 1u;
            OpSbcValue(cpu, 3u);
            if (!cpu->negative) return true;
        }
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return false;
}

static bool DrawSpecialRoundPass(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!TableServiceCall(service, 0x84dda8u, 0x84e5b5u, false) ||
        !TableServiceCall(service, 0x84ddabu, 0x84de6eu, false) ||
        !TableServiceCall(service, 0x84ddaeu, 0x84e640u, false) ||
        !TableServiceCall(service, 0x84ddb1u, 0x84e817u, false))
        return false;
    ClearRoundCursor(memory, cpu);
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpDp(cpu, ROUND_ATTEMPTS));
    do {
        if (!TableServiceCall(service, 0x84ddc6u, 0x84e829u, false))
            return false;
        BeginRoundMatchScan(memory, cpu);
        if (FindRoundMatch(memory, cpu)) return true;
        OpStepMem(memory, cpu, OpDp(cpu, ROUND_SYMBOL), 1);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ROUND_SAVED_CURSOR)));
        OpStepMem(memory, cpu, OpAbsX(cpu, ROUND_CURSORS), -1);
        OpStepMem(memory, cpu, OpDp(cpu, ROUND_ATTEMPTS), -1);
    } while (!cpu->zero);
    if (!TableServiceCall(service, 0x84ddf5u, 0x84e640u, false))
        return false;
    ClearRoundCursor(memory, cpu);
    return true;
}

static void FindRoundMatchValue(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, bool exact) {
    do {
        OpLda(memory, cpu, OpAbsX(cpu, ROUND_MATCH_TYPES));
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, ROUND_MATCH_VALUES));
            OpCmpValue(cpu, exact ? 8u : 3u);
            if (exact ? cpu->zero != 0u : cpu->negative != 0u) {
                OpStepMem(memory, cpu, OpDp(cpu, ROUND_SYMBOL), 1);
                if (exact) {
                    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ROUND_SAVED_CURSOR)));
                    OpStepMem(memory, cpu, OpAbsX(cpu, ROUND_CURSORS), -1);
                }
                return;
            }
        }
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

static bool DrawSpecialRound(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!TableServiceCall(service, 0x84dd90u, 0x84de6eu, false))
        return false;
    OpLoadA(cpu, 4u);
    if (!TableServiceCall(service, 0x84dd96u, 0x808299u, false))
        return false;
    OpSta(memory, cpu, OpDp(cpu, ROUND_SYMBOL));
    OpStz(memory, cpu, OpDp(cpu, ROUND_SCRATCH));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, ROUND_PASSES));
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, ROUND_MATCH_TYPE), cpu->x);
    do {
        if (!DrawSpecialRoundPass(service)) return false;
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpDp(cpu, ROUND_MATCH_TYPE));
        OpStepMem(memory, cpu, OpDp(cpu, ROUND_PASSES), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, ROUND_SYMBOL_LIMIT));
    if (!TableServiceCall(service, 0x84de13u, 0x84e829u, false))
        return false;
    BeginRoundMatchScan(memory, cpu);
    FindRoundMatchValue(memory, cpu, true);
    OpStepMem(memory, cpu, OpDp(cpu, ROUND_SYMBOL_LIMIT), -1);
    if (!TableServiceCall(service, 0x84de40u, 0x84e829u, false))
        return false;
    BeginRoundMatchScan(memory, cpu);
    FindRoundMatchValue(memory, cpu, false);
    OpStz(memory, cpu, OpDp(cpu, ROUND_SCRATCH));
    return TableServiceCall(service, 0x84de68u, 0x84e5b5u, false);
}

static bool FinishSpecialRound(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLda(memory, cpu, OpAbs(cpu, ROUND_VALUE));
    OpSta(memory, cpu, OpAbs(cpu, ROUND_NUMBER));
    OpStz(memory, cpu, OpAbs(cpu, ROUND_NUMBER_SLOT));
    if (!TableServiceCall(service, 0x84dd38u, 0x84eb36u, false))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, ROUND_CHANGED_DIGITS));
    OpSta(memory, cpu, OpAbs(cpu, ROUND_NUMBER_MOVEMENT));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, ROUND_PENDING_NUMBER));
    if (!TableServiceCall(service, 0x84dd47u, 0x84ebdbu, false))
        return false;
    OpStz(memory, cpu, OpAbs(cpu, ROUND_VARIANT));
    OpStz(memory, cpu, OpAbs(cpu, ROUND_COUNTDOWN));
    OpStz(memory, cpu, OpDp(cpu, ROUND_SYMBOL_MODE));
    StoreRoundMarker(memory, cpu, 0x22u);
    OpLoadA(cpu, 0u);
    return true;
}

static void EndSpecialRoundMode(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StoreRoundMarker(memory, cpu, 2u);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, ROUND_SYMBOL_MODE));
    OpSta(memory, cpu, OpAbs(cpu, ROUND_RANDOM_SELECTION));
    OpSta(memory, cpu, OpDp(cpu, ROUND_SYMBOL_LIMIT));
    OpTestBits(memory, cpu, OpAbs(cpu, ROUND_VARIANT), false);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, ROUND_RANDOM_SELECTION + 2u));
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, ROUND_COUNTDOWN));
    OpSta(memory, cpu, OpAbs(cpu, ROUND_TOTAL));
    StoreRoundMarker(memory, cpu, 0x24u);
    OpLoadA(cpu, 0u);
}

Lufia2ExecutionResult Lufia2CardTableAdvanceRound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, false) ||
        cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84dc87u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84dd29u), false};
    OpLda(memory, cpu, OpAbs(cpu, ROUND_VARIANT));
    OpAndValue(cpu, 8u);
    if (cpu->zero) {
        if (!AdvanceOrdinaryRoundCounters(&service) || !DrawOrdinaryRound(&service))
            return service.result;
    } else {
        OpStepMem(memory, cpu, OpAbs(cpu, ROUND_VALUE), -1);
        if (cpu->zero) {
            FinishSpecialRound(&service);
            return service.result;
        }
        OpStepMem(memory, cpu, OpAbs(cpu, ROUND_COUNTDOWN), -1);
        if (cpu->zero) {
            EndSpecialRoundMode(memory, cpu);
            return service.result;
        }
        if (!DrawSpecialRound(&service)) return service.result;
    }
    OpLoadA(cpu, 1u);
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableChooseRoundColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, false) ||
        cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84de6eu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84deb8u), false};
    for (;;) {
        if (!TableServiceCall(&service, 0x84de6eu, 0x84e9b1u, false) ||
            !TableServiceCall(&service, 0x84de71u, 0x84dc55u, false))
            return service.result;
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, ROUND_COLUMN_FLAGS + 4u));
            if (!cpu->zero) {
                OpLoadA(cpu, 2u);
                break;
            }
        } else {
            OpCmpValue(cpu, 4u);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, ROUND_COLUMN_FLAGS + 2u));
                if (!cpu->zero) {
                    OpLoadA(cpu, 1u);
                    break;
                }
            } else {
                OpCmpValue(cpu, 3u);
                if (cpu->zero) {
                    OpLda(memory, cpu, OpAbs(cpu, ROUND_COLUMN_FLAGS));
                    if (!cpu->zero) {
                        OpLoadA(cpu, 0u);
                        break;
                    }
                } else {
                    OpCmpValue(cpu, 6u);
                    if (!cpu->zero) continue;
                }
            }
        }
        OpLdx(cpu, 0u);
        for (;;) {
            OpLda(memory, cpu, OpAbsX(cpu, ROUND_COLUMN_FLAGS));
            if (!cpu->zero) break;
            OpInx(cpu);
            OpInx(cpu);
        }
        OpTxa(cpu);
        cpu->carry = 0u;
        RorA16(cpu);
        break;
    }
    OpSta(memory, cpu, OpAbs(cpu, ROUND_LAYOUT));
    return service.result;
}
