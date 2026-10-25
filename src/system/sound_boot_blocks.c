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

static uint8_t BootSoundAcknowledged(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (uint32_t polls = 0u; polls < BOOT_SOUND_POLL_LIMIT; ++polls) {
        OpCmp(memory, cpu, OpAbs(cpu, BOOT_SOUND_DATA_PORT));
        if (cpu->zero)
            return 1u;
    }
    return 0u;
}

static void ReadBootSoundSource(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint32_t pc) {
    if (pc)
        EmitExecutionCheckpoint(memory, cpu, pc);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, BOOT_SOUND_SOURCE));
    if (pc)
        EmitExecutionCheckpoint(memory, cpu, pc + 2u);
    OpIny(cpu);
}

static void BootSoundCheckpoint(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint32_t pc, bool stepped) {
    if (stepped)
        EmitExecutionCheckpoint(memory, cpu, pc);
}

static void PrepareBootSoundBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  bool stepped) {
    BootSoundCheckpoint(memory, cpu, 0x809c49u, stepped);
    PushAccumulator8(memory, cpu);
    BootSoundCheckpoint(memory, cpu, 0x809c4au, stepped);
    OpRepWidths(cpu, 0x20u);
    ReadBootSoundSource(memory, cpu, stepped ? 0x809c4cu : 0u);
    BootSoundCheckpoint(memory, cpu, 0x809c4fu, stepped);
    OpIny(cpu);
    BootSoundCheckpoint(memory, cpu, 0x809c50u, stepped);
    OpTax(cpu);
    ReadBootSoundSource(memory, cpu, stepped ? 0x809c51u : 0u);
    BootSoundCheckpoint(memory, cpu, 0x809c54u, stepped);
    OpIny(cpu);
    BootSoundCheckpoint(memory, cpu, 0x809c55u, stepped);
    OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_ADDRESS_PORT));
    BootSoundCheckpoint(memory, cpu, 0x809c58u, stepped);
    OpSepWidths(cpu, 0x20u);
    BootSoundCheckpoint(memory, cpu, 0x809c5au, stepped);
    OpCpx(cpu, 1u);
    BootSoundCheckpoint(memory, cpu, 0x809c5du, stepped);
    OpLoadA(cpu, 0u);
    BootSoundCheckpoint(memory, cpu, 0x809c5fu, stepped);
    RolA8(cpu);
    BootSoundCheckpoint(memory, cpu, 0x809c60u, stepped);
    OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_FLAG_PORT));
    BootSoundCheckpoint(memory, cpu, 0x809c63u, stepped);
    OpAdcValue(cpu, 0x7fu);
    BootSoundCheckpoint(memory, cpu, 0x809c65u, stepped);
    LoadA8(cpu, Pull8(memory, cpu));
    BootSoundCheckpoint(memory, cpu, 0x809c66u, stepped);
    OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_DATA_PORT));
}

static uint8_t BeginBootSoundBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PrepareBootSoundBlock(memory, cpu, false);
    return BootSoundAcknowledged(memory, cpu);
}

static void FinishBootSoundPair(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                bool stepped) {
    BootSoundCheckpoint(memory, cpu, 0x809c3bu, stepped);
    OpSepWidths(cpu, 0x20u);
    BootSoundCheckpoint(memory, cpu, 0x809c3du, stepped);
    OpDex(cpu);
    BootSoundCheckpoint(memory, cpu, 0x809c3eu, stepped);
}

static void WriteBootSoundPair(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               bool stepped) {
    BootSoundCheckpoint(memory, cpu, 0x809c36u, stepped);
    OpRepWidths(cpu, 0x20u);
    BootSoundCheckpoint(memory, cpu, 0x809c38u, stepped);
    OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_DATA_PORT));
    FinishBootSoundPair(memory, cpu, stepped);
}

