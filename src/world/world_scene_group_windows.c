#include "core/child_call.h"
#include "lufia2/world_scene_group_windows.h"
#include "system/wram.h"
#include "world/world_scene_internal.h"
#include "world/world_scene_resources_internal.h"

enum {
    ROM_GROUP_SCENE_WINDOW_SHAPES = 0xb106u,
    GROUP_SCENE_WINDOW_SHAPE_BYTES = 14u,
    GROUP_SCENE_WINDOW_SOUND = 0x8eu,
    GROUP_SCENE_WINDOW_FINAL_SOUND = 0x2bu
};

static bool GroupSceneWindowChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  Lufia2PushedChildCall child, void *context,
                                  uint32_t site, uint32_t target,
                                  Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, 2u, 0x86u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceEntry(cpu)) {
        *result = ExecutionHandoff(cpu, site + 3u);
        return false;
    }
    return true;
}

static Lufia2ExecutionResult LoadGroupSceneWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry, uint8_t shape) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, entry);
    Lufia2ExecutionResult result = ExecutionReturned(0x86b0c7u);
    OpLdy(cpu, (uint16_t)(ROM_GROUP_SCENE_WINDOW_SHAPES +
                          shape * GROUP_SCENE_WINDOW_SHAPE_BYTES));
    OpLoadA(cpu, shape);
    if (!GroupSceneWindowChild(memory, cpu, child, context, 0x86b0bfu, 0x86c54bu,
                               &result))
        return result;
    OpLoadA(cpu, GROUP_SCENE_WINDOW_SOUND);
    GroupSceneWindowChild(memory, cpu, child, context, 0x86b0c4u, 0x86d378u, &result);
    return result;
}

Lufia2ExecutionResult Lufia2WorldMapLoadGroupSceneWindow0(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return LoadGroupSceneWindow(memory, cpu, child, context, 0x86b0bau, 0u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadGroupSceneWindow1(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return LoadGroupSceneWindow(memory, cpu, child, context, 0x86b0c8u, 1u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadGroupSceneWindow2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return LoadGroupSceneWindow(memory, cpu, child, context, 0x86b0cfu, 2u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadGroupSceneWindow3(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return LoadGroupSceneWindow(memory, cpu, child, context, 0x86b0d6u, 3u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadGroupSceneWindow4(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return LoadGroupSceneWindow(memory, cpu, child, context, 0x86b0ddu, 4u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadGroupSceneWindow5(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return LoadGroupSceneWindow(memory, cpu, child, context, 0x86b0e4u, 5u);
}

Lufia2ExecutionResult Lufia2WorldMapSetGroupSceneWindowShape(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b0ebu);
    Lufia2ExecutionResult result = ExecutionReturned(0x86b105u);
    OpLdx(cpu, WRAM_WORLD_SCENE_WINDOW_RECORD_STORAGE);
    OpLdy(cpu, 0xacu);
    OpWrite16(memory, WorldSceneObjectField(cpu, 0u), cpu->y);
    OpLdy(cpu, 0xb2u);
    OpWrite16(memory, WorldSceneObjectField(cpu, 1u), cpu->y);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 2u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 3u));
    OpLoadA(cpu, GROUP_SCENE_WINDOW_FINAL_SOUND);
    GroupSceneWindowChild(memory, cpu, child, context, 0x86b102u, 0x86d378u, &result);
    return result;
}
