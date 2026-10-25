#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    SEARCH_CURSOR = 0x00u,
    SEARCH_CANDIDATE = 0x02u,
    SEARCH_ATTEMPTS = 0x04u,
    SEARCH_TYPE = 0x06u,
    SEARCH_SOURCE = 0x08u,
    SEARCH_NOT_FOUND = 0x0bu,
    SEARCH_MASK = 0x0eu,
    SEARCH_COLUMN = (WRAM_POKER_ROUNDS_REMAINING - 1u) & 0xffffu,
    SEARCH_SYMBOLS = WRAM_CARD_GAME_TILE_BUFFER & 0xffffu,
    SEARCH_DESIRED = WRAM_CARD_TABLE_RANDOM_SELECTION & 0xffffu,
    SEARCH_MODE = (WRAM_CARD_TABLE_RANDOM_SELECTION + 2u) & 0xffffu,
    SEARCH_CURSORS = WRAM_CARD_TABLE_PREVIOUS_HAND_WORDS & 0xffffu,
    SEARCH_COUNT = WRAM_CARD_TABLE_SELECTION_ACTIVE & 0xffffu,
    SEARCH_MATCH_TYPES = (WRAM_POKER_RANK_ORDER + 5u) & 0xffffu,
    SEARCH_MATCH_VALUES = WRAM_CARD_TABLE_MATCH_VALUES & 0xffffu
};

static bool SearchWordEntry(const Lufia2CpuState *cpu) {
    return TableServiceReady(cpu, false) && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit;
}

static void ReadSearchSymbol(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, SEARCH_SYMBOLS));
    OpIncA(cpu);
}

static bool FindPreferredCandidate(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, SEARCH_CURSOR));
    OpIncA(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SEARCH_COUNT)));
    OpCpx(cpu, 1u);
    if (!cpu->zero) OpIncA(cpu);
    cpu->carry = 0u;
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, SEARCH_SOURCE));
    OpTay(cpu);
    OpLda(memory, cpu, OpAbs(cpu, SEARCH_MODE));
    OpAndValue(cpu, 0xff00u);
    if (!cpu->zero) {
        OpStepMem(memory, cpu, OpDp(cpu, SEARCH_CANDIDATE), 1);
        ReadSearchSymbol(memory, cpu);
        OpCmpValue(cpu, 7u);
        if (cpu->zero) return true;
        OpStz(memory, cpu, OpDp(cpu, SEARCH_CANDIDATE));
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SEARCH_COUNT)));
    OpCpx(cpu, 1u);
    if (!cpu->zero) OpLdx(cpu, 3u);
    do {
        ReadSearchSymbol(memory, cpu);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, SEARCH_MASK)));
        OpCmp(memory, cpu, OpAbs(cpu, SEARCH_DESIRED));
        if (cpu->zero) return true;
        OpDey(cpu);
        OpDey(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, 5u);
    OpLda(memory, cpu, OpDp(cpu, SEARCH_MASK));
    OpAndValue(cpu, 8u);
    if (!cpu->zero) OpLdx(cpu, 10u);
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
    do {
        OpStepMem(memory, cpu, OpDp(cpu, SEARCH_CANDIDATE), 1);
        ReadSearchSymbol(memory, cpu);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, SEARCH_MASK)));
        OpCmp(memory, cpu, OpAbs(cpu, SEARCH_DESIRED));
        if (cpu->zero) return true;
        OpDey(cpu);
        OpDey(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS), -1);
        if (cpu->negative) OpStz(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
        OpDex(cpu);
    } while (!cpu->zero);
    return false;
}

Lufia2ExecutionResult Lufia2CardTableChoosePreferredSymbol(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SearchWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e6c8u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e74eu), false};
    if (!TableServiceCall(&service, 0x84e6c8u, 0x84e640u, false))
        return service.result;
    OpStz(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
    OpLda(memory, cpu, OpAbs(cpu, SEARCH_MODE));
    if (cpu->zero) {
        if (!TableServiceCall(&service, 0x84e6d2u, 0x84e7efu, false))
            return service.result;
    } else if (!FindPreferredCandidate(memory, cpu)) {
        if (!TableServiceCall(&service, 0x84e742u, 0x84e7efu, false))
            return service.result;
    }
    OpLda(memory, cpu, OpDp(cpu, SEARCH_MASK));
    OpAndValue(cpu, 8u);
    if (!cpu->zero) OpStz(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableChooseSingleMatch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SearchWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e74fu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e772u), false};
    if (!TableServiceCall(&service, 0x84e74fu, 0x84e640u, false))
        return service.result;
    OpLda(memory, cpu, OpAbs(cpu, SEARCH_MODE));
    bool fallback = cpu->zero != 0u;
    if (!fallback) {
        OpLdx(cpu, 1u);
        OpWriteX(memory, cpu, OpDp(cpu, SEARCH_TYPE), cpu->x);
        if (!TableServiceCall(&service, 0x84e75cu, 0x84e673u, false))
            return service.result;
        OpLda(memory, cpu, OpDp(cpu, SEARCH_NOT_FOUND));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, SEARCH_MODE));
            OpAndValue(cpu, 0xff00u);
            fallback = !cpu->zero;
        }
    }
    if (fallback) {
        OpStz(memory, cpu, OpDp(cpu, SEARCH_CANDIDATE));
        if (!TableServiceCall(&service, 0x84e76du, 0x84e7efu, false))
            return service.result;
    }
    OpStz(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
    return service.result;
}

