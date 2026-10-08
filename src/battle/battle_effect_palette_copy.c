#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_PALETTE_INDEX = 0u,
    DP_PALETTE_COUNT = 2u,
    DP_PALETTE_UPLOAD = 0x73u,
    DP_PALETTE_STREAM = 0xc3u,
    EFFECT_PALETTE_BACKUP = 0x7ff1dbu,
    EFFECT_PALETTE_CURRENT = 0x0320u
};

static void ReadEffectPaletteRange(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_PALETTE_STREAM));
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_INDEX));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_PALETTE_STREAM), 1);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_PALETTE_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_PALETTE_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_PALETTE_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_PALETTE_INDEX), 1u);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_COUNT));
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpDp(cpu, DP_PALETTE_INDEX));
    OpRepWidths(cpu, 0x20u);
    OpAslA(cpu);
    OpTax(cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectRestorePaletteRange(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x81968au);
    OpSetDataBank(memory, cpu, 0x7eu);
    ReadEffectPaletteRange(memory, cpu);
    do {
        OpLda(memory, cpu, OpLongX(cpu, EFFECT_PALETTE_BACKUP));
        OpSta(memory, cpu, OpAbsX(cpu, EFFECT_PALETTE_CURRENT));
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_PALETTE_COUNT), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_UPLOAD));
    return ExecutionReturned(0x8196c4u);
}
