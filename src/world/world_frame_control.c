#include "core/cpu_internal.h"
#include "lufia2/world_frame_control_steps.h"

Lufia2ExecutionResult Lufia2WorldFrameControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x86929fu:
        return ExecutionHandoff(cpu, 0x86925cu);
    case 0x869282u:
    case 0x86929cu:
        SimulateJsrFrame(memory, cpu, (uint16_t)(entry + 2u));
        return ExecutionHandoff(cpu, entry == 0x869282u ? 0x86e11fu : 0x86974fu);
    default:
        return ExecutionHandoff(cpu, entry);
    }
}
