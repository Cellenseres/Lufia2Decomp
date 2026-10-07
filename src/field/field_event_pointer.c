#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum { EVENT_RUNNING_POINTER = WRAM_FIELD_EVENT_RUNNING_POINTER };

Lufia2ExecutionResult Lufia2FieldPublishEventPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80e8e2u);
    PushAccumulator16(memory, cpu);
    OpTya(cpu);
    OpSta(memory, cpu, EVENT_RUNNING_POINTER);
    OpSepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, WRAM_FIELD_EVENT_RUNNING_BANK);
    OpRepWidths(cpu, 0x20u);
    PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x80e8f3u);
}
