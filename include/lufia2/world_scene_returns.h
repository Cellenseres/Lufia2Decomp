#ifndef LUFIA2_WORLD_SCENE_RETURNS_H
#define LUFIA2_WORLD_SCENE_RETURNS_H

#include "lufia2/execution.h"

#define LUFIA2_WORLD_FRAME_LOOP_API 1

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2WorldMapModeZeroInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapModeNonzeroInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapIdleSceneObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapContinueFrameTail(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry,
    Lufia2PushedChildCall child, void *context);

/* Continue with the existing guest stack. */
Lufia2ExecutionResult Lufia2WorldMapContinueSession(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapCheckFrameMovement(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapCheckFramePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapCheckFramePlaneState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
