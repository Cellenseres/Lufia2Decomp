#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/field.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2FieldSetSceneDisplay(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xf2f3u);
    OpLda(memory, cpu, 0x7f0001u);
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
        OpLda(memory, cpu, 0x7f0002u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_TS));
        OpLda(memory, cpu, 0x7f0003u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
        OpLda(memory, cpu, 0x7f0004u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
        OpLoadA(cpu, 0xe0u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
        OpLda(memory, cpu, 0x7f0005u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
        OpStz(memory, cpu, OpAbs(cpu, SNES_MOSAIC));
        return ExecutionReturned(0x80f320u);
    }
    OpLoadA(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    OpStz(memory, cpu, OpAbs(cpu, SNES_TS));
    OpStz(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpStz(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
    OpStz(memory, cpu, OpAbs(cpu, SNES_MOSAIC));
    OpLoadA(cpu, 0xe0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    return ExecutionReturned(0x80f337u);
}

Lufia2ExecutionResult Lufia2FieldCopyScenePalette(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->decimal)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xf338u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_FIELD_PALETTE_SOURCE);
    OpTax(cpu);
    OpLda(memory, cpu, WRAM_FIELD_PALETTE_SKIP_BYTES);
    cpu->carry = 0u;
    OpAdcValue(cpu, WRAM_CGRAM_BUFFER);
    OpTay(cpu);
    OpLoadA(cpu, 0x00ffu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_PALETTE_SKIP_BYTES));
    PushDataBank(memory, cpu);
    OpMoveNext(memory, cpu, 0u, 0x9bu);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_CGRAM_BUFFER));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x80f35au);
}
