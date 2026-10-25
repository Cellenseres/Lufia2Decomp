#ifndef LUFIA2_TEXT_SCENE_H
#define LUFIA2_TEXT_SCENE_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LUFIA2_TEXT_SCENE_READER_CALLS 1

/* Child services include original C0B7 operand reads. */
Lufia2ExecutionResult Lufia2TextRunSceneOperation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#ifdef __cplusplus
}
#endif

#endif
