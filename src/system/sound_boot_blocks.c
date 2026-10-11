#include "lufia2/system.h"
#include "core/cpu_ops.h"

enum {
    BOOT_SOUND_SOURCE = 0x5d,
    BOOT_SOUND_DATA_PORT = 0x2140,
    BOOT_SOUND_FLAG_PORT = 0x2141,
    BOOT_SOUND_ADDRESS_PORT = 0x2142,
    BOOT_SOUND_POLL_LIMIT = 65536,
    BOOT_SOUND_BLOCK_LIMIT = 4096
};

static uint8_t BootSoundAcknowledged(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (uint32_t polls = 0u; polls < BOOT_SOUND_POLL_LIMIT; ++polls) {
        OpCmp(memory, cpu, OpAbs(cpu, BOOT_SOUND_DATA_PORT));
        if (cpu->zero)
            return 1u;
    }
    return 0u;
}

static void ReadBootSoundSource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, BOOT_SOUND_SOURCE));
    OpIny(cpu);
}

static uint8_t BeginBootSoundBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushAccumulator8(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    ReadBootSoundSource(memory, cpu);
    OpIny(cpu);
    OpTax(cpu);
    ReadBootSoundSource(memory, cpu);
    OpIny(cpu);
    OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_ADDRESS_PORT));
    OpSepWidths(cpu, 0x20u);
    OpCpx(cpu, 1u);
    OpLoadA(cpu, 0u);
    RolA8(cpu);
    OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_FLAG_PORT));
    OpAdcValue(cpu, 0x7fu);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_DATA_PORT));
    return BootSoundAcknowledged(memory, cpu);
}

static uint8_t SendBootSoundBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult *result) {
    ReadBootSoundSource(memory, cpu);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    do {
        OpRepWidths(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_DATA_PORT));
        OpSepWidths(cpu, 0x20u);
        OpDex(cpu);
        if (cpu->zero)
            break;
        ExchangeAccumulatorBytes(cpu);
        ReadBootSoundSource(memory, cpu);
        ExchangeAccumulatorBytes(cpu);
        if (!BootSoundAcknowledged(memory, cpu)) {
            *result = ExecutionHandoff(cpu, 0x809c30u);
            return 0u;
        }
        OpIncA(cpu);
    } while (1);
    if (!BootSoundAcknowledged(memory, cpu)) {
        *result = ExecutionHandoff(cpu, 0x809c40u);
        return 0u;
    }
    do {
        OpAdcValue(cpu, 3u);
    } while (cpu->zero);
    return 1u;
}

Lufia2ExecutionResult Lufia2UploadBootSoundBlocks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809c0fu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLdy(cpu, 0u);
    OpLoadA(cpu, 0xbbaau);
    if (!BootSoundAcknowledged(memory, cpu))
        return ExecutionHandoff(cpu, 0x809c18u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xccu);
    for (uint32_t blocks = 0u; blocks < BOOT_SOUND_BLOCK_LIMIT; ++blocks) {
        if (!BeginBootSoundBlock(memory, cpu))
            return ExecutionHandoff(cpu, 0x809c69u);
        if (!cpu->overflow) {
            UnpackStatus(cpu, Pull8(memory, cpu));
            return ExecutionReturned(0x809c71u);
        }
        Lufia2ExecutionResult result;
        if (!SendBootSoundBlock(memory, cpu, &result))
            return result;
    }
    return ExecutionHandoff(cpu, 0x809c49u);
}
