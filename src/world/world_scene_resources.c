#include "core/child_call.h"
#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "system/wram.h"
#include "world/world_scene_resources_internal.h"
#include "world/world_scene_internal.h"

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
    SCENE_DMA_CHANNEL = 7u,
    SCENE_RESOURCE_VARIANT = WRAM_BATTLE_EFFECT_SPECIAL_PARTY,
    SCENE_OBJECT_A = WRAM_WORLD_MAP_OBJECT_TABLE,
    SCENE_OBJECT_B = WRAM_WORLD_MAP_OBJECT_TABLE + 0x1du,
    SCENE_OBJECT_H = WRAM_WORLD_MAP_OBJECT_TABLE + 7u * 0x1du,
    SCENE_OBJECT_V = WRAM_WORLD_MAP_OBJECT_TABLE + 21u * 0x1du,
    SCENE_WINDOW_ORIGIN = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 1u,
    SCENE_WINDOW_WIDTH = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 2u
};

typedef struct WorldSceneResources {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} WorldSceneResources;

static bool ResourceSceneCallMode(WorldSceneResources *scene, uint32_t site,
                                  uint32_t target, uint8_t frame_size, bool byte_mode) {
    if (!CallChildWithFrame(scene->memory, scene->cpu, scene->child, scene->context,
                            site, target, frame_size, 0x86u)) {
        scene->result = ExecutionReturned(site);
        scene->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceMode(scene->cpu, byte_mode)) {
        scene->result = ExecutionHandoff(scene->cpu, site + frame_size + 1u);
        return false;
    }
    return true;
}

static bool ResourceSceneCall(WorldSceneResources *scene, uint32_t site,
                              uint32_t target, uint8_t frame_size) {
    return ResourceSceneCallMode(scene, site, target, frame_size, true);
}

static bool PrepareActorScene(WorldSceneResources *scene, uint32_t entry,
                              uint16_t track, uint16_t records, uint16_t anchor_x,
                              uint16_t anchor_y) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    OpLdx(cpu, track);
    OpLdy(cpu, records);
    if (!ResourceSceneCall(scene, entry + 6u, 0x869470u, 2u) ||
        !ResourceSceneCall(scene, entry + 9u, 0x8692a1u, 2u) ||
        !ResourceSceneCall(scene, entry + 12u, 0x86c284u, 2u))
        return false;
    OpLdx(cpu, anchor_x);
    OpWrite16(memory, OpAbs(cpu, SCENE_GROUP_ANCHOR + 2u), cpu->x);
    OpLdy(cpu, anchor_y);
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
    if (!PrepareActorScene(&scene, 0x86afb2u, 0xdf1du, 0xdf0fu, 0x748u, 0xd28u))
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
    if (!PrepareActorScene(&scene, 0x86afe3u, 0xdf4fu, 0xdf41u, 0x748u, 0xd28u) ||
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

Lufia2ExecutionResult Lufia2WorldMapLoadGroupSceneBuffer(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    return LoadSceneResource(memory, cpu, child, context, 0x86b19au, 0x292u, 0xe000u);
}

static bool PositionGroupFromWorld(WorldSceneResources *scene, uint32_t entry,
                                   uint16_t vertical_offset) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_X_LOW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_GROUP_ANCHOR + 2u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_Y));
    cpu->carry = 0u;
    OpAdcValue(cpu, vertical_offset);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_GROUP_ANCHOR + 5u));
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0x20u);
    OpWrite16(memory, OpAbs(cpu, SCENE_GROUP_SPACING), cpu->y);
    return ResourceSceneCall(scene, entry + 30u, 0x86c2b3u, 2u);
}

Lufia2ExecutionResult
Lufia2WorldMapPositionNearbySceneGroup(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bbc4u);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86bbe5u)};
    PositionGroupFromWorld(&scene, 0x86bbc4u, 8u);
    return scene.result;
}

Lufia2ExecutionResult
Lufia2WorldMapStartSceneObjectAnimation(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86be72u);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86be79u)};
    OpStz(memory, cpu, WorldSceneObjectField(cpu, 0x0fu));
    OpLoadA(cpu, 0x20u);
    ResourceSceneCall(&scene, 0x86be76u, 0x86e0b9u, 2u);
    return scene.result;
}

