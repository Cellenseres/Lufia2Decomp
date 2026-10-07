#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    SOUND_REPLY_PORT = 0x002142u,
    SOUND_REPLY_BANK_PORT = 0x002143u,
    SOUND_REPLY_POLL_LIMIT = 65536u
};

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
