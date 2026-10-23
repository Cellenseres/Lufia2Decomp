#include "lufia2/world_scene_timed_windows.h"
#include "core/child_call.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "world/world_scene_resources_internal.h"

enum {
    SCENE_COLOR_DIRECTION = WRAM_FIELD_LAYER_SCROLL_Y + 4u,
    SCENE_COLOR_INTENSITY = WRAM_FIELD_LAYER_SCROLL_Y + 5u,
    SCENE_SPARK_EMISSION = WRAM_FIELD_LAYER_SCROLL_Y + 6u,
    SCENE_WINDOW_INDEX = 0x63u,
    SCENE_WINDOW_LAYER = 0u,
    SCENE_WINDOW_ORIENTATION = 4u
};

typedef struct TimedScene {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} TimedScene;

static bool CallTimedSceneChild(TimedScene *scene, uint32_t site, uint32_t target) {
    if (!CallChildWithFrame(scene->memory, scene->cpu, scene->child, scene->context,
                            site, target, 2u, 0x86u)) {
        scene->result = ExecutionReturned(site);
        scene->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceEntry(scene->cpu)) {
        scene->result = ExecutionHandoff(scene->cpu, site + 3u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2WorldMapReduceSceneSparkEmission(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b24cu);
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_SPARK_EMISSION), -1);
    return ExecutionReturned(0x86b24fu);
}

Lufia2ExecutionResult Lufia2WorldMapBeginSceneColorRise(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b250u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_COLOR_DIRECTION));
    OpStz(memory, cpu, OpAbs(cpu, SCENE_COLOR_INTENSITY));
    return ExecutionReturned(0x86b258u);
}

Lufia2ExecutionResult Lufia2WorldMapBeginSceneColorFall(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b259u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_COLOR_DIRECTION));
    OpLoadA(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_COLOR_INTENSITY));
    return ExecutionReturned(0x86b263u);
}

static Lufia2ExecutionResult LoadTimedSlopedWindow(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint16_t record) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, entry);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86b271u)};
    OpLdy(cpu, record);
    OpLoadA(cpu, 0u);
    if (!CallTimedSceneChild(&scene, 0x86b269u, 0x86c54bu))
        return scene.result;
    OpLoadA(cpu, 0x74u);
    CallTimedSceneChild(&scene, 0x86b26eu, 0x86d378u);
    return scene.result;
}

static Lufia2ExecutionResult LoadMirroredSceneWindows(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint16_t record) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, entry);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86b291u)};
    OpLdy(cpu, record);
    OpLoadA(cpu, 0u);
    PushY(memory, cpu);
    if (!CallTimedSceneChild(&scene, 0x86b282u, 0x86c54bu))
        return scene.result;
    OpLdy(cpu, PullIndexValue(memory, cpu));
    OpLoadA(cpu, 1u);
    if (!CallTimedSceneChild(&scene, 0x86b288u, 0x86c54bu))
        return scene.result;
    OpLda(memory, cpu, DirectIndexedAddress(cpu, SCENE_WINDOW_ORIENTATION, cpu->x));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0x80u));
    OpSta(memory, cpu, DirectIndexedAddress(cpu, SCENE_WINDOW_ORIENTATION, cpu->x));
    return scene.result;
}

