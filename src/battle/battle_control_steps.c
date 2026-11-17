#include "core/cpu_internal.h"
#include "lufia2/battle_control_steps.h"

Lufia2ExecutionResult Lufia2BattleControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x81de9eu:
        SimulateJslFrame(memory, cpu, cpu->program_bank, 0xdea1u);
        cpu->program_bank = 0x85u;
        return ExecutionHandoff(cpu, 0x85ec81u);
    case 0x858e97u: case 0x859c7au: case 0x859cd6u:
    case 0x858f49u: case 0x85aa7eu: {
        const uint16_t low = Pull8(memory, cpu);
        const uint16_t address = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
        if (entry != 0x858f49u && entry != 0x85aa7eu)
            cpu->program_bank = Pull8(memory, cpu);
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) |
            (uint16_t)(address + 1u));
    }
    default:
        return ExecutionHandoff(cpu, entry);
    }
}
