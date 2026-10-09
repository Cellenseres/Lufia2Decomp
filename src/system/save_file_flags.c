#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    SRAM_FILE_FLAGS_LONG = 0x700000u,
    SAVE_FILE_HIGH_FLAGS = 0xf0u
};

Lufia2ExecutionResult Lufia2SaveTestFileHighFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80905fu);
    OpPushX(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x809064u, 0x8091d3u, 2u, cpu->program_bank)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x809064u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpLda(memory, cpu, OpLongX(cpu, SRAM_FILE_FLAGS_LONG));
    OpAndValue(cpu, SAVE_FILE_HIGH_FLAGS);
    cpu->carry = !cpu->zero;
    OpPullX(memory, cpu);
    return ExecutionReturned(0x809072u);
}
