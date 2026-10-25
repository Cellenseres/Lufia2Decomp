#include "menu/arithmetic_wait.h"
#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    SYMBOL_SAVED_CURSOR = 0x00u,
    SYMBOL_CANDIDATE = 0x02u,
    SYMBOL_ATTEMPTS = 0x04u,
    SYMBOL_MATCH_TYPE = 0x06u,
    SYMBOL_SOURCE = 0x08u,
    SYMBOL_NOT_FOUND = 0x0bu,
    SYMBOL_MASK = 0x0eu,
    SYMBOL_COLUMN = (WRAM_POKER_ROUNDS_REMAINING - 1u) & 0xffffu,
    SYMBOL_COLUMN_SPEEDS = WRAM_CARD_GAME_OPENING_CHOICES & 0xffffu,
    SYMBOL_COLUMN_STEPS = (WRAM_POKER_OPPONENT_VALUES + 3u) & 0xffffu,
    SYMBOL_COLUMN_POSITIONS = (WRAM_POKER_ROUND_STAKE + 3u) & 0xffffu,
    SYMBOL_STOPPED_COLUMNS = (WRAM_POKER_ROUND_STATUS - 1u) & 0xffffu,
    SYMBOL_STOPPED_SPEEDS = (WRAM_POKER_RANK_ORDER - 1u) & 0xffffu,
    SYMBOL_MATCH_TYPES = (WRAM_POKER_RANK_ORDER + 5u) & 0xffffu,
    SYMBOL_MATCH_VALUES = WRAM_CARD_TABLE_MATCH_VALUES & 0xffffu,
    SYMBOL_CURSORS = WRAM_CARD_TABLE_PREVIOUS_HAND_WORDS & 0xffffu,
    SYMBOL_MATCH_COUNT = WRAM_CARD_TABLE_SELECTION_ACTIVE & 0xffffu,
    SYMBOL_RANDOM_MODE = WRAM_CARD_TABLE_RANDOM_SELECTION & 0xffffu,
    SYMBOL_STOP_DELAY = (WRAM_BOARD_GAME_LINE_TOTALS + 12u) & 0xffffu,
    SYMBOL_RAMP_TIMERS = (WRAM_POKER_ACTION_STATE + 1u) & 0xffffu,
    SYMBOL_RAMP_OFFSETS = (WRAM_CARD_GAME_REPLAY_CURSOR + 3u) & 0xffffu,
    SYMBOL_NUMBER = WRAM_CARD_TABLE_PAYOUT_INDEX & 0xffffu,
    SYMBOL_NUMBER_SLOT = WRAM_CARD_TABLE_PAYOUT_STATE & 0xffffu,
    SYMBOL_CHANGED_DIGITS = WRAM_CARD_TABLE_PAYOUT_VALUE & 0xffffu,
    SYMBOL_THIRD_PENDING = WRAM_CARD_TABLE_THIRD_NUMBER_PENDING & 0xffffu,
    SYMBOL_THIRD_MOVEMENT = WRAM_CARD_TABLE_THIRD_NUMBER_MOVEMENT & 0xffffu,
    SYMBOL_COLUMN_LABELS = (WRAM_POKER_OPPONENT_INDICES - 1u) & 0xffffu,
    SYMBOL_RAMP_TABLE = 0x84f1f5u,
    SYMBOL_MULTIPLIER = 0x004202u,
    SYMBOL_PRODUCT = 0x004216u
};

static bool SymbolWordEntry(const Lufia2CpuState *cpu) {
    return TableServiceReady(cpu, false) && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit;
}

Lufia2ExecutionResult Lufia2CardTablePrepareColumnSymbols(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TableServiceReady(cpu, false))
        return ExecutionHandoff(cpu, 0x84e640u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpAbs(cpu, SYMBOL_COLUMN));
    OpOraValue(cpu, 0x3a00u);
    OpSta(memory, cpu, SYMBOL_MULTIPLIER);
    WaitForArithmetic(memory, cpu, 0x84e64fu, 4u);
    OpLda(memory, cpu, SYMBOL_PRODUCT);
    OpSta(memory, cpu, OpDp(cpu, SYMBOL_SOURCE));
    OpLda(memory, cpu, OpAbs(cpu, SYMBOL_COLUMN));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR), cpu->x);
    OpLda(memory, cpu, OpAbsX(cpu, SYMBOL_COLUMN_SPEEDS));
    OpDecA(cpu);
    OpCmpValue(cpu, 5u);
    if (cpu->negative) OpLoadA(cpu, 25u);
    OpSta(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR));
    OpStz(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84e672u);
}

