#ifndef LUFIA2_FIELD_PHASE_H
#define LUFIA2_FIELD_PHASE_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2FieldFrameStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2FieldActorInstallStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif

#endif