Lufia2ExecutionResult
Lufia2WorldMapPositionOffsetSceneGroup(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86be7au);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86bec3u)};
    if (!PositionGroupFromWorld(&scene, 0x86be7au, 0x20u))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    static const uint16_t actors[] = {SCENE_OBJECT_A, SCENE_OBJECT_B, SCENE_OBJECT_H};
    static const uint16_t horizontal[] = {0xffe8u, 0xfff8u, 0xffdau};
    static const uint16_t vertical[] = {0xfffau, 0xfffcu, 0xcu};
    for (unsigned actor = 0u; actor < 3u; ++actor) {
        OpLdx(cpu, actors[actor]);
        OpLoadA(cpu, horizontal[actor]);
        OpLdy(cpu, vertical[actor]);
        if (!ResourceSceneCallMode(&scene, 0x86bea6u + actor * 12u, 0x86bec4u, 2u,
                                   false))
            return scene.result;
    }
    OpSepWidths(cpu, 0x20u);
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapRunGroupScene2(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b15au);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86b199u)};
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_RESOURCE_VARIANT));
    if (!ResourceSceneCall(&scene, 0x86b15fu, 0x86b19au, 2u) ||
        !PrepareActorScene(&scene, 0x86b162u, 0xdf71u, 0xdf63u, 0x860u, 0x8b8u) ||
        !ResourceSceneCall(&scene, 0x86b186u, 0x86c582u, 2u))
        return scene.result;
    OpLoadA(cpu, 0xd4u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpLdx(cpu, 0xb1adu);
    ResourceSceneCall(&scene, 0x86b196u, 0x86c1d2u, 2u);
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapRunObjectScene(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b59eu);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86b5d1u)};
    OpLdx(cpu, 0xdf93u);
    OpLdy(cpu, 0xdf85u);
    if (!ResourceSceneCall(&scene, 0x86b5a4u, 0x869470u, 2u) ||
        !ResourceSceneCall(&scene, 0x86b5a7u, 0x8692a1u, 2u))
        return scene.result;
    OpLdx(cpu, SCENE_OBJECT_B);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 0x0fu));
    OpLoadA(cpu, 1u);
    if (!ResourceSceneCall(&scene, 0x86b5b3u, 0x86e0b9u, 2u))
        return scene.result;
    WorldSceneCopyPositionToObject(memory, cpu);
    if (!ResourceSceneCall(&scene, 0x86b5c0u, 0x86c582u, 2u))
        return scene.result;
    OpLoadA(cpu, 0x15u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_SKYLINE_BAND_COUNT));
    OpLdx(cpu, 0xb5d2u);
    ResourceSceneCall(&scene, 0x86b5ceu, 0x86c1d2u, 2u);
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapRunNearbyGroupScene(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bb5cu);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86bb8bu)};
    if (!ResourceSceneCall(&scene, 0x86bb5cu, 0x86bbe6u, 2u) ||
        !ResourceSceneCall(&scene, 0x86bb5fu, 0x86c08cu, 2u))
        return scene.result;
    OpLdx(cpu, 0xe01du);
    OpLdy(cpu, 0xe00fu);
    static const uint32_t children[] = {0x869470u, 0x8692a1u, 0x86bb8cu,
                                        0x86c284u, 0x86bbc4u, 0x86c582u};
    for (unsigned phase = 0u; phase < 6u; ++phase)
        if (!ResourceSceneCall(&scene, 0x86bb68u + phase * 3u, children[phase], 2u))
            return scene.result;
    OpLoadA(cpu, 0x1bu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpLdy(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, WRAM_FIELD_STREAMED_ROW_VRAM - 2u), cpu->y);
    OpLdx(cpu, 0xbbf9u);
    ResourceSceneCall(&scene, 0x86bb88u, 0x86c1d2u, 2u);
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapRunOffsetGroupScene(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86be30u);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86be71u)};
    if (!ResourceSceneCall(&scene, 0x86be30u, 0x86bbe6u, 2u))
        return scene.result;
    OpLdx(cpu, 0xe04fu);
    OpLdy(cpu, 0xe033u);
    static const uint32_t children[] = {0x869470u, 0x8692a1u, 0x86bb8cu, 0x86c284u,
                                        0x86be7au};
    for (unsigned phase = 0u; phase < 5u; ++phase)
        if (!ResourceSceneCall(&scene, 0x86be39u + phase * 3u, children[phase], 2u))
            return scene.result;
    OpLdx(cpu, SCENE_OBJECT_A);
    if (!ResourceSceneCall(&scene, 0x86be4bu, 0x86be72u, 2u))
        return scene.result;
    OpLdx(cpu, SCENE_OBJECT_B);
    if (!ResourceSceneCall(&scene, 0x86be51u, 0x86be72u, 2u))
        return scene.result;
    OpStepMem(memory, cpu, WorldSceneObjectField(cpu, 0x11u), true);
    OpLdx(cpu, SCENE_OBJECT_H);
    if (!ResourceSceneCall(&scene, 0x86be59u, 0x86be72u, 2u))
        return scene.result;
    OpStepMem(memory, cpu, WorldSceneObjectField(cpu, 0x11u), true);
    OpStepMem(memory, cpu, WorldSceneObjectField(cpu, 0x11u), true);
    if (!ResourceSceneCall(&scene, 0x86be60u, 0x86c582u, 2u))
        return scene.result;
    OpLoadA(cpu, 0x1bu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_SKYLINE_BAND_COUNT));
    OpLdx(cpu, 0xbed9u);
    ResourceSceneCall(&scene, 0x86be6eu, 0x86c1d2u, 2u);
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapRunDualObjectScene(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c04eu);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86c08bu)};
    if (!ResourceSceneCall(&scene, 0x86c04eu, 0x86c08cu, 2u))
        return scene.result;
    OpLdx(cpu, 0xe0a3u);
    OpLdy(cpu, 0xe095u);
    if (!ResourceSceneCall(&scene, 0x86c057u, 0x869470u, 2u) ||
        !ResourceSceneCall(&scene, 0x86c05au, 0x8692a1u, 2u) ||
        !ResourceSceneCall(&scene, 0x86c05du, 0x86bb8cu, 2u))
        return scene.result;
    for (unsigned actor = 0u; actor < 2u; ++actor) {
        OpLdx(cpu, (uint16_t)(SCENE_OBJECT_A + actor * 0x1du));
        OpStz(memory, cpu, WorldSceneObjectField(cpu, 0x0fu));
        OpLoadA(cpu, (uint16_t)(0x22u + actor));
        if (!ResourceSceneCall(&scene, 0x86c067u + actor * 10u, 0x86e0b9u, 2u))
            return scene.result;
    }
    OpStz(memory, cpu, OpAbs(cpu, SCENE_WINDOW_ORIGIN));
    OpLdy(cpu, 0x1800u);
    OpWrite16(memory, OpAbs(cpu, SCENE_WINDOW_WIDTH), cpu->y);
    OpLoadA(cpu, 0x1du);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_SKYLINE_BAND_COUNT));
    OpLdx(cpu, 0xc09fu);
    ResourceSceneCall(&scene, 0x86c088u, 0x86c1d2u, 2u);
    return scene.result;
}

