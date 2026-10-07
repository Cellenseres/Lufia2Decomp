#include "core/cpu_ops.h"
#include "lufia2/system.h"
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
