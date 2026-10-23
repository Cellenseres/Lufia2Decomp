#include "core/child_call.h"
#include "lufia2/world_scene_timed_resources.h"
#include "system/wram.h"
#include "world/world_scene_resources_internal.h"

enum {
    SCENE_WINDOW_PARAMETER = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 1u,
    SCENE_MUSIC_FADE_RATE = 0xf0u
};

Lufia2ExecutionResult Lufia2WorldMapIdleSceneScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    (void)memory;
    if (cpu->program_bank != 0x86u)
        return ExecutionHandoff(cpu, 0x86af38u);
    return ExecutionReturned(0x86af38u);
}

Lufia2ExecutionResult Lufia2WorldMapIdleGroupScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    (void)memory;
    if (cpu->program_bank != 0x86u)
        return ExecutionHandoff(cpu, 0x86afe2u);
    return ExecutionReturned(0x86afe2u);
}

static Lufia2ExecutionResult SceneResourceUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static Lufia2ExecutionResult StepResourceSceneTrails(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, entry);
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_WINDOW_PARAMETER), 1);
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_WINDOW_PARAMETER), 1);
    const uint32_t site = entry + 6u;
    if (!CallChildWithFrame(memory, cpu, child, context, site,
                            0x86e71eu, 2u, 0x86u))
        return SceneResourceUnwound(site);
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, site + 3u);
    return ExecutionReturned(site + 3u);
}

Lufia2ExecutionResult Lufia2WorldMapStepResourceSceneTrails(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return StepResourceSceneTrails(memory, cpu, child, context, 0x86bfebu);
}

Lufia2ExecutionResult Lufia2WorldMapStepFadingResourceSceneTrails(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return StepResourceSceneTrails(memory, cpu, child, context, 0x86c02cu);
}

Lufia2ExecutionResult Lufia2WorldMapBeginResourceSceneMusicFade(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c036u);
    OpLoadA(cpu, SCENE_MUSIC_FADE_RATE);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x86c038u,
                            0x8096ccu, 3u, 0x86u))
        return SceneResourceUnwound(0x86c038u);
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c03cu);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x86c03cu,
                            0x809692u, 3u, 0x86u))
        return SceneResourceUnwound(0x86c03cu);
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c040u);
    return ExecutionReturned(0x86c040u);
}
