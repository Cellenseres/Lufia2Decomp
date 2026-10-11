#include "lufia2/menu.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum { MENU_FRAME_POLL_LIMIT = 65536 };

Lufia2ExecutionResult Lufia2WaitMenuFrameChange(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u)
        return ExecutionHandoff(cpu, 0x8293c2u);
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
    for (uint32_t polls = 0u; polls < MENU_FRAME_POLL_LIMIT; ++polls) {
        OpCmp(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
        if (!cpu->zero) {
            UnpackStatus(cpu, Pull8(memory, cpu));
            if (cpu->accumulator_is_8_bit)
                LoadA8(cpu, Pull8(memory, cpu));
            else
                PullAccumulator16(memory, cpu);
            return ExecutionReturned(0x8293ceu);
        }
    }
    return ExecutionHandoff(cpu, 0x8293c8u);
}
