#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/system.h"

static Lufia2ExecutionResult SceneResetUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2SceneUploadRequestedTilemaps(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x808285u);
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x808290u, 0x8087fcu, 2u, 0x80u))
        return SceneResetUnwound(0x808290u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x808298u);
}
