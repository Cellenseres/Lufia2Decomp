#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/frame_wait.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum { FRAME_POLL_LIMIT = 65536, FRAME_COUNT_LIMIT = 4096 };

static Lufia2ExecutionResult StepFrameCounter(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu, uint32_t entry,
                                              uint32_t counter) {
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    EmitExecutionCheckpoint(memory, cpu, entry);
    if (entry == counter) {
        OpLda(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
        return ExecutionHandoff(cpu, entry + 2u);
    }
    if (entry == counter + 2u) {
        OpCmp(memory, cpu, OpDp(cpu, DP_NMI_FRAME_COUNTER));
        return ExecutionHandoff(cpu, entry + 2u);
    }
    return ExecutionHandoff(cpu, cpu->zero ? counter + 2u : entry + 2u);
}

Lufia2ExecutionResult Lufia2FrameWaitCounterStep(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu, uint32_t entry) {
    uint32_t counter;
    switch (entry) {
    case 0x8293c6u:
    case 0x8293c8u:
    case 0x8293cau:
        counter = 0x8293c6u;
        break;
    case 0x83900cu:
    case 0x83900eu:
    case 0x839010u:
        counter = 0x83900cu;
        break;
    case 0x8eb550u:
    case 0x8eb552u:
    case 0x8eb554u:
        counter = 0x8eb550u;
        break;
    case 0x868b4cu:
    case 0x868b4eu:
    case 0x868b50u:
        counter = 0x868b4cu;
        break;
    case 0x869750u:
    case 0x869752u:
    case 0x869754u:
        counter = 0x869750u;
        break;
    case 0x80bf0bu:
    case 0x80bf0du:
    case 0x80bf0fu:
        counter = 0x80bf0bu;
        break;
    case 0x848d4du:
    case 0x848d4fu:
    case 0x848d51u:
        counter = 0x848d4du;
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    return StepFrameCounter(memory, cpu, entry, counter);
}

Lufia2ExecutionResult Lufia2FrameWaitAdditionalStep(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    uint32_t entry) {
    uint32_t counter;
    switch (entry) {
    case 0x80937bu:
    case 0x80937du:
    case 0x80937fu:
        counter = 0x80937bu;
        break;
    case 0x848363u:
    case 0x848365u:
    case 0x848367u:
        counter = 0x848363u;
        break;
    case 0x84a5acu:
    case 0x84a5aeu:
    case 0x84a5b0u:
        counter = 0x84a5acu;
        break;
    case 0x85851cu:
    case 0x85851eu:
    case 0x858520u:
        counter = 0x85851cu;
        break;
    case 0x85ec94u:
    case 0x85ec96u:
    case 0x85ec98u:
        counter = 0x85ec94u;
        break;
    case 0x85ecb7u:
    case 0x85ecb9u:
    case 0x85ecbbu:
        counter = 0x85ecb7u;
        break;
    case 0x8eb288u:
    case 0x8eb28au:
    case 0x8eb28cu:
        counter = 0x8eb288u;
        break;
    case 0x8ec2c4u:
    case 0x8ec2c6u:
    case 0x8ec2c8u:
        counter = 0x8ec2c4u;
        break;
    default:
        return Lufia2FrameWaitCounterStep(memory, cpu, entry);
    }
    return StepFrameCounter(memory, cpu, entry, counter);
}

Lufia2ExecutionResult Lufia2FrameWaitPollStep(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu, uint32_t entry) {
    switch (entry) {
    case 0x8293c8u:
    case 0x8293cau:
    case 0x83900eu:
    case 0x839010u:
    case 0x8eb552u:
    case 0x8eb554u:
    case 0x868b4eu:
    case 0x868b50u:
    case 0x869752u:
    case 0x869754u:
        return Lufia2FrameWaitCounterStep(memory, cpu, entry);
    default:
        return ExecutionHandoff(cpu, entry);
    }
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

Lufia2ExecutionResult Lufia2FrameWaitSaveRegistersStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x8293c2u:
    case 0x868b48u:
    case 0x86974fu:
    case 0x84a5a8u:
        EmitExecutionCheckpoint(memory, cpu, entry);
        SaveAccumulator(memory, cpu);
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x8293c3u:
    case 0x868b49u:
    case 0x809378u:
    case 0x84a5a9u:
        EmitExecutionCheckpoint(memory, cpu, entry);
        Push8(memory, cpu, PackStatus(cpu));
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x8293c4u:
    case 0x868b4au:
    case 0x809379u:
    case 0x84a5aau:
        EmitExecutionCheckpoint(memory, cpu, entry);
        OpSepWidths(cpu, 0x20u);
        return ExecutionHandoff(cpu, entry + 2u);
    default:
        return ExecutionHandoff(cpu, entry);
    }
}

Lufia2ExecutionResult Lufia2FrameWaitReturnStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != entry >> 16)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x8293ceu:
    case 0x839012u:
    case 0x8eb556u:
    case 0x868b54u:
    case 0x869757u:
    case 0x80bf11u:
    case 0x848d53u:
    case 0x809382u:
    case 0x848369u:
    case 0x84a5b4u:
    case 0x858522u:
    case 0x8eb28eu: {
        const uint16_t low = Pull8(memory, cpu);
        const uint16_t address = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) |
                                       (uint16_t)(address + 1u));
    }
    default:
        return ExecutionHandoff(cpu, entry);
    }
}