static uint8_t SendBootSoundBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  Lufia2ExecutionResult *result) {
    ReadBootSoundSource(memory, cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    do {
        WriteBootSoundPair(memory, cpu, false);
        if (cpu->zero)
            break;
        ExchangeAccumulatorBytes(cpu);
        ReadBootSoundSource(memory, cpu, 0u);
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

static void PrepareBootSoundTransfer(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLdy(cpu, 0u);
    OpLoadA(cpu, 0xbbaau);
}

Lufia2ExecutionResult Lufia2BeginBootSoundTransfer(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809c0fu);
    PrepareBootSoundTransfer(memory, cpu);
    return ExecutionHandoff(cpu, 0x809c18u);
}

Lufia2ExecutionResult Lufia2UploadBootSoundBlocks(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809c0fu);
    PrepareBootSoundTransfer(memory, cpu);
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

static Lufia2ExecutionResult BootSoundWritePairStep(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    uint32_t entry) {
    if (entry == 0x809c35u) {
        EmitExecutionCheckpoint(memory, cpu, 0x809c35u);
        OpIncA(cpu);
    }
    if (entry == 0x809c3bu)
        FinishBootSoundPair(memory, cpu, true);
    else
        WriteBootSoundPair(memory, cpu, true);
    return ExecutionHandoff(cpu, cpu->zero ? 0x809c40u : 0x809c2bu);
}

Lufia2ExecutionResult Lufia2BootSoundTransferStep(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->index_is_8_bit || cpu->decimal ||
        cpu->accumulator_is_8_bit != (entry != 0x809c3bu))
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x809c23u:
        ReadBootSoundSource(memory, cpu, 0x809c23u);
        EmitExecutionCheckpoint(memory, cpu, 0x809c26u);
        ExchangeAccumulatorBytes(cpu);
        EmitExecutionCheckpoint(memory, cpu, 0x809c27u);
        OpLoadA(cpu, 0u);
        EmitExecutionCheckpoint(memory, cpu, 0x809c29u);
        return ExecutionHandoff(cpu, 0x809c36u);
    case 0x809c2bu:
        EmitExecutionCheckpoint(memory, cpu, 0x809c2bu);
        ExchangeAccumulatorBytes(cpu);
        ReadBootSoundSource(memory, cpu, 0x809c2cu);
        EmitExecutionCheckpoint(memory, cpu, 0x809c2fu);
        ExchangeAccumulatorBytes(cpu);
        return ExecutionHandoff(cpu, 0x809c30u);
    case 0x809c35u:
    case 0x809c36u:
    case 0x809c3bu:
        return BootSoundWritePairStep(memory, cpu, entry);
    case 0x809c49u:
        PrepareBootSoundBlock(memory, cpu, true);
        return ExecutionHandoff(cpu, 0x809c69u);
    default:
        return ExecutionHandoff(cpu, entry);
    }
}

Lufia2ExecutionResult Lufia2BootSoundOpcodeStep(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu, uint32_t entry) {
    const bool wide = entry == 0x809c38u || entry == 0x809c3bu ||
                      (entry >= 0x809c4cu && entry <= 0x809c58u);
    if (cpu->program_bank != 0x80u || cpu->index_is_8_bit || cpu->decimal ||
        cpu->accumulator_is_8_bit != !wide)
        return ExecutionHandoff(cpu, entry);
    uint32_t next = entry + 1u;
    switch (entry) {
    case 0x809c23u:
    case 0x809c2cu:
    case 0x809c4cu:
    case 0x809c51u:
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, BOOT_SOUND_SOURCE));
        next = entry + 2u;
        break;
    case 0x809c25u:
    case 0x809c2eu:
    case 0x809c4eu:
    case 0x809c4fu:
    case 0x809c53u:
    case 0x809c54u:
        OpIny(cpu);
        break;
    case 0x809c26u:
    case 0x809c2bu:
    case 0x809c2fu:
        ExchangeAccumulatorBytes(cpu);
        break;
    case 0x809c27u:
    case 0x809c5du:
        OpLoadA(cpu, 0u);
        next = entry + 2u;
        break;
    case 0x809c29u:
        next = 0x809c36u;
        break;
    case 0x809c35u:
        OpIncA(cpu);
        break;
    case 0x809c36u:
    case 0x809c4au:
        OpRepWidths(cpu, 0x20u);
        next = entry + 2u;
        break;
    case 0x809c38u:
    case 0x809c66u:
        OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_DATA_PORT));
        next = entry + 3u;
        break;
    case 0x809c3bu:
    case 0x809c58u:
        OpSepWidths(cpu, 0x20u);
        next = entry + 2u;
        break;
    case 0x809c3du:
        OpDex(cpu);
        break;
    case 0x809c3eu:
        next = cpu->zero ? 0x809c40u : 0x809c2bu;
        break;
    case 0x809c49u:
        PushAccumulator8(memory, cpu);
        break;
    case 0x809c50u:
        OpTax(cpu);
        break;
    case 0x809c55u:
        OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_ADDRESS_PORT));
        next = entry + 3u;
        break;
    case 0x809c5au:
        OpCpx(cpu, 1u);
        next = entry + 3u;
        break;
    case 0x809c5fu:
        RolA8(cpu);
        break;
    case 0x809c60u:
        OpSta(memory, cpu, OpAbs(cpu, BOOT_SOUND_FLAG_PORT));
        next = entry + 3u;
        break;
    case 0x809c63u:
        OpAdcValue(cpu, 0x7fu);
        next = entry + 2u;
        break;
    case 0x809c65u:
        LoadA8(cpu, Pull8(memory, cpu));
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    return ExecutionHandoff(cpu, next);
}