static Lufia2ExecutionResult SubmitTimedSlopedWindow(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint16_t record) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, entry);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86b2b0u)};
    OpLdy(cpu, record);
    OpLoadA(cpu, 1u);
    if (!CallTimedSceneChild(&scene, 0x86b2a1u, 0x86c54bu))
        return scene.result;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, SCENE_WINDOW_LAYER));
    if (!CallTimedSceneChild(&scene, 0x86b2a8u, 0x86c4e0u))
        return scene.result;
    OpLoadA(cpu, 0xccu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapLoadSceneSlopedWindow0(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return LoadTimedSlopedWindow(memory, cpu, child, context, 0x86b264u, 0xb3c6u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadSceneSlopedWindow1(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return LoadTimedSlopedWindow(memory, cpu, child, context, 0x86b272u, 0xb3d4u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadSceneSlopedWindow2(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return LoadTimedSlopedWindow(memory, cpu, child, context, 0x86b277u, 0xb3e2u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadSceneMirroredWindows0(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return LoadMirroredSceneWindows(memory, cpu, child, context, 0x86b27cu, 0xb3f0u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadSceneMirroredWindows1(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return LoadMirroredSceneWindows(memory, cpu, child, context, 0x86b292u, 0xb3feu);
}

Lufia2ExecutionResult Lufia2WorldMapLoadSceneMirroredWindows2(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return LoadMirroredSceneWindows(memory, cpu, child, context, 0x86b297u, 0xb40cu);
}

Lufia2ExecutionResult Lufia2WorldMapSubmitSceneSlopedWindow0(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return SubmitTimedSlopedWindow(memory, cpu, child, context, 0x86b29cu, 0xb39cu);
}

Lufia2ExecutionResult Lufia2WorldMapSubmitSceneSlopedWindow1(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return SubmitTimedSlopedWindow(memory, cpu, child, context, 0x86b2b1u, 0xb3aau);
}

Lufia2ExecutionResult Lufia2WorldMapSubmitSceneSlopedWindow2(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return SubmitTimedSlopedWindow(memory, cpu, child, context, 0x86b2b6u, 0xb3b8u);
}

Lufia2ExecutionResult Lufia2WorldMapStartSceneWindowCollapse(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b2bbu);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86b2e6u)};
    static const uint16_t records[] = {0xb41au, 0xb428u, 0xb436u,
                                      0xb39cu, 0xb3aau, 0xb3b8u};
    OpStz(memory, cpu, OpDp(cpu, SCENE_WINDOW_INDEX));
    for (unsigned window = 0; window < 6u; ++window) {
        OpLdy(cpu, records[window]);
        if (!CallTimedSceneChild(&scene, 0x86b2c0u + window * 6u, 0x86b2e7u))
            return scene.result;
    }
    OpLoadA(cpu, 0x73u);
    CallTimedSceneChild(&scene, 0x86b2e3u, 0x86d378u);
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapInstallSceneWindowPalette(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b2f7u);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86b34fu)};
    if (!CallTimedSceneChild(&scene, 0x86b2f7u, 0x86c582u))
        return scene.result;
    OpLdy(cpu, 0xb444u);
    OpLoadA(cpu, 0u);
    if (!CallTimedSceneChild(&scene, 0x86b2ffu, 0x86c336u))
        return scene.result;
    OpLoadA(cpu, 0u);
    OpStz(memory, cpu, OpDp(cpu, SCENE_WINDOW_LAYER));
    if (!CallTimedSceneChild(&scene, 0x86b306u, 0x86c2d9u))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0xb420u);
    OpLdy(cpu, 0x0340u);
    OpLoadA(cpu, 0xdfu);
    PushDataBank(memory, cpu);
    OpMoveNext(memory, cpu, 0x7eu, 0xa6u);
    OpLdx(cpu, 0xeb20u);
    OpLdy(cpu, 0x0420u);
    OpLoadA(cpu, 0xffu);
    OpMoveNext(memory, cpu, 0x7eu, 0xa6u);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x0320u);
    OpWrite16(memory, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_SOURCE), cpu->x);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_BANK));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_MODE));
    OpLdx(cpu, 0x01e0u);
    OpWrite16(memory, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_LENGTH), cpu->x);
    if (!CallTimedSceneChild(&scene, 0x86b33du, 0x86974fu))
        return scene.result;
    OpLoadA(cpu, 0xb1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 0xeau);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x2cu);
    CallTimedSceneChild(&scene, 0x86b34cu, 0x86d378u);
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapPrepareSceneWindowCollapse(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b2e7u);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86b2f6u)};
    OpLda(memory, cpu, OpDp(cpu, SCENE_WINDOW_INDEX));
    if (!CallTimedSceneChild(&scene, 0x86b2e9u, 0x86c54bu))
        return scene.result;
    OpLda(memory, cpu, DirectIndexedAddress(cpu, 13u, cpu->x));
    OpSta(memory, cpu, DirectIndexedAddress(cpu, 10u, cpu->x));
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, DirectIndexedAddress(cpu, 12u, cpu->x));
    OpStepMem(memory, cpu, OpDp(cpu, SCENE_WINDOW_INDEX), 1);
    return scene.result;
}
