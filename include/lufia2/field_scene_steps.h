#ifndef LUFIA2_FIELD_SCENE_STEPS_H
#define LUFIA2_FIELD_SCENE_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_FIELD_SCENE_OBJECT_STEPS 1
#define LUFIA2_SCENE_OBJECT_MAX_OPCODES 18u

#ifdef __cplusplus
extern "C" {
#endif

/* Process one object slot, stopping before its original child calls. */
Lufia2ExecutionResult Lufia2FieldSceneObjectStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif

#endif
