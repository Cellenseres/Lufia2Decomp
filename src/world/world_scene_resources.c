#include "core/child_call.h"
#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "system/wram.h"
#include "world/world_scene_resources_internal.h"

enum {
    SCENE_GROUP_ANCHOR = WRAM_WORLD_MAP_OBJECT_TABLE + 6u * 0x1du,
    SCENE_GROUP_SPACING = WRAM_FIELD_STREAMED_ROW_VRAM,
    SCENE_CONTROL = WRAM_FIELD_STREAMED_ROW_SOURCE + 2u,
    SCENE_PARAMETER = WRAM_FIELD_LAYER_SCROLL_Y + 2u,
    SCENE_PALETTE_INPUT = WRAM_CGRAM_BUFFER + 0x20u,
    SCENE_PALETTE_WORK = 0xdc20u,
    SCENE_PALETTE_BASE = 0xdc00u,
    SCENE_PALETTE_BYTES = 0xe0u,
    SCENE_PALETTE_DARKEN_COLOR = 0x14a5u,
    PALETTE_SUBTRACT_COLOR = 0x15u,
    PALETTE_COLORS_LEFT = 0x26u,
    RESOURCE_INDEX = 0x54u,
    RESOURCE_LENGTH = 0x58u,
    RESOURCE_DESTINATION = 0x60u,
    RESOURCE_DESTINATION_BANK = 0x62u,
    SCENE_GRAPHICS_RESOURCE = 0x293u,
    SCENE_GRAPHICS_BUFFER = 0xe000u,
    SCENE_GRAPHICS_VRAM = 0x4000u,
    SCENE_DMA_CHANNEL = 7u
};

typedef struct WorldSceneResources {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} WorldSceneResources;

static bool ResourceSceneCall(WorldSceneResources *scene, uint32_t site,
                              uint32_t target, uint8_t frame_size) {
    if (!CallChildWithFrame(scene->memory, scene->cpu, scene->child, scene->context,
                            site, target, frame_size, 0x86u)) {
        scene->result = ExecutionReturned(site);
        scene->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceEntry(scene->cpu)) {
        scene->result = ExecutionHandoff(scene->cpu, site + frame_size + 1u);
        return false;
    }
    return true;
}

static bool PrepareActorScene(WorldSceneResources *scene, uint32_t entry,
                              uint16_t track, uint16_t records) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    OpLdx(cpu, track);
    OpLdy(cpu, records);
    if (!ResourceSceneCall(scene, entry + 6u, 0x869470u, 2u) ||
        !ResourceSceneCall(scene, entry + 9u, 0x8692a1u, 2u) ||
        !ResourceSceneCall(scene, entry + 12u, 0x86c284u, 2u))
        return false;
    OpLdx(cpu, 0x748u);
    OpWrite16(memory, OpAbs(cpu, SCENE_GROUP_ANCHOR + 2u), cpu->x);
    OpLdy(cpu, 0xd28u);
    OpWrite16(memory, OpAbs(cpu, SCENE_GROUP_ANCHOR + 5u), cpu->y);
    OpLdy(cpu, 0x20u);
    OpWrite16(memory, OpAbs(cpu, SCENE_GROUP_SPACING), cpu->y);
    return ResourceSceneCall(scene, entry + 33u, 0x86c2b3u, 2u);
}

Lufia2ExecutionResult Lufia2WorldMapRunGroupScene0(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86afb2u);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86afdcu)};
    if (!PrepareActorScene(&scene, 0x86afb2u, 0xdf1du, 0xdf0fu))
        return scene.result;
    OpLdx(cpu, 0xafddu);
    if (!ResourceSceneCall(&scene, 0x86afd9u, 0x86c1d2u, 2u))
        return scene.result;
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapRunGroupScene1(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86afe3u);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86b01fu)};
    if (!PrepareActorScene(&scene, 0x86afe3u, 0xdf4fu, 0xdf41u) ||
        !ResourceSceneCall(&scene, 0x86b007u, 0x86c582u, 2u))
        return scene.result;
    OpLoadA(cpu, 0x71u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x0cu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PARAMETER));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpLdx(cpu, 0xb020u);
    if (!ResourceSceneCall(&scene, 0x86b01cu, 0x86c1d2u, 2u))
        return scene.result;
    return scene.result;
}

