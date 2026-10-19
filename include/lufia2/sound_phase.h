#ifndef LUFIA2_SOUND_PHASE_H
#define LUFIA2_SOUND_PHASE_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2SoundHeaderStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2SoundReplyStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2SoundCommandStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif

#endif
