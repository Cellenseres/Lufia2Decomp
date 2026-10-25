#include "core/child_call.h"
#include "core/cpu_internal.h"
#include "lufia2/special_menu.h"

Lufia2ExecutionResult Lufia2SpecialMenuSendSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, 0x84a607u);
    if (!CallChildWithFrame(memory, cpu, child, context,
                            0x84a607u, 0x80953bu, 3u, 0x84u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x84a607u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, (uint32_t)cpu->program_bank << 16 | 0xa60bu);
    return ExecutionReturned(0x84a60bu);
}

Lufia2ExecutionResult Lufia2SpecialMenuPlaySong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, 0x84a60cu);
    if (!CallChildWithFrame(memory, cpu, child, context,
                            0x84a60cu, 0x8093feu, 3u, 0x84u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x84a60cu);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, (uint32_t)cpu->program_bank << 16 | 0xa610u);
    return ExecutionReturned(0x84a610u);
}