static Lufia2ExecutionResult RunResourceScene(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              void *context, uint32_t entry,
                                              uint16_t records, uint16_t script) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, entry);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86bf98u)};
    OpLdx(cpu, script);
    PushIndex(memory, cpu);
    OpLdx(cpu, 0xe04fu);
    OpLdy(cpu, records);
    static const uint32_t children[] = {0x869470u, 0x8692a1u, 0x86bb8cu,
                                        0x86bf99u, 0x86e84fu, 0x86c582u};
    for (unsigned phase = 0u; phase < 6u; ++phase)
        if (!ResourceSceneCall(&scene, 0x86bf7au + phase * 3u, children[phase], 2u))
            return scene.result;
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_SKYLINE_BAND_COUNT));
    OpPullX(memory, cpu);
    if (!ResourceSceneCall(&scene, 0x86bf95u, 0x86c1d2u, 2u))
        return scene.result;
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene0(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    return RunResourceScene(memory, cpu, child, context, 0x86bf70u, 0xe041u, 0xbfdeu);
}

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene1(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    return RunResourceScene(memory, cpu, child, context, 0x86bff5u, 0xe05du, 0xbfdeu);
}

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene2(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    return RunResourceScene(memory, cpu, child, context, 0x86c002u, 0xe06bu, 0xbfdeu);
}

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene3(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    return RunResourceScene(memory, cpu, child, context, 0x86c00fu, 0xe079u, 0xc01cu);
}

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene4(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    return RunResourceScene(memory, cpu, child, context, 0x86c041u, 0xe087u, 0xbfdeu);
}

Lufia2ExecutionResult Lufia2WorldMapInstallScenePalette(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bb8cu);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86bbc3u)};
    OpLdx(cpu, SCENE_PALETTE_DARKEN_COLOR);
    OpWrite16(memory, OpDp(cpu, PALETTE_SUBTRACT_COLOR), cpu->x);
    OpLdx(cpu, SCENE_PALETTE_INPUT);
    OpLoadA(cpu, 0x70u);
    OpSta(memory, cpu, OpDp(cpu, PALETTE_COLORS_LEFT));
    if (!ResourceSceneCall(&scene, 0x86bb98u, 0x86966cu, 2u))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, SCENE_PALETTE_WORK);
    OpLdy(cpu, SCENE_PALETTE_INPUT);
    OpLoadA(cpu, SCENE_PALETTE_BYTES - 1u);
    PushDataBank(memory, cpu);
    OpMoveNext(memory, cpu, 0x7eu, 0x7fu);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, SCENE_PALETTE_BASE);
    OpWrite16(memory, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_SOURCE), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_BANK));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_MODE));
    OpLdx(cpu, SCENE_PALETTE_BYTES);
    OpWrite16(memory, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_LENGTH), cpu->x);
    return scene.result;
}

static void PrepareSceneResource(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint16_t resource, uint16_t destination) {
    OpLdx(cpu, resource);
    OpWrite16(memory, OpDp(cpu, RESOURCE_INDEX), cpu->x);
    OpLdx(cpu, destination);
    OpWrite16(memory, OpDp(cpu, RESOURCE_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION_BANK));
}

static void UploadSceneTiles(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, SCENE_GRAPHICS_VRAM);
    OpWrite16(memory, OpAbs(cpu, SNES_VMADDL), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(SCENE_DMA_CHANNEL)));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(SCENE_DMA_CHANNEL)));
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, RESOURCE_DESTINATION)));
    OpWrite16(memory, OpAbs(cpu, SNES_A1TL(SCENE_DMA_CHANNEL)), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION_BANK));
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(SCENE_DMA_CHANNEL)));
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, RESOURCE_LENGTH)));
    OpWrite16(memory, OpAbs(cpu, SNES_DASL(SCENE_DMA_CHANNEL)), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
}

Lufia2ExecutionResult Lufia2WorldMapUploadSceneGraphics(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bf99u);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86bfddu)};
    if (!ResourceSceneCall(&scene, 0x86bf99u, 0x86d3a5u, 2u))
        return scene.result;
    OpStz(memory, cpu, OpAbs(cpu, SNES_NMITIMEN));
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    PrepareSceneResource(memory, cpu, SCENE_GRAPHICS_RESOURCE, SCENE_GRAPHICS_BUFFER);
    if (!ResourceSceneCall(&scene, 0x86bfb0u, 0x808e9du, 3u))
        return scene.result;
    UploadSceneTiles(memory, cpu);
    return scene.result;
}

static Lufia2ExecutionResult
LoadSceneResource(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                  Lufia2PushedChildCall child, void *context, uint32_t entry,
                  uint16_t resource, uint16_t destination) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, entry);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(entry + 18u)};
    OpLdy(cpu, resource);
    OpWrite16(memory, OpDp(cpu, RESOURCE_INDEX), cpu->y);
    OpLdx(cpu, destination);
    OpWrite16(memory, OpDp(cpu, RESOURCE_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION_BANK));
    if (!ResourceSceneCall(&scene, entry + 14u, 0x808e9du, 3u))
        return scene.result;
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapLoadSceneBuffer(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    return LoadSceneResource(memory, cpu, child, context, 0x86bbe6u, 0x291u, 0xe000u);
}

Lufia2ExecutionResult
Lufia2WorldMapLoadAlternateSceneBuffer(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    return LoadSceneResource(memory, cpu, child, context, 0x86c08cu, 0x294u, 0xf000u);
}
