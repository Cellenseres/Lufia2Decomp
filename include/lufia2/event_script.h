#ifndef LUFIA2_EVENT_SCRIPT_H
#define LUFIA2_EVENT_SCRIPT_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Runs the current field event until yielding or handing off. */
Lufia2ExecutionResult Lufia2FieldRunEventScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#ifdef __cplusplus
}
#endif

#endif
