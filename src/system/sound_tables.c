#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    SOUND_DATA_PORT = 0x002140u,
    SOUND_TABLE_ADDRESS = 0x5800u,
    SOUND_TABLE_SLOTS = 32u
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

static Lufia2ExecutionResult SoundUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SoundRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint8_t command, uint32_t site) {
    OpLoadA(cpu, command);
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    Lufia2WriteSoundDriverMode(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 5u));
    return child(context, cpu, 0x809a0au, site + 3u, 2u);
}

Lufia2ExecutionResult Lufia2SoundDriverWrite0A(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8096ccu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    OpSta(memory, cpu, SOUND_DATA_PORT);
    if (!SoundRequest(memory, cpu, child, context, 0x0au, 0x8096d5u))
        return SoundUnwound(0x8096d8u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8096dcu);
}

Lufia2ExecutionResult Lufia2SoundDriverWriteComplement10(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8096ddu);
    SoundPushAccumulator(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpSta(memory, cpu, SOUND_DATA_PORT);
    if (!SoundRequest(memory, cpu, child, context, 0x10u, 0x8096e9u))
        return SoundUnwound(0x8096ecu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SoundPullAccumulator(memory, cpu);
    return ExecutionReturned(0x8096f1u);
}

Lufia2ExecutionResult Lufia2SoundDriverRead08(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8096f2u);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    if (!SoundRequest(memory, cpu, child, context, 8u, 0x8096f7u))
        return SoundUnwound(0x8096fau);
    OpLda(memory, cpu, SOUND_DATA_PORT);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x809702u);
}

Lufia2ExecutionResult Lufia2WaitSoundDriverFlagsClear(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8096b9u);
    SoundPushAccumulator(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    for (uint32_t polls = 0u; polls < 65536u; ++polls) {
        SimulateJslFrame(memory, cpu, cpu->program_bank, 0x96c3u);
        if (!child(context, cpu, 0x8095f0u, 0x8096c0u, 3u))
            return SoundUnwound(0x8096c0u);
        OpBitValue(cpu, 0x0cu);
        if (cpu->zero) {
            PullDataBank(memory, cpu);
            UnpackStatus(cpu, Pull8(memory, cpu));
            SoundPullAccumulator(memory, cpu);
            return ExecutionReturned(0x8096cbu);
        }
    }
    return ExecutionHandoff(cpu, 0x8096c0u);
}

Lufia2ExecutionResult Lufia2InitializeSoundResourceTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x809703u);
    SoundPushAccumulator(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    OpLdx(cpu, SOUND_TABLE_ADDRESS);
    OpWriteX(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT), cpu->x);
    if (!SoundRequest(memory, cpu, child, context, 0x11u, 0x809713u))
        return SoundUnwound(0x809716u);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT));
    if (!SoundRequest(memory, cpu, child, context, 0x13u, 0x809720u))
        return SoundUnwound(0x809723u);
    OpLdx(cpu, SOUND_TABLE_SLOTS);
    OpLoadA(cpu, 0u);
    do {
        SoundPushAccumulator(memory, cpu);
        SimulateJsrFrame(memory, cpu, 0x972eu);
        if (!child(context, cpu, 0x809886u, 0x80972cu, 2u))
            return SoundUnwound(0x80972cu);
        SoundPullAccumulator(memory, cpu);
        OpIncA(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    if (!SoundRequest(memory, cpu, child, context, 0x12u, 0x809736u))
        return SoundUnwound(0x809739u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MUSIC_APU_UPLOAD_ADDRESS), cpu->x);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    SoundPullAccumulator(memory, cpu);
    return ExecutionReturned(0x809746u);
}