static void CountUnitDoubleMatches(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLdy(cpu, 5u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, SEARCH_MATCH_TYPES));
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, SEARCH_MATCH_VALUES));
            OpCmpValue(cpu, 1u);
            if (cpu->zero)
                OpStepMem(memory, cpu, OpDp(cpu, SEARCH_CANDIDATE), 1);
        }
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

static bool SearchDoubleMatch(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    if (!TableServiceCall(service, 0x84e792u, 0x84e817u, false))
        return false;
    OpLda(memory, cpu, OpAbs(cpu, SEARCH_COLUMN));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, SEARCH_CURSOR));
    OpSta(memory, cpu, OpAbsX(cpu, SEARCH_CURSORS));
    OpWriteX(memory, cpu, OpDp(cpu, SEARCH_CURSOR), cpu->x);
    OpLdx(cpu, 5u);
    OpWriteX(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS), cpu->x);
    do {
        if (!TableServiceCall(service, 0x84e7a7u, 0x84e829u, false))
            return false;
        OpLdy(cpu, 5u);
        OpLdx(cpu, 0u);
        bool found = false;
        do {
            OpLda(memory, cpu, OpAbsX(cpu, SEARCH_MATCH_TYPES));
            OpCmpValue(cpu, 2u);
            if (cpu->zero) {
                found = true;
                break;
            }
            OpInx(cpu);
            OpInx(cpu);
            OpDey(cpu);
        } while (!cpu->zero);
        if (!found)
            return TableServiceCall(service, 0x84e7bdu, 0x84e7efu, false);
        OpStepMem(memory, cpu, OpDp(cpu, SEARCH_CANDIDATE), 1);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SEARCH_CURSOR)));
        OpStepMem(memory, cpu, OpAbsX(cpu, SEARCH_CURSORS), -1);
        OpStepMem(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS), -1);
    } while (!cpu->zero);
    return true;
}

Lufia2ExecutionResult Lufia2CardTableChooseDoubleMatch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SearchWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e773u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e7cfu), false};
    if (!TableServiceCall(&service, 0x84e773u, 0x84e640u, false))
        return service.result;
    OpLda(memory, cpu, OpAbs(cpu, SEARCH_MODE));
    if (!cpu->zero) {
        OpLdx(cpu, 2u);
        OpWriteX(memory, cpu, OpDp(cpu, SEARCH_TYPE), cpu->x);
        if (!TableServiceCall(&service, 0x84e780u, 0x84e673u, false))
            return service.result;
        OpLda(memory, cpu, OpAbs(cpu, SEARCH_MODE));
        OpAndValue(cpu, 0xff00u);
        if (cpu->zero) {
            CountUnitDoubleMatches(memory, cpu);
            OpStz(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
            return service.result;
        }
        OpLda(memory, cpu, OpDp(cpu, SEARCH_NOT_FOUND));
        if (cpu->zero) {
            OpStz(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
            return service.result;
        }
        if (!TableServiceCall(&service, 0x84e78fu, 0x84e640u, false))
            return service.result;
    }
    if (!SearchDoubleMatch(&service)) return service.result;
    OpStz(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableScanSeventhSymbol(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SearchWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e7efu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e816u), false};
    OpLda(memory, cpu, OpDp(cpu, SEARCH_CANDIDATE));
    OpSta(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
    if (!TableServiceCall(&service, 0x84e7f3u, 0x84e640u, false))
        return service.result;
    OpIncA(cpu);
    OpIncA(cpu);
    cpu->carry = 0u;
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, SEARCH_SOURCE));
    OpTay(cpu);
    OpLdx(cpu, 3u);
    bool found = false;
    do {
        OpStepMem(memory, cpu, OpDp(cpu, SEARCH_CANDIDATE), 1);
        ReadSearchSymbol(memory, cpu);
        OpCmpValue(cpu, 7u);
        if (cpu->zero) {
            found = true;
            break;
        }
        OpDey(cpu);
        OpDey(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    if (!found) {
        OpLda(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
        OpSta(memory, cpu, OpDp(cpu, SEARCH_CANDIDATE));
    }
    OpStz(memory, cpu, OpDp(cpu, SEARCH_ATTEMPTS));
    return service.result;
}
