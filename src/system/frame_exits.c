#include "core/cpu_internal.h"
#include "lufia2/frame_exit_steps.h"

Lufia2ExecutionResult Lufia2FrameExitStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x83829eu:
        UnpackStatus(cpu, Pull8(memory, cpu));
        return ExecutionHandoff(cpu, 0x83829fu);
    case 0x83829fu:
    case 0x85ecdau:
    case 0x85ecefu: {
        const uint16_t low = Pull8(memory, cpu);
        const uint16_t address = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
        if (entry != 0x83829fu)
            cpu->program_bank = Pull8(memory, cpu);
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) |
            (uint16_t)(address + 1u));
    }
    default:
        return ExecutionHandoff(cpu, entry);
    }
}