Lufia2ExecutionResult Lufia2CardTableRememberRoundSymbol(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SymbolWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e817u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 3u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, SYMBOL_STOPPED_SPEEDS));
        OpSta(memory, cpu, OpAbsX(cpu, SYMBOL_CURSORS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x84e828u);
}

static bool FindRequestedSymbol(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, SYMBOL_MATCH_COUNT));
    cpu->carry = 0u;
    RolA16(cpu);
    OpDecA(cpu);
    OpTay(cpu);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, SYMBOL_MATCH_TYPES));
        OpCmp(memory, cpu, OpDp(cpu, SYMBOL_MATCH_TYPE));
        if (cpu->zero) {
            OpStz(memory, cpu, OpDp(cpu, SYMBOL_NOT_FOUND));
            OpLda(memory, cpu, OpAbsX(cpu, SYMBOL_MATCH_VALUES));
            OpCmp(memory, cpu, OpAbs(cpu, SYMBOL_RANDOM_MODE));
            if (cpu->zero) return true;
        }
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return false;
}

Lufia2ExecutionResult Lufia2CardTableFindMatchingSymbol(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SymbolWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e673u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e6c7u), false};
    if (!TableServiceCall(&service, 0x84e673u, 0x84e817u, false))
        return service.result;
    OpLda(memory, cpu, OpAbs(cpu, SYMBOL_COLUMN));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR));
    OpSta(memory, cpu, OpAbsX(cpu, SYMBOL_CURSORS));
    OpWriteX(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR), cpu->x);
    OpLdx(cpu, 5u);
    OpLda(memory, cpu, OpDp(cpu, SYMBOL_MASK));
    OpAndValue(cpu, 8u);
    if (!cpu->zero) OpLdx(cpu, 10u);
    OpWriteX(memory, cpu, OpDp(cpu, SYMBOL_ATTEMPTS), cpu->x);
    do {
        if (!TableServiceCall(&service, 0x84e692u, 0x84e829u, false))
            return service.result;
        if (FindRequestedSymbol(memory, cpu)) return service.result;
        OpStepMem(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE), 1);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR)));
        OpStepMem(memory, cpu, OpAbsX(cpu, SYMBOL_CURSORS), -1);
        OpStepMem(memory, cpu, OpDp(cpu, SYMBOL_ATTEMPTS), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, SYMBOL_NOT_FOUND));
    OpStz(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE));
    return service.result;
}

static void CheckColumnBoundary(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, SYMBOL_COLUMN_POSITIONS));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsX(cpu, SYMBOL_COLUMN_STEPS));
    OpCmpValue(cpu, 0x1c00u);
}

static bool WaitForStoppedSymbol(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    CheckColumnBoundary(memory, cpu);
    bool wait = cpu->negative != 0u;
    for (;;) {
        if (wait) {
            if (!TableServiceCall(service, 0x84e5deu, 0x84e9b1u, false))
                return false;
            CheckColumnBoundary(memory, cpu);
            if (cpu->negative) continue;
        }
        OpStepMem(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE), -1);
        if (cpu->negative) return true;
        wait = true;
    }
}

static bool PaintStoppedColumnLabels(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        OpLoadA(cpu, 0x0c00u);
        OpSta(memory, cpu, OpDp(cpu, SYMBOL_ATTEMPTS));
        OpLda(memory, cpu, OpAbsY(cpu, SYMBOL_STOPPED_SPEEDS));
        if (!cpu->zero) {
            OpLoadA(cpu, 0x1800u);
            OpSta(memory, cpu, OpDp(cpu, SYMBOL_ATTEMPTS));
        }
        OpLda(memory, cpu, OpAbsX(cpu, SYMBOL_COLUMN_LABELS));
        OpSta(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR));
        OpInx(cpu);
        OpInx(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, SYMBOL_COLUMN_LABELS));
        OpSta(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE));
        OpInx(cpu);
        OpInx(cpu);
        OpLoadA(cpu, 3u);
        OpSta(memory, cpu, OpDp(cpu, SYMBOL_MATCH_TYPE));
        if (!TableServiceCall(service, 0x84e635u, 0x84f105u, false))
            return false;
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 6u);
    } while (!cpu->zero);
    return true;
}

