#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum { TABLE_COLOR_POINTER = 8u, TABLE_COLOR_BANK = 10u };

Lufia2ExecutionResult Lufia2CardTableUpdateHdma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, 0x84f12fu);
    if (cpu->accumulator_is_8_bit) PushAccumulator8(memory, cpu);
    else PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, WRAM_CARD_TABLE_PHASE);
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, WRAM_CARD_TABLE_PHASE);
        OpLoadA(cpu, 0x7eu);
        OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(4u)));
        OpLdx(cpu, 0x4000u);
        OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(4u)), cpu->x);
        OpLoadA(cpu, 0x32u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(4u)));
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(4u)));
        OpLoadA(cpu, 0x10u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    } else {
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 0u);
            OpSta(memory, cpu, WRAM_CARD_TABLE_PHASE);
            OpSta(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit) LoadA8(cpu, Pull8(memory, cpu));
    else PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x84f172u);
}

Lufia2ExecutionResult Lufia2CardTableBuildHdma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x84f173u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, TABLE_COLOR_BANK));
    OpLdx(cpu, 0xf19du);
    OpWriteX(memory, cpu, OpDp(cpu, TABLE_COLOR_POINTER), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0u);
    for (unsigned word = 0u; word < 32768u; ++word) {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, TABLE_COLOR_POINTER));
        if (cpu->zero) {
            OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
            UnpackStatus(cpu, Pull8(memory, cpu));
            return ExecutionReturned(0x84f19cu);
        }
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
    }
    return ExecutionHandoff(cpu, 0x84f189u);
}
