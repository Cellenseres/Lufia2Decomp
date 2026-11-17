#ifndef LUFIA2_ACTOR_SCENE_RESET_STEPS_H
#define LUFIA2_ACTOR_SCENE_RESET_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_SCENE_SLOT_RESET_STEPS 1
#define LUFIA2_SCENE_RESET_STATUS_OPCODES 7u
#define LUFIA2_SCENE_RESET_RUNTIME_OPCODES 9u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2ActorResetSceneSlotStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
