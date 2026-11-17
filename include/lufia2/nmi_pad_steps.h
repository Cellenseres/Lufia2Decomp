#ifndef LUFIA2_NMI_PAD_STEPS_H
#define LUFIA2_NMI_PAD_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_NMI_PAD_STEPS 1
#define LUFIA2_NMI_PAD_STEP_OPCODES 16u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2NmiReadPadStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