static bool PrepareGroupTransition(WorldSceneResources *scene) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_RESOURCE_VARIANT));
    if (!ResourceSceneCall(scene, 0x86b640u, 0x86b19au, 2u) ||
        !PrepareActorScene(scene, 0x86b643u, 0xdfdfu, 0xdfd1u, 0x860u, 0x8b8u) ||
        !ResourceSceneCall(scene, 0x86b667u, 0x86c582u, 2u))
        return false;
    OpLdy(cpu, 0xb82fu);
    OpLoadA(cpu, 0u);
    if (!ResourceSceneCall(scene, 0x86b66fu, 0x86c336u, 2u))
        return false;
    for (unsigned buffer = 0u; buffer < 2u; ++buffer) {
        OpLoadA(cpu, 0u);
        OpStz(memory, cpu, OpDp(cpu, 0u));
        if (!ResourceSceneCall(scene, 0x86b676u + buffer * 7u, 0x86c2d9u, 2u))
            return false;
    }
    OpLoadA(cpu, 0xb1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 0xeau);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_SCROLL_Y + 6u), -1);
    OpLdx(cpu, 0xb732u);
    return ResourceSceneCall(scene, 0x86b695u, 0x86c1d2u, 2u) &&
           ResourceSceneCall(scene, 0x86b698u, 0x86d37du, 2u);
}

