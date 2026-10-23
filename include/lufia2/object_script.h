#ifndef LUFIA2_OBJECT_SCRIPT_H
#define LUFIA2_OBJECT_SCRIPT_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $83:E0FC loads and runs the object's script. */
Lufia2ExecutionResult Lufia2ObjectRunScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $83:E10F continues with the saved data-bank frame. */
Lufia2ExecutionResult Lufia2ObjectContinueScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
