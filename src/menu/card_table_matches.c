#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    MATCH_SOURCE = 0x08u,
    MATCH_SOURCE_BANK = 0x0au,
    MATCH_MASK = 0x0eu,
    MATCH_OUTPUT = 0x22u,
    MATCH_TYPES = (WRAM_POKER_RANK_ORDER + 5u) & 0xffffu,
    MATCH_SYMBOLS = (WRAM_CARD_TABLE_MATCH_VALUES - 18u) & 0xffffu,
    MATCH_VALUES = WRAM_CARD_TABLE_MATCH_VALUES & 0xffffu,
    MATCH_CURSORS = WRAM_CARD_TABLE_PREVIOUS_HAND_WORDS & 0xffffu,
    MATCH_COLUMN_SOURCE = WRAM_CARD_GAME_TILE_BUFFER & 0xffffu
};

static bool MatchWordEntry(const Lufia2CpuState *cpu) {
    return TableServiceReady(cpu, false) && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit;
}

Lufia2ExecutionResult Lufia2CardTableAppendMatchSymbol(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MatchWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e91bu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, MATCH_SOURCE));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, MATCH_OUTPUT)));
    OpIncA(cpu);
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, MATCH_MASK)));
    OpSta(memory, cpu, OpAbsX(cpu, MATCH_SYMBOLS));
    OpInx(cpu);
    OpInx(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, MATCH_OUTPUT), cpu->x);
    return ExecutionReturned(0x84e929u);
}

Lufia2ExecutionResult Lufia2CardTableReadMatchColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !MatchWordEntry(cpu))
        return ExecutionHandoff(cpu, 0x84e906u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e91au), false};
    cpu->carry = 0u;
    RolA16(cpu);
    OpTay(cpu);
    OpIny(cpu);
    OpIny(cpu);
    if (!TableServiceCall(&service, 0x84e90bu, 0x84e91bu, false))
        return service.result;
    OpDey(cpu);
    OpDey(cpu);
    if (!TableServiceCall(&service, 0x84e910u, 0x84e91bu, false))
        return service.result;
    for (unsigned byte = 0u; byte < 4u; ++byte) OpIny(cpu);
    TableServiceCall(&service, 0x84e917u, 0x84e91bu, false);
    return service.result;
}

static void CountMatchPair(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpAbsX(cpu, MATCH_TYPES), 1);
    OpSta(memory, cpu, OpAbsX(cpu, MATCH_VALUES));
}

static void CompareMatchTriple(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint32_t first, uint32_t second, uint32_t third) {
    OpLda(memory, cpu, first);
    OpCmp(memory, cpu, second);
    if (cpu->zero) CountMatchPair(memory, cpu);
    OpCmp(memory, cpu, third);
    if (cpu->zero) CountMatchPair(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, MATCH_TYPES));
    if (cpu->zero) {
        OpLda(memory, cpu, second);
        OpCmp(memory, cpu, third);
        if (cpu->zero) CountMatchPair(memory, cpu);
    }
}

static void FindColumnMatches(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    OpLdy(cpu, 3u);
    do {
        CompareMatchTriple(memory, cpu,
            OpAbsX(cpu, MATCH_SYMBOLS), OpAbsX(cpu, MATCH_SYMBOLS + 6u),
            OpAbsX(cpu, MATCH_SYMBOLS + 12u));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    CompareMatchTriple(memory, cpu,
        OpAbs(cpu, MATCH_SYMBOLS + 2u), OpAbs(cpu, MATCH_SYMBOLS + 6u),
        OpAbs(cpu, MATCH_SYMBOLS + 16u));
    OpInx(cpu);
    OpInx(cpu);
    CompareMatchTriple(memory, cpu,
        OpAbs(cpu, MATCH_SYMBOLS + 14u), OpAbs(cpu, MATCH_SYMBOLS + 6u),
        OpAbs(cpu, MATCH_SYMBOLS + 4u));
}

Lufia2ExecutionResult Lufia2CardTableBuildColumnMatches(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, false) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84e829u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84e905u), false};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, MATCH_SOURCE_BANK));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, MATCH_OUTPUT), cpu->x);
    OpLoadA(cpu, 19u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, MATCH_TYPES));
        OpInx(cpu);
        OpInx(cpu);
        OpDecA(cpu);
    } while (!cpu->zero);
    static const uint32_t sites[] = {0x84e84cu, 0x84e85eu, 0x84e870u};
    for (unsigned column = 0u; column < 3u; ++column) {
        if (column) {
            OpLdx(cpu, (uint16_t)(column * 6u));
            OpWriteX(memory, cpu, OpDp(cpu, MATCH_OUTPUT), cpu->x);
        }
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(MATCH_CURSORS + column * 2u)));
        if (!cpu->zero) {
            OpLdx(cpu, (uint16_t)(MATCH_COLUMN_SOURCE + column * 58u));
            OpWriteX(memory, cpu, OpDp(cpu, MATCH_SOURCE), cpu->x);
            if (!TableServiceCall(&service, sites[column], 0x84e906u, false))
                return service.result;
        }
    }
    FindColumnMatches(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return service.result;
}
