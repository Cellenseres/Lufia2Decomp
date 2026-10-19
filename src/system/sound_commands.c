#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "lufia2/sound_phase.h"
#include "system/wram.h"

enum {
    SOUND_COMMAND_PORT = 0x002140u,
    SOUND_MODE_PORT = 0x002142u,
    SOUND_MODE_MIRROR_PORT = 0x002143u,
    SOUND_COMMAND_LIMIT = 0x8fu,
    SOUND_DRIVER_COMMAND_MODE = 4u
};

static void SoundPushAccumulator(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
}

static void SoundPullAccumulator(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

Lufia2ExecutionResult Lufia2SoundCommandStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x80953bu:
        SoundPushAccumulator(memory, cpu);
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x80953cu:
    case 0x8099fdu:
        Push8(memory, cpu, PackStatus(cpu));
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x80953du:
    case 0x8099feu:
        UnpackStatus(cpu, (uint8_t)(PackStatus(cpu) | 0x20u));
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x80953fu:
        UnpackStatus(cpu, (uint8_t)(PackStatus(cpu) & ~0x10u));
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x809543u:
        return ExecutionHandoff(cpu, cpu->carry ? 0x809551u : entry + 2u);
    case 0x80954bu:
    case 0x80954eu:
        SimulateJsrFrame(memory, cpu, (uint16_t)(entry + 2u));
        return ExecutionHandoff(cpu, entry == 0x80954bu ? 0x8099fdu : 0x809a0au);
    case 0x809551u:
    case 0x809a08u:
        UnpackStatus(cpu, Pull8(memory, cpu));
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x809552u:
        SoundPullAccumulator(memory, cpu);
        return ExecutionHandoff(cpu, entry + 1u);
    case 0x809553u:
    case 0x809a09u: {
        const uint16_t low = Pull8(memory, cpu);
        const uint16_t address = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
        if (entry == 0x809553u)
            cpu->program_bank = Pull8(memory, cpu);
        return ExecutionHandoff(cpu,
            ((uint32_t)cpu->program_bank << 16) | (uint16_t)(address + 1u));
    }
    default:
        break;
    }
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x809541u:
        OpCmpValue(cpu, SOUND_COMMAND_LIMIT);
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x809545u:
        OpSta(memory, cpu, SOUND_COMMAND_PORT);
        return ExecutionHandoff(cpu, entry + 4u);
    case 0x809549u:
        OpLoadA(cpu, SOUND_DRIVER_COMMAND_MODE);
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x809a00u:
    case 0x809a04u:
        OpSta(memory, cpu, entry == 0x809a00u ? SOUND_MODE_PORT : SOUND_MODE_MIRROR_PORT);
        return ExecutionHandoff(cpu, entry + 4u);
    default:
        return ExecutionHandoff(cpu, entry);
    }
}

Lufia2ExecutionResult Lufia2WriteSoundDriverMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    OpSta(memory, cpu, SOUND_MODE_PORT);
    OpSta(memory, cpu, SOUND_MODE_MIRROR_PORT);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x809a09u);
}

static Lufia2ExecutionResult SoundUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2SendSoundCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80953bu);
    SoundPushAccumulator(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    OpCmpValue(cpu, SOUND_COMMAND_LIMIT);
    if (!cpu->carry) {
        OpSta(memory, cpu, SOUND_COMMAND_PORT);
        OpLoadA(cpu, SOUND_DRIVER_COMMAND_MODE);
        SimulateJsrFrame(memory, cpu, 0x954du);
        Lufia2WriteSoundDriverMode(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        SimulateJsrFrame(memory, cpu, 0x9550u);
        if (!child(context, cpu, 0x809a0au, 0x80954eu, 2u))
            return SoundUnwound(0x80954eu);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    SoundPullAccumulator(memory, cpu);
    return ExecutionReturned(0x809553u);
}

Lufia2ExecutionResult Lufia2SendImmediateSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x848775u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, WRAM_FIELD_MAP_FLAGS);
    BitImmediate8(cpu, 2u);
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu);
        SimulateJslFrame(memory, cpu, 0x84u, 0x8782u);
        if (!child(context, cpu, 0x80953bu, 0x84877fu, 3u))
            return SoundUnwound(0x84877fu);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, WRAM_SOUND_COMMAND);
    }
    return ExecutionReturned(0x848789u);
}
