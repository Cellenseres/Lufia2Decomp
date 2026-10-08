#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"

enum {
    DP_PALETTE_SOURCE = 8u,
    DP_PALETTE_DESTINATION = 11u,
    DP_PALETTE_UPLOAD = 0x73u,
    DP_PALETTE_STREAM = 0xc3u,
    PALETTE_BACKUP_BASE = 0xf1dbu,
    PALETTE_CURRENT_BASE = 0x0320u,
    PALETTE_ROM_BASE = 0x8000u,
    PALETTE_ROM_BANK = 0x96u,
    PALETTE_BACKUP_BANK = 0x7fu,
    PALETTE_WORK_BANK = 0x95u
};

static void AdvancePaletteStream(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_PALETTE_STREAM), 1);
}

static void ReadPaletteDestination(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_PALETTE_STREAM));
    AdvancePaletteStream(memory, cpu);
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpAslA(cpu);
    OpOra(memory, cpu, DirectLongPointer(memory, cpu, DP_PALETTE_STREAM));
    AdvancePaletteStream(memory, cpu);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    PushAccumulator16(memory, cpu);
    OpAdcValue(cpu, PALETTE_BACKUP_BASE);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDL));
    PullAccumulator16(memory, cpu);
    OpAdcValue(cpu, PALETTE_CURRENT_BASE);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_DESTINATION));
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_PALETTE_STREAM));
    AdvancePaletteStream(memory, cpu);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
}

static void ReadPaletteSource(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_PALETTE_STREAM));
    AdvancePaletteStream(memory, cpu);
    AdvancePaletteStream(memory, cpu);
    for (unsigned shift = 0u; shift < 5u; ++shift)
        OpAslA(cpu);
    OpAdcValue(cpu, PALETTE_ROM_BASE);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_SOURCE));
    OpSepWidths(cpu, 0x20u);
    LoadA8(cpu, PALETTE_ROM_BANK);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_SOURCE + 2u));
    LoadA8(cpu, PALETTE_BACKUP_BANK);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLdy(cpu, 0u);
}

Lufia2ExecutionResult Lufia2BattleEffectLoadPaletteRange(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x8196c6u);
    PushY(memory, cpu);
    OpSetDataBank(memory, cpu, PALETTE_WORK_BANK);
    ReadPaletteDestination(memory, cpu);
    ReadPaletteSource(memory, cpu);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_PALETTE_SOURCE));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        const uint16_t destination = Read16Direct(memory, cpu, DP_PALETTE_DESTINATION);
        OpSta(memory, cpu, AbsoluteIndexedAddress(cpu, destination, cpu->y));
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    LoadA8(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_UPLOAD));
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x81915au);
}
