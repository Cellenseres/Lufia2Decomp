#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    SOUND_SOURCE_BANK = 0x5fu,
    SOUND_DATA_PORT = 0x2140u,
    SOUND_TRANSFER_PORT = 0x002143u,
    SOUND_DRIVER_SIGNATURE = 0xbbaau
};

static void SoundSaveAccumulator(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
}

static void SoundRestoreAccumulator(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

Lufia2ExecutionResult Lufia2WriteSoundTransferMarker(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x8099f4u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, SOUND_TRANSFER_PORT);
    ExchangeAccumulatorBytes(cpu);
    return ExecutionReturned(0x8099fcu);
}

Lufia2ExecutionResult Lufia2AdvanceSoundSourceBank(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8097dau);
    OpLdy(cpu, 0x8000u);
    OpLda(memory, cpu, OpDp(cpu, SOUND_SOURCE_BANK));
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, SOUND_SOURCE_BANK));
    SoundSaveAccumulator(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8097e4u);
}

Lufia2ExecutionResult Lufia2CheckSoundDriverSignature(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    cpu->carry = 0u;
    SoundSaveAccumulator(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0u);
    OpLoadA(cpu, SOUND_DRIVER_SIGNATURE);
    OpCmp(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT));
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 1u);
        OpLda(memory, cpu, OpStack(cpu, 1u));
        OpOraValue(cpu, 1u);
        OpSta(memory, cpu, OpStack(cpu, 1u));
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    SoundRestoreAccumulator(memory, cpu);
    return ExecutionReturned(0x8099c9u);
}

static Lufia2ExecutionResult SoundParameterCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint32_t site, uint32_t end, uint8_t command) {
    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    SoundSaveAccumulator(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT));
    OpLoadA(cpu, command);
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    Lufia2WriteSoundDriverMode(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 5u));
    if (!child(context, cpu, 0x809a0au, site + 3u, 2u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site + 3u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    SoundRestoreAccumulator(memory, cpu);
    return ExecutionReturned(end);
}

Lufia2ExecutionResult Lufia2SoundDriverWrite1F(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundParameterCommand(memory, cpu, child, context,
        0x8099cau, 0x8099d0u, 0x8099d7u, 0x1fu);
}

Lufia2ExecutionResult Lufia2SoundDriverWrite20(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundParameterCommand(memory, cpu, child, context,
        0x8099d8u, 0x8099deu, 0x8099e5u, 0x20u);
}

Lufia2ExecutionResult Lufia2SoundDriverWrite21(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundParameterCommand(memory, cpu, child, context,
        0x8099e6u, 0x8099ecu, 0x8099f3u, 0x21u);
}
