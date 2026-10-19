#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum { FRAME_POLL_LIMIT = 65536, FRAME_COUNT_LIMIT = 4096 };

Lufia2ExecutionResult Lufia2FrameWaitPollStep(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    uint32_t poll;
    switch (entry) {
    case 0x8293c8u:
    case 0x8293cau:
        poll = 0x8293c8u;
        break;
    case 0x83900eu:
    case 0x839010u:
        poll = 0x83900eu;
        break;
    case 0x8eb552u:
    case 0x8eb554u:
        poll = 0x8eb552u;
        break;
    case 0x868b4eu:
    case 0x868b50u:
        poll = 0x868b4eu;
        break;
    case 0x869752u:
    case 0x869754u:
        poll = 0x869752u;
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    EmitExecutionCheckpoint(memory, cpu, entry);
    if (entry == poll) {
        OpCmp(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
        return ExecutionHandoff(cpu, entry + 2u);
    }
    return ExecutionHandoff(cpu, cpu->zero ? poll : entry + 2u);
}

static Lufia2ExecutionResult WaitForFrameChange(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu, uint32_t poll_pc,
                                                uint32_t return_pc) {
    OpLda(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
    for (uint32_t polls = 0; polls < FRAME_POLL_LIMIT; ++polls) {
        OpCmp(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
        if (!cpu->zero)
            return ExecutionReturned(return_pc);
    }
    return ExecutionHandoff(cpu, poll_pc);
}

static void SaveAccumulator(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
}

static void RestoreAccumulator(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

Lufia2ExecutionResult Lufia2FieldWaitForNextFrame(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83900cu);
    return WaitForFrameChange(memory, cpu, 0x83900eu, 0x839012u);
}

Lufia2ExecutionResult Lufia2ItemRegionWaitForNextFrame(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x8eu)
        return ExecutionHandoff(cpu, 0x8eb550u);
    return WaitForFrameChange(memory, cpu, 0x8eb552u, 0x8eb556u);
}

Lufia2ExecutionResult Lufia2MenuWaitForNextFrame(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x86u)
        return ExecutionHandoff(cpu, 0x868b48u);
    SaveAccumulator(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    Lufia2ExecutionResult result =
        WaitForFrameChange(memory, cpu, 0x868b4eu, 0x868b54u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    RestoreAccumulator(memory, cpu);
    return result;
}

Lufia2ExecutionResult Lufia2WorldWaitForNextFrame(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x86u)
        return ExecutionHandoff(cpu, 0x86974fu);
    SaveAccumulator(memory, cpu);
    Lufia2ExecutionResult result =
        WaitForFrameChange(memory, cpu, 0x869752u, 0x869757u);
    if (result.flow == LUFIA2_EXECUTION_RETURNED)
        RestoreAccumulator(memory, cpu);
    return result;
}

Lufia2ExecutionResult Lufia2WorldWaitFrames(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *context) {
    if (cpu->program_bank != 0x86u || !child)
        return ExecutionHandoff(cpu, 0x869748u);
    for (unsigned frames = 0; frames < FRAME_COUNT_LIMIT; ++frames) {
        if (!CallChildWithFrame(memory, cpu, child, context, 0x869748u, 0x86974fu, 2u,
                                0x86u))
            return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x869748u,
                                           0u};
        OpDecA(cpu);
        if (cpu->zero)
            return ExecutionReturned(0x86974eu);
    }
    return ExecutionHandoff(cpu, 0x869748u);
}

Lufia2ExecutionResult Lufia2FrameWaitRestoreRegisters(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      uint32_t entry) {
    uint32_t return_pc;
    switch (entry) {
    case 0x8293ccu:
        return_pc = 0x8293ceu;
        break;
    case 0x868b52u:
        return_pc = 0x868b54u;
        break;
    case 0x869756u:
        return_pc = 0x869757u;
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u || cpu->stack < 0x1f00u || cpu->stack > 0x1ff8u ||
        (entry != 0x869756u && !cpu->accumulator_is_8_bit))
        return ExecutionHandoff(cpu, entry);
    if (entry != 0x869756u)
        UnpackStatus(cpu, Pull8(memory, cpu));
    RestoreAccumulator(memory, cpu);
    return ExecutionHandoff(cpu, return_pc);
}
