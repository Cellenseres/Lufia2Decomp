#ifndef LUFIA2_FIELD_TRIGGER_CONTINUATIONS_H
#define LUFIA2_FIELD_TRIGGER_CONTINUATIONS_H

#include "lufia2/execution.h"

#define LUFIA2_FIELD_TRIGGER_CONTINUATIONS 1

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2FieldContinueTriggerChecks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry,
    Lufia2PushedChildCall child, void *context);

#ifdef __cplusplus
}
#endif
#endif
