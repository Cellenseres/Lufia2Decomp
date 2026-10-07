#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    SOUND_DATA_PORT = 0x002140u,
    SOUND_PARAMETER_PORT = 0x002141u,
    SOUND_RESOURCE_TABLE = 0x9a62u,
    SOUND_RESOURCE_LIMIT = 0x8fu,
    SOUND_RESOURCE_END = 0xffu
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

static Lufia2ExecutionResult SoundChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SoundRequestMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint8_t mode, uint32_t site, uint8_t wait) {
    OpLoadA(cpu, mode);
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    Lufia2WriteSoundDriverMode(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    if (!wait)
        return 1u;
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 5u));
    return child(context, cpu, 0x809a0au, site + 3u, 2u);
}

static Lufia2ExecutionResult SoundDriverRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint32_t site, uint32_t end,
    uint8_t mode, uint8_t write_value, uint8_t read_value) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, entry);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    if (write_value)
        OpSta(memory, cpu, SOUND_DATA_PORT);
    if (!SoundRequestMode(memory, cpu, child, context, mode, site, 1u))
        return SoundChildUnwound(site + 3u);
    if (read_value)
        OpLda(memory, cpu, read_value == 2u
            ? OpAbs(cpu, SOUND_DATA_PORT) : SOUND_DATA_PORT);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(end);
}

Lufia2ExecutionResult Lufia2SoundDriverRequest05(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundDriverRequest(memory, cpu, child, context,
        0x8095d2u, 0x8095d7u, 0x8095deu, 5u, 0u, 0u);
}

Lufia2ExecutionResult Lufia2SoundDriverRead1E(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundDriverRequest(memory, cpu, child, context,
        0x8095dfu, 0x8095e4u, 0x8095efu, 0x1eu, 0u, 1u);
}

Lufia2ExecutionResult Lufia2SoundDriverRead0F(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundDriverRequest(memory, cpu, child, context,
        0x8095f0u, 0x8095f5u, 0x809600u, 0x0fu, 0u, 1u);
}

Lufia2ExecutionResult Lufia2SoundDriverWrite0C(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundDriverRequest(memory, cpu, child, context,
        0x809612u, 0x80961bu, 0x809622u, 0x0cu, 1u, 0u);
}

Lufia2ExecutionResult Lufia2SoundDriverWrite0D(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundDriverRequest(memory, cpu, child, context,
        0x809623u, 0x80962cu, 0x809633u, 0x0du, 1u, 0u);
}

Lufia2ExecutionResult Lufia2SoundDriverRead18(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundDriverRequest(memory, cpu, child, context,
        0x809634u, 0x809639u, 0x809643u, 0x18u, 0u, 2u);
}

Lufia2ExecutionResult Lufia2SoundDriverWrite09(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundDriverRequest(memory, cpu, child, context,
        0x809644u, 0x80964du, 0x809654u, 9u, 1u, 0u);
}

Lufia2ExecutionResult Lufia2SoundDriverRequest15(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SoundDriverRequest(memory, cpu, child, context,
        0x809685u, 0x80968au, 0x809691u, 0x15u, 0u, 0u);
}

Lufia2ExecutionResult Lufia2SendUncheckedSoundCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80956au);
    SoundSaveAccumulator(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    OpSta(memory, cpu, SOUND_DATA_PORT);
    if (!SoundRequestMode(memory, cpu, child, context, 4u, 0x809574u, 1u))
        return SoundChildUnwound(0x809577u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SoundRestoreAccumulator(memory, cpu);
    return ExecutionReturned(0x80957cu);
}

Lufia2ExecutionResult Lufia2SoundDriverWrite1A(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809655u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SOUND_PARAMETER_INHIBIT));
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, SOUND_DATA_PORT);
        OpTxa(cpu);
        OpSta(memory, cpu, SOUND_PARAMETER_PORT);
        SoundRequestMode(memory, cpu, child, context, 0x1au, 0x80966au, 0u);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80966eu);
}

Lufia2ExecutionResult Lufia2SoundDriverWrite1B(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80966fu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    OpSta(memory, cpu, SOUND_DATA_PORT);
    OpTxa(cpu);
    OpSta(memory, cpu, SOUND_PARAMETER_PORT);
    if (!SoundRequestMode(memory, cpu, child, context, 0x1bu, 0x80967du, 1u))
        return SoundChildUnwound(0x809680u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x809684u);
}

Lufia2ExecutionResult Lufia2PlaySoundResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809554u);
    SoundSaveAccumulator(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    OpCmpValue(cpu, SOUND_RESOURCE_LIMIT);
    if (!cpu->carry) {
        SimulateJsrFrame(memory, cpu, 0x9563u);
        if (!child(context, cpu, 0x80957du, 0x809561u, 2u))
            return SoundChildUnwound(0x809561u);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    SoundRestoreAccumulator(memory, cpu);
    return ExecutionReturned(0x809569u);
}

Lufia2ExecutionResult Lufia2PrepareSongResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80969fu);
    SoundSaveAccumulator(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1u);
    SimulateJsrFrame(memory, cpu, 0x96a8u);
    if (!child(context, cpu, 0x80941au, 0x8096a6u, 2u))
        return SoundChildUnwound(0x8096a6u);
    if (!cpu->carry &&
        !SoundRequestMode(memory, cpu, child, context, 7u, 0x8096adu, 1u))
        return SoundChildUnwound(0x8096b0u);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    SoundRestoreAccumulator(memory, cpu);
    return ExecutionReturned(0x8096b8u);
}

Lufia2ExecutionResult Lufia2LoadSoundResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->program_bank != 0x80u ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x80957du);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    SoundSaveAccumulator(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT));
    if (!SoundRequestMode(memory, cpu, child, context, 0x19u, 0x809585u, 1u))
        return SoundChildUnwound(0x809588u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    SoundRestoreAccumulator(memory, cpu);
    SoundSaveAccumulator(memory, cpu);
    SetAccumulatorWidth(cpu, 0u);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, SOUND_RESOURCE_TABLE));
    OpTax(cpu);
    SetAccumulatorWidth(cpu, 1u);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpInx(cpu);
    OpCmpValue(cpu, SOUND_RESOURCE_END);
    if (cpu->zero) {
        SoundRestoreAccumulator(memory, cpu);
        cpu->carry = 1u;
        return ExecutionReturned(0x8095a5u);
    }
    OpSta(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT));
    if (!SoundRequestMode(memory, cpu, child, context, 0x13u, 0x8095abu, 1u))
        return SoundChildUnwound(0x8095aeu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MUSIC_APU_REPLY)));
    OpWriteX(memory, cpu, OpAbs(cpu, SOUND_DATA_PORT), cpu->y);
    if (!SoundRequestMode(memory, cpu, child, context, 0x11u, 0x8095b9u, 1u))
        return SoundChildUnwound(0x8095bcu);
    OpLdy(cpu, 0u);
    for (uint32_t bytes = 0u; bytes < 65536u; ++bytes) {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpCmpValue(cpu, SOUND_RESOURCE_END);
        if (cpu->zero) {
            SoundRestoreAccumulator(memory, cpu);
            cpu->carry = 0u;
            return ExecutionReturned(0x8095d1u);
        }
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_SOUND_RESOURCE_OUTPUT));
        OpInx(cpu);
    }
    return ExecutionHandoff(cpu, 0x8095c2u);
}
