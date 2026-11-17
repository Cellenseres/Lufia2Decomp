#ifndef LUFIA2_FIELD_CONTROL_STEPS_H
#define LUFIA2_FIELD_CONTROL_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_FIELD_CONTROL_STEPS 1

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2FieldControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
