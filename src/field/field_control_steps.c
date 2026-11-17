#include "core/cpu_internal.h"
#include "lufia2/field_control_steps.h"
#include "lufia2/field_region_control_steps.h"

Lufia2ExecutionResult Lufia2FieldControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != entry >> 16 || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    uint32_t target;
    switch (entry) {
    case 0x8381cbu: target = 0x80cbaeu; break;
    case 0x83c1feu: target = 0x8ebbafu; break;
    case 0x83c20fu: target = 0x83d350u; break;
    case 0x83c213u: target = 0x8eb63bu; break;
    case 0x83bc25u:
        SetIndexWidth(cpu, 1u);
        return ExecutionHandoff(cpu, 0x83bc27u);
    case 0x83c0f9u: case 0x83bc27u: case 0x83c245u: {
        const uint16_t low = Pull8(memory, cpu);
        const uint16_t address = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) |
            (uint16_t)(address + 1u));
    }
    default:
        return ExecutionHandoff(cpu, entry);
    }
    SimulateJslFrame(memory, cpu, cpu->program_bank, (uint16_t)(entry + 3u));
    cpu->program_bank = (uint8_t)(target >> 16);
    return ExecutionHandoff(cpu, target);
}

Lufia2ExecutionResult Lufia2FieldRegionControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x83u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    uint32_t target;
    uint8_t frame_size;
    switch (entry) {
    case 0x83827du:
        PullDataBank(memory, cpu);
        return ExecutionHandoff(cpu, 0x83827eu);
    case 0x838265u:
        target = 0x83b66eu;
        frame_size = 2u;
        break;
    case 0x838268u:
        target = 0x83b747u;
        frame_size = 2u;
        break;
    case 0x83827au:
        target = 0x83b711u;
        frame_size = 2u;
        break;
    case 0x83827eu:
        target = 0x83c0efu;
        frame_size = 2u;
        break;
    case 0x838281u:
        target = 0x83fb71u;
        frame_size = 3u;
        break;
    case 0x838289u:
        target = 0x83816du;
        frame_size = 2u;
        break;
    case 0x838292u:
        target = 0x83813bu;
        frame_size = 2u;
        break;
    case 0x83829bu:
        target = 0x838386u;
        frame_size = 2u;
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
