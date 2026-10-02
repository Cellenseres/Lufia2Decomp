#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2SceneScriptReadOperand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpIny(cpu);
    if (!cpu->negative) {
        if (cpu->accumulator_is_8_bit)
            PushAccumulator8(memory, cpu);
        else
            PushAccumulator16(memory, cpu);
        Push8(memory, cpu, PackStatus(cpu));
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK & 0xffffu));
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK & 0xffffu));
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        UnpackStatus(cpu, Pull8(memory, cpu));
        if (cpu->accumulator_is_8_bit)
            OpLoadA(cpu, Pull8(memory, cpu));
        else
            PullAccumulator16(memory, cpu);
        if (cpu->index_is_8_bit)
            return ExecutionHandoff(cpu, 0x80c0ccu);
        OpLdy(cpu, 0x8000u);
    }
    return ExecutionReturned(0x80c0cfu);
}
