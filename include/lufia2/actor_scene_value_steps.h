#ifndef LUFIA2_ACTOR_SCENE_VALUE_STEPS_H
#define LUFIA2_ACTOR_SCENE_VALUE_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_ACTOR_SCENE_VALUE_STEPS_API 1
#define LUFIA2_ACTOR_SCENE_VALUE_MAX_OPCODES 20u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2ActorSceneValueStep(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu, uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