Lufia2ExecutionResult Lufia2FrameWaitRegisterStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x8293ccu:
    case 0x868b52u:
    case 0x809381u:
    case 0x84a5b2u:
        UnpackStatus(cpu, Pull8(memory, cpu));
        break;
    case 0x8293cdu:
    case 0x868b53u:
    case 0x869756u:
    case 0x84a5b3u:
        RestoreAccumulator(memory, cpu);
        break;
    case 0x8293c4u:
    case 0x868b4au:
    case 0x809379u:
    case 0x84a5aau:
        UnpackStatus(cpu, (uint8_t)(PackStatus(cpu) | 0x20u));
        return ExecutionHandoff(cpu, entry + 2u);
    default:
        return Lufia2FrameWaitSaveRegistersStep(memory, cpu, entry);
    }
    return ExecutionHandoff(cpu, entry + 1u);
}

Lufia2ExecutionResult Lufia2FieldWaitForNextFrame(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83900cu);
    return WaitForFrameChange(memory, cpu, 0x83900eu, 0x839012u);
}

Lufia2ExecutionResult Lufia2TextWaitForNextFrame(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x80bf0bu);
    return WaitForFrameChange(memory, cpu, 0x80bf0du, 0x80bf11u);
}

Lufia2ExecutionResult Lufia2AncientCaveWaitForNextFrame(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, 0x848d4du);
    return WaitForFrameChange(memory, cpu, 0x848d4fu, 0x848d53u);
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

Lufia2ExecutionResult Lufia2WaitFramePreserveStatus(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x809378u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    const Lufia2ExecutionResult result =
        WaitForFrameChange(memory, cpu, 0x80937du, 0x809382u);
    if (result.flow == LUFIA2_EXECUTION_RETURNED)
        UnpackStatus(cpu, Pull8(memory, cpu));
    return result;
}

Lufia2ExecutionResult Lufia2WaitFramePreserveAccumulator(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, 0x84a5a8u);
    SaveAccumulator(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    const Lufia2ExecutionResult result =
        WaitForFrameChange(memory, cpu, 0x84a5aeu, 0x84a5b4u);
    if (result.flow == LUFIA2_EXECUTION_RETURNED) {
        UnpackStatus(cpu, Pull8(memory, cpu));
        RestoreAccumulator(memory, cpu);
    }
    return result;
}

Lufia2ExecutionResult Lufia2WaitForNextFrame(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x8eu)
        return ExecutionHandoff(cpu, 0x8eb288u);
    return WaitForFrameChange(memory, cpu, 0x8eb28au, 0x8eb28eu);
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
