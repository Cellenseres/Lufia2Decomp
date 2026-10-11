#include "lufia2/field.h"
#include "core/cpu_ops.h"
#include "core/joypad.h"

Lufia2ExecutionResult Lufia2FieldTakeHighButtons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x838674u);
    (void)TakeButtonPress8(memory, cpu, JOYPAD_HIGH_BYTE);
    return ExecutionReturned(0x83867au);
}

Lufia2ExecutionResult Lufia2FieldReadFacingStepCode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x83c246u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x83c1b0u));
    return ExecutionReturned(0x83c24eu);
}