Lufia2ExecutionResult Lufia2CardTableStopColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SymbolWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e5b5u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e63fu), false};
    OpLoadA(cpu, 0x23u);
    if (!TableServiceCall(&service, 0x84e5b8u, 0x84a607u, false))
        return service.result;
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, SYMBOL_STOP_DELAY));
    OpLda(memory, cpu, OpDp(cpu, SYMBOL_ATTEMPTS));
    if (!TableServiceCall(&service, 0x84e5c3u, 0x808299u, false))
        return service.result;
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE));
    OpSta(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE));
    OpLda(memory, cpu, OpAbs(cpu, SYMBOL_COLUMN));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    if (!WaitForStoppedSymbol(&service)) return service.result;
    OpLda(memory, cpu, OpAbsX(cpu, SYMBOL_COLUMN_SPEEDS));
    OpDecA(cpu);
    OpCmpValue(cpu, 5u);
    if (cpu->negative) OpLoadA(cpu, 25u);
    OpSta(memory, cpu, OpAbsX(cpu, SYMBOL_COLUMN_SPEEDS));
    OpSta(memory, cpu, OpAbsX(cpu, SYMBOL_STOPPED_SPEEDS));
    OpStz(memory, cpu, OpAbsX(cpu, SYMBOL_COLUMN_STEPS));
    OpStz(memory, cpu, OpAbsX(cpu, SYMBOL_COLUMN_POSITIONS));
    if (!TableServiceCall(&service, 0x84e609u, 0x84ea4bu, false))
        return service.result;
    OpStepMem(memory, cpu, OpAbs(cpu, SYMBOL_STOPPED_COLUMNS), 1);
    PaintStoppedColumnLabels(&service);
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableStartColumns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SymbolWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e92au);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e9b0u), false};
    OpStz(memory, cpu, OpAbs(cpu, SYMBOL_NUMBER));
    OpLdx(cpu, 2u);
    OpWriteX(memory, cpu, OpAbs(cpu, SYMBOL_NUMBER_SLOT), cpu->x);
    if (!TableServiceCall(&service, 0x84e933u, 0x84eb36u, false))
        return service.result;
    OpLda(memory, cpu, OpAbs(cpu, SYMBOL_CHANGED_DIGITS));
    OpSta(memory, cpu, OpAbs(cpu, SYMBOL_THIRD_MOVEMENT));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SYMBOL_THIRD_PENDING));
    if (!TableServiceCall(&service, 0x84e942u, 0x84ebdbu, false))
        return service.result;
    OpStz(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR));
    OpStz(memory, cpu, OpAbs(cpu, SYMBOL_RAMP_OFFSETS + 4u));
    OpLdx(cpu, 4u);
    OpWriteX(memory, cpu, OpAbs(cpu, SYMBOL_RAMP_OFFSETS + 2u), cpu->x);
    OpLdx(cpu, 8u);
    OpWriteX(memory, cpu, OpAbs(cpu, SYMBOL_RAMP_OFFSETS), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, SYMBOL_RAMP_TIMERS), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, SYMBOL_RAMP_TIMERS + 4u), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, SYMBOL_RAMP_TIMERS + 2u), cpu->x);
    OpLdx(cpu, 0x0100u);
    OpWriteX(memory, cpu, OpAbs(cpu, SYMBOL_COLUMN_STEPS), cpu->x);
    for (;;) {
        OpLda(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR));
        OpSta(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE));
        do {
            cpu->carry = 0u;
            RolA16(cpu);
            OpTay(cpu);
            OpLda(memory, cpu, OpAbsY(cpu, SYMBOL_RAMP_TIMERS));
            OpDecA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, SYMBOL_RAMP_TIMERS));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsY(cpu, SYMBOL_RAMP_OFFSETS));
                OpTax(cpu);
                OpLda(memory, cpu, OpLongX(cpu, SYMBOL_RAMP_TABLE));
                if (cpu->zero) {
                    OpLoadA(cpu, 0x0b00u);
                    OpSta(memory, cpu, OpAbsY(cpu, SYMBOL_COLUMN_STEPS));
                    OpStepMem(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR), 1);
                    OpLda(memory, cpu, OpDp(cpu, SYMBOL_SAVED_CURSOR));
                    OpCmpValue(cpu, 3u);
                    if (cpu->zero) return service.result;
                } else {
                    OpSta(memory, cpu, OpAbsY(cpu, SYMBOL_RAMP_TIMERS));
                    OpInx(cpu);
                    OpInx(cpu);
                    OpLda(memory, cpu, OpLongX(cpu, SYMBOL_RAMP_TABLE));
                    OpSta(memory, cpu, OpAbsY(cpu, SYMBOL_COLUMN_STEPS));
                    OpInx(cpu);
                    OpInx(cpu);
                    OpTxa(cpu);
                    OpSta(memory, cpu, OpAbsY(cpu, SYMBOL_RAMP_OFFSETS));
                }
            }
            OpStepMem(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE), 1);
            OpLda(memory, cpu, OpDp(cpu, SYMBOL_CANDIDATE));
            OpCmpValue(cpu, 3u);
        } while (!cpu->zero);
        if (!TableServiceCall(&service, 0x84e9abu, 0x84e9b1u, false))
            return service.result;
    }
}
