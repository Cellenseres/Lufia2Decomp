#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "field/event_script_internal.h"
#include "lufia2/field.h"

static uint8_t EventReaderReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && !cpu->index_is_8_bit && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2FieldRewindEventByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!EventReaderReady(cpu) || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x80e8d0u);
    OpDey(cpu);
    if (!cpu->negative) {
        OpLda(memory, cpu, EVENT_SCRIPT_BANK);
        OpDecA(cpu);
        OpSta(memory, cpu, EVENT_SCRIPT_BANK);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        OpLdy(cpu, 0xffffu);
    }
    return ExecutionReturned(0x80e8e1u);
}

Lufia2ExecutionResult Lufia2FieldReadEventWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EventReaderReady(cpu) || !cpu->accumulator_is_8_bit || !child)
        return ExecutionHandoff(cpu, 0x80e8adu);
    const uint32_t sites[] = {0x80e8adu, 0x80e8b1u};
    for (unsigned byte = 0u; byte < 2u; ++byte) {
        if (!CallChildWithFrame(memory, cpu, child, context, sites[byte], 0x80e8b9u, 2u, 0x80u)) {
            Lufia2ExecutionResult result = ExecutionReturned(sites[byte]);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        if (!byte)
            PushAccumulator8(memory, cpu);
    }
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x80e8b8u);
}

Lufia2ExecutionResult Lufia2FieldSetEventPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!EventReaderReady(cpu))
        return ExecutionHandoff(cpu, 0x80e8f4u);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, EVENT_SCRIPT_BASE_BANK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    if (!cpu->negative) {
        OpOraValue(cpu, EVENT_BANK_WINDOW);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, EVENT_SCRIPT_BASE_BANK);
        OpIncA(cpu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        OpRepWidths(cpu, 0x20u);
    }
    return ExecutionReturned(0x80e911u);
}
