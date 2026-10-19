#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "lufia2/sound_phase.h"

enum {
    SOUND_REPLY_PORT = 0x002142u,
    SOUND_REPLY_BANK_PORT = 0x002143u,
    SOUND_REPLY_POLL_LIMIT = 65536u
};

Lufia2ExecutionResult Lufia2SoundReplyStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, entry);
    switch (entry) {
    case 0x809a0au:
        Push8(memory, cpu, PackStatus(cpu));
        break;
    case 0x809a0bu:
        UnpackStatus(cpu, (uint8_t)(PackStatus(cpu) | 0x20u));
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x809a0du:
    case 0x809a15u:
        if (cpu->accumulator_is_8_bit != 1u)
            return ExecutionHandoff(cpu, entry);
        OpLoadA(cpu, entry == 0x809a0du ? 0xcdu : 0xefu);
        return ExecutionHandoff(cpu, entry + 2u);
    case 0x809a0fu:
    case 0x809a17u:
    case 0x809a26u:
    case 0x809a2cu:
        return Lufia2SoundPollStep(memory, cpu, entry);
    case 0x809a13u:
    case 0x809a1bu:
    case 0x809a2au:
    case 0x809a30u:
        return Lufia2SoundPollBranchStep(cpu, entry);
    case 0x809a1du:
        TransferDirectToA(cpu);
        break;
    case 0x809a1eu:
    case 0x809a22u:
        if (cpu->accumulator_is_8_bit != 1u)
            return ExecutionHandoff(cpu, entry);
        OpSta(memory, cpu, entry == 0x809a1eu
            ? SOUND_REPLY_PORT : SOUND_REPLY_BANK_PORT);
        return ExecutionHandoff(cpu, entry + 4u);
    case 0x809a32u:
        UnpackStatus(cpu, Pull8(memory, cpu));
        break;
    case 0x809a33u: {
        const uint16_t low = Pull8(memory, cpu);
        const uint16_t address = (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
        return ExecutionHandoff(cpu, 0x800000u | (uint16_t)(address + 1u));
    }
    default:
        return ExecutionHandoff(cpu, entry);
    }
    return ExecutionHandoff(cpu, entry + 1u);
}

static bool SoundReplyMatches(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t port) {
    for (uint32_t polls = 0u; polls < SOUND_REPLY_POLL_LIMIT; ++polls) {
        OpCmp(memory, cpu, port);
        if (cpu->zero)
            return true;
    }
    return false;
}

static bool SoundReplyCleared(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t port) {
    for (uint32_t polls = 0u; polls < SOUND_REPLY_POLL_LIMIT; ++polls) {
        OpLda(memory, cpu, port);
        if (cpu->zero)
            return true;
    }
    return false;
}

Lufia2ExecutionResult Lufia2WaitSoundDriverReply(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809a0au);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    OpLoadA(cpu, 0xcdu);
    if (!SoundReplyMatches(memory, cpu, SOUND_REPLY_PORT))
        return ExecutionHandoff(cpu, 0x809a0fu);
    OpLoadA(cpu, 0xefu);
    if (!SoundReplyMatches(memory, cpu, SOUND_REPLY_BANK_PORT))
        return ExecutionHandoff(cpu, 0x809a17u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, SOUND_REPLY_PORT);
    OpSta(memory, cpu, SOUND_REPLY_BANK_PORT);
    if (!SoundReplyCleared(memory, cpu, SOUND_REPLY_PORT))
        return ExecutionHandoff(cpu, 0x809a26u);
    if (!SoundReplyCleared(memory, cpu, SOUND_REPLY_BANK_PORT))
        return ExecutionHandoff(cpu, 0x809a2cu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x809a33u);
}
