#include "lufia2/world_map.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"

Lufia2ExecutionResult Lufia2WorldMapRandomScaledWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || !child || cpu->decimal ||
            cpu->program_bank != 0x86u)
        return ExecutionHandoff(cpu, 0x869e3bu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    for (uint8_t byte = 0u; byte < 2u; ++byte) {
        uint32_t site = byte ? 0x869e44u : 0x869e3eu;
        if (!CallChildWithFrame(memory, cpu, child, context,
                site, 0x8082c7u, 3u, 0x86u)) {
            Lufia2ExecutionResult result = ExecutionReturned(site);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
            return ExecutionHandoff(cpu, site + 4u);
        OpSta(memory, cpu, OpDp(cpu, byte));
    }
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x869e4au, 0x86a52fu, 2u, 0x86u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x869e4au);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x869e4eu);
}
