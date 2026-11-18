#ifndef LUFIA2_SCREEN_FADE_STEPS_H
#define LUFIA2_SCREEN_FADE_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_SCREEN_FADE_STEPS_API 1
#define LUFIA2_SCREEN_FADE_STEPS_MAX_OPCODES 12u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2ScreenFadeStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
