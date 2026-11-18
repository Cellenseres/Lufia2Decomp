#ifndef LUFIA2_SAVE_ACTION_STEPS_H
#define LUFIA2_SAVE_ACTION_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_SAVE_ACTION_STEPS 1
#define LUFIA2_SAVE_ACTION_MAX_OPCODES 11u

#ifdef __cplusplus
extern "C" {
#endif

/* Finish an input or cancellation phase before its original child call. */
Lufia2ExecutionResult Lufia2SaveActionStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif

#endif