static bool ReloadTransitionScene(WorldSceneResources *scene) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    OpLdx(cpu, 0u);
    for (unsigned axis = 0u; axis < 3u; ++axis)
        OpWrite16(memory, OpAbs(cpu, (uint16_t)(WRAM_FIELD_LAYER_SCROLL_Y + axis * 2u)), cpu->x);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_RESOURCE_VARIANT));
    OpLdy(cpu, 0x290u);
    OpWrite16(memory, OpDp(cpu, RESOURCE_INDEX), cpu->y);
    OpLdx(cpu, 0xe000u);
    OpWrite16(memory, OpDp(cpu, RESOURCE_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, RESOURCE_DESTINATION_BANK));
    if (!ResourceSceneCall(scene, 0x86b6bau, 0x808e9du, 3u) ||
        !ResourceSceneCall(scene, 0x86b6beu, 0x86d397u, 2u))
        return false;
    OpLdx(cpu, 0xe001u);
    OpLdy(cpu, 0xdff3u);
    if (!ResourceSceneCall(scene, 0x86b6c7u, 0x869470u, 2u))
        return false;
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_DIRTY), -1);
    if (!ResourceSceneCall(scene, 0x86b6cdu, 0x8692a1u, 2u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0xb220u);
    OpLdy(cpu, SCENE_PALETTE_INPUT);
    OpLoadA(cpu, SCENE_PALETTE_BYTES - 1u);
    PushDataBank(memory, cpu);
    OpMoveNext(memory, cpu, 0x7eu, 0xa6u);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ResourceSceneCall(scene, 0x86b6e2u, 0x86a0a2u, 2u);
}

static bool StartTransitionObjects(WorldSceneResources *scene) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    OpLdx(cpu, SCENE_OBJECT_B);
    OpStz(memory, cpu, WorldSceneObjectField(cpu, 0x0fu));
    OpLoadA(cpu, 0x13u);
    if (!ResourceSceneCall(scene, 0x86b6ecu, 0x86e0b9u, 2u))
        return false;
    WorldSceneCopyPositionToObject(memory, cpu);
    OpLdy(cpu, 0xfff8u);
    OpWrite16(memory, WorldSceneObjectField(cpu, 0x0du), cpu->y);
    OpLdx(cpu, SCENE_OBJECT_V);
    OpStz(memory, cpu, WorldSceneObjectField(cpu, 0x0fu));
    OpLoadA(cpu, 0xcu);
    if (!ResourceSceneCall(scene, 0x86b705u, 0x86e0b9u, 2u))
        return false;
    OpLoadA(cpu, 0x38u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 0x12u));
    OpLdy(cpu, 0u);
    OpWrite16(memory, WorldSceneObjectField(cpu, 5u), cpu->y);
    OpLdy(cpu, 0x80u);
    OpWrite16(memory, WorldSceneObjectField(cpu, 7u), cpu->y);
    OpLdy(cpu, 0x70u);
    OpWrite16(memory, WorldSceneObjectField(cpu, 9u), cpu->y);
    return ResourceSceneCall(scene, 0x86b71bu, 0x86c582u, 2u);
}

Lufia2ExecutionResult Lufia2WorldMapRunGroupTransitionScene(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            Lufia2PushedChildCall child,
                                                            void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b63bu);
    WorldSceneResources scene = {memory, cpu, child, context,
                                 ExecutionReturned(0x86b731u)};
    if (!PrepareGroupTransition(&scene) || !ReloadTransitionScene(&scene) ||
        !StartTransitionObjects(&scene))
        return scene.result;
    OpLoadA(cpu, 0x6fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x19u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_SKYLINE_BAND_COUNT));
    OpLdx(cpu, 0xb835u);
    ResourceSceneCall(&scene, 0x86b72eu, 0x86c1d2u, 2u);
    return scene.result;
}
