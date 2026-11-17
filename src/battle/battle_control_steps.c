#include "core/cpu_internal.h"
#include "lufia2/battle_control_steps.h"
#include "lufia2/battle_target_control_steps.h"

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

Lufia2ExecutionResult Lufia2BattleTargetControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x81u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    uint32_t target;
    uint8_t frame_size;
    switch (entry) {
    case 0x81ccacu:
        target = 0x81be58u;
        frame_size = 2u;
        break;
    case 0x81ccd8u:
        target = 0x81be58u;
        frame_size = 2u;
        break;
    case 0x81ccdbu:
        target = 0x859dd4u;
        frame_size = 3u;
        break;
    case 0x81ccdfu:
        target = 0x81d9d0u;
        frame_size = 2u;
        break;
    case 0x81cce8u:
        target = 0x85ec81u;
        frame_size = 3u;
        break;
    case 0x81d581u:
        target = 0x81bebcu;
        frame_size = 3u;
        break;
    case 0x81d5afu:
        target = 0x81d92cu;
        frame_size = 2u;
        break;
    case 0x81d5b4u:
        target = 0x81d920u;
        frame_size = 2u;
        break;
    case 0x81d5e3u:
        target = 0x808878u;
        frame_size = 3u;
        break;
    case 0x81d618u:
        target = 0x81d948u;
        frame_size = 2u;
        break;
    case 0x81d635u:
        target = 0x81d9d0u;
        frame_size = 2u;
        break;
    case 0x81d640u:
        target = 0x859cc0u;
        frame_size = 3u;
        break;
    case 0x81d644u:
        target = 0x859c64u;
        frame_size = 3u;
        break;
    case 0x81d64au:
        target = 0x85ec81u;
        frame_size = 3u;
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    if (frame_size == 3u) {
        SimulateJslFrame(memory, cpu, cpu->program_bank, (uint16_t)(entry + 3u));
        cpu->program_bank = (uint8_t)(target >> 16);
    } else {
        SimulateJsrFrame(memory, cpu, (uint16_t)(entry + 2u));
    }
    return ExecutionHandoff(cpu, target);
}
