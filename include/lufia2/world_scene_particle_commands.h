#ifndef LUFIA2_WORLD_SCENE_PARTICLE_COMMANDS_H
#define LUFIA2_WORLD_SCENE_PARTICLE_COMMANDS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2WorldMapSpawnTrailingSceneParticle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapFindOrbitSceneParticleSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
