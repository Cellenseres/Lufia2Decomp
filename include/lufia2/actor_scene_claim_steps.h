#ifndef LUFIA2_ACTOR_SCENE_CLAIM_STEPS_H
#define LUFIA2_ACTOR_SCENE_CLAIM_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_ACTOR_SCENE_CLAIM_STEPS_API 1
#define LUFIA2_ACTOR_SCENE_CLAIM_MAX_OPCODES 11u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2ActorClearSceneClaimStep(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
