#ifndef LUFIA2_ACTOR_SCENE_PRESENTATION_STEPS_H
#define LUFIA2_ACTOR_SCENE_PRESENTATION_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_ACTOR_SCENE_PRESENTATION_STEPS_API 1
#define LUFIA2_ACTOR_SCENE_PRESENTATION_MAX_OPCODES 18u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2ActorScenePresentationStep(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
