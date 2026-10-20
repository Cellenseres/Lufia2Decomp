#ifndef LUFIA2_SCENE_REBUILD_H
#define LUFIA2_SCENE_REBUILD_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Lufia2SceneRebuildPhase {
    LUFIA2_SCENE_REBUILD_FIND_SPRITE,
    LUFIA2_SCENE_REBUILD_RESET_FLAGS,
    LUFIA2_SCENE_REBUILD_PARTY_OFFSETS
} Lufia2SceneRebuildPhase;

/* Run one finite phase between original child calls. */
Lufia2ExecutionResult Lufia2ActorRebuildScenePhase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2SceneRebuildPhase phase, unsigned max_slots);

#ifdef __cplusplus
}
#endif

#endif
