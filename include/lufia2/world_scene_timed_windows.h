#ifndef LUFIA2_WORLD_SCENE_TIMED_WINDOWS_H
#define LUFIA2_WORLD_SCENE_TIMED_WINDOWS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2WorldMapReduceSceneSparkEmission(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapBeginSceneColorRise(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapBeginSceneColorFall(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadSceneSlopedWindow0(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadSceneSlopedWindow1(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadSceneSlopedWindow2(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadSceneMirroredWindows0(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadSceneMirroredWindows1(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadSceneMirroredWindows2(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapSubmitSceneSlopedWindow0(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapSubmitSceneSlopedWindow1(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapSubmitSceneSlopedWindow2(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapStartSceneWindowCollapse(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapInstallSceneWindowPalette(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapPrepareSceneWindowCollapse(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context);

#ifdef __cplusplus
}
#endif

#endif
