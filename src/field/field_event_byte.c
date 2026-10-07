#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/field.h"

Lufia2ExecutionResult Lufia2FieldReadEventByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80e8b9u);
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpIny(cpu);
    if (!cpu->negative) {
        PushAccumulator8(memory, cpu);
        OpLda(memory, cpu, EVENT_SCRIPT_BANK);
        OpIncA(cpu);
        OpSta(memory, cpu, EVENT_SCRIPT_BANK);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadA8(cpu, Pull8(memory, cpu));
        OpLdy(cpu, EVENT_BANK_WINDOW);
    }
    return ExecutionReturned(0x80e8cfu);
}
