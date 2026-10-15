#include "core/child_call.h"
#include "lufia2/world_map.h"
#include "world/world_scene_internal.h"

enum {
    SCENE_SELECTOR = 0x54u,
    SCENE_PLANE_MODE = 0x11ddu,
    SCENE_VIEW_MODE = 0x11deu,
    SCENE_RESOURCE_DIRTY = WRAM_WORLD_MAP_RESOURCE_DIRTY,
    SCENE_FRAME = 0x1226u,
    SCENE_SCRIPT = 0x122eu,
    SCENE_CONTROL = WRAM_FIELD_STREAMED_ROW_SOURCE + 2u,
    SCENE_ACTOR = WRAM_WORLD_MAP_OBJECT_TABLE + 0x1du,
    SCENE_ACTOR_POSE = 0x0fu,
    ROM_SCENE_CALLBACKS = 0xaf0au,
    ROM_EMPTY_SCENE_SCRIPT = 0xaf38u,
    ROM_ACTOR_SCENE_SCRIPT = 0xaf6du
};

typedef struct WorldSceneSequence {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} WorldSceneSequence;

static bool SceneSequenceEntry(const Lufia2CpuState *cpu, Lufia2PushedChildCall child,
                               bool save_bank) {
    return child && cpu->program_bank == 0x86u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
           (save_bank || cpu->data_bank == 0x86u);
}

static bool SequenceCall(WorldSceneSequence *scene, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = scene->cpu;
    if (!CallChildWithFrame(scene->memory, cpu, scene->child, scene->context, site,
                            target, 2u, 0x86u)) {
        scene->result = ExecutionReturned(site);
        scene->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!SceneSequenceEntry(cpu, scene->child, false)) {
        scene->result = ExecutionHandoff(cpu, site + 3u);
        return false;
    }
    return true;
}

static void PrepareSceneSequence(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PLANE_MODE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_VIEW_MODE));
    OpStz(memory, cpu, OpAbs(cpu, SCENE_RESOURCE_DIRTY));
}

static void ResetSceneScript(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    for (unsigned field = 0u; field < 4u; ++field)
        OpWrite16(memory, OpAbs(cpu, (uint16_t)(SCENE_FRAME + field * 2u)), cpu->x);
    OpLdx(cpu, ROM_EMPTY_SCENE_SCRIPT);
    OpWrite16(memory, OpAbs(cpu, SCENE_SCRIPT), cpu->x);
    OpLoadA(cpu, 0x1du);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
}

Lufia2ExecutionResult Lufia2WorldMapRunSceneSequence(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    if (!SceneSequenceEntry(cpu, child, true))
        return ExecutionHandoff(cpu, 0x86aeb1u);
    WorldSceneSequence scene = {memory, cpu, child, context,
                                ExecutionReturned(0x86af09u)};
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0x86u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, SCENE_SELECTOR));
    OpAslA(cpu);
    OpTax(cpu);
    PushIndex(memory, cpu);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_RESOURCE_ID));
    if (!SequenceCall(&scene, 0x86aec8u, 0x86cdf5u))
        return scene.result;
    PrepareSceneSequence(memory, cpu);
    if (!SequenceCall(&scene, 0x86aed8u, 0x86ccfcu))
        return scene.result;
    ResetSceneScript(memory, cpu);
    OpPullX(memory, cpu);
    const uint16_t callback =
        Read16Bank(memory, 0x86u, (uint16_t)(ROM_SCENE_CALLBACKS + cpu->x));
    if (!SequenceCall(&scene, 0x86aef6u, 0x860000u | callback))
        return scene.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    if (!cpu->negative && !SequenceCall(&scene, 0x86aefeu, 0x86d37du))
        return scene.result;
    if (!SequenceCall(&scene, 0x86af01u, 0x86d397u) ||
        !SequenceCall(&scene, 0x86af04u, 0x86d330u))
        return scene.result;
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return scene.result;
}

static Lufia2ExecutionResult RunSceneActorTrack(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context, uint32_t entry,
                                                uint16_t track, uint16_t records) {
    if (!SceneSequenceEntry(cpu, child, false))
        return ExecutionHandoff(cpu, entry);
    WorldSceneSequence scene = {memory, cpu, child, context,
                                ExecutionReturned(0x86af6cu)};
    OpLdx(cpu, track);
    OpLdy(cpu, records);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_VIEW_MODE));
    if (!SequenceCall(&scene, 0x86af45u, 0x869470u) ||
        !SequenceCall(&scene, 0x86af48u, 0x8692a1u))
        return scene.result;
    OpLdx(cpu, SCENE_ACTOR);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_ACTOR_POSE));
    OpLoadA(cpu, 0x1cu);
    if (!SequenceCall(&scene, 0x86af54u, 0x86e0b9u))
        return scene.result;
    WorldSceneCopyPositionToObject(memory, cpu);
    OpLoadA(cpu, 0x1bu);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_CONTROL));
    OpLdx(cpu, ROM_ACTOR_SCENE_SCRIPT);
    if (!SequenceCall(&scene, 0x86af69u, 0x86c1d2u))
        return scene.result;
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack0(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    return RunSceneActorTrack(memory, cpu, child, context, 0x86af3au, 0xdd55u, 0xdd47u);
}

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack1(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    return RunSceneActorTrack(memory, cpu, child, context, 0x86af88u, 0xdda3u, 0xdd95u);
}

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack2(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    return RunSceneActorTrack(memory, cpu, child, context, 0x86af90u, 0xdde3u, 0xdd47u);
}

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack3(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    return RunSceneActorTrack(memory, cpu, child, context, 0x86af98u, 0xde39u, 0xde2bu);
}

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack4(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    return RunSceneActorTrack(memory, cpu, child, context, 0x86afa0u, 0xde81u, 0xdd95u);
}

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack5(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    return RunSceneActorTrack(memory, cpu, child, context, 0x86afa8u, 0xdecfu, 0xdec1u);
}

Lufia2ExecutionResult Lufia2WorldMapUpdateSceneActor(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    if (!SceneSequenceEntry(cpu, child, false))
        return ExecutionHandoff(cpu, 0x86af71u);
    WorldSceneSequence scene = {memory, cpu, child, context,
                                ExecutionReturned(0x86af87u)};
    OpLdx(cpu, SCENE_ACTOR);
    WorldSceneCopyPositionToObject(memory, cpu);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, WRAM_UNK_7E1210)));
    OpWrite16(memory, OpAbs(cpu, WRAM_WORLD_MAP_MOTION_HEADING), cpu->y);
    if (!SequenceCall(&scene, 0x86af84u, 0x869934u))
        return scene.result;
    return scene.result;
}
