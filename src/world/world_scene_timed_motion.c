#include <stddef.h>

#include "core/child_call.h"
#include "lufia2/world_scene_timed_motion.h"
#include "system/wram.h"
#include "world/world_scene_internal.h"
#include "world/world_scene_resources_internal.h"
#include "world/world_fade_internal.h"

enum {
    SCENE_FIRST_OBJECT = WRAM_WORLD_MAP_OBJECT_TABLE,
    SCENE_SECOND_OBJECT = WRAM_WORLD_MAP_OBJECT_TABLE + 0x1du,
    SCENE_EXTRA_OBJECT = WRAM_WORLD_MAP_OBJECT_TABLE + 21u * 0x1du,
    SCENE_PARTICLE_OBJECTS = WRAM_WORLD_MAP_OBJECT_TABLE + 6u * 0x1du,
    SCENE_ORBIT_PARTICLES = WRAM_WORLD_MAP_OBJECT_TABLE + 2u * 0x1du,
    SCENE_FRAME = WRAM_FIELD_LAYER_SCROLL_Y,
    SCENE_AGE = WRAM_FIELD_LAYER_SCROLL_Y + 2u,
    SCENE_TRAVEL = WRAM_FIELD_LAYER_SCROLL_Y + 4u,
    SCENE_PULSE_POSITION = WRAM_FIELD_STREAMED_ROW_SOURCE + 3u,
    SCENE_PULSE_BANK = WRAM_FIELD_STREAMED_ROW_SOURCE + 5u,
    SCENE_PULSE_DISTANCE = WRAM_FIELD_STREAMED_ROW_SOURCE + 6u,
    SCENE_PULSE_FLAGS = WRAM_FIELD_STREAMED_COLUMN_SOURCE,
    SCENE_ORBIT_ANGLE = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 1u,
    SCENE_ORBIT_RADIUS = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 2u,
    SCENE_PULSE_LIMIT = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 6u,
    SCENE_VECTOR_ANGLE = WRAM_FIELD_STREAMED_COLUMN_VRAM + 2u,
    SCENE_VECTOR_RADIUS = WRAM_FIELD_STREAMED_COLUMN_VRAM + 3u,
    DP_ANIMATION = 0x63u,
    DP_VECTOR_X = 8u,
    DP_VECTOR_Y = 12u
};

typedef struct TimedScene {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} TimedScene;

static bool CallTimedChild(TimedScene *scene, uint32_t site, uint32_t target) {
    if (!CallChildWithFrame(scene->memory, scene->cpu, scene->child,
                            scene->context, site, target, 2u, 0x86u)) {
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

Lufia2ExecutionResult Lufia2WorldMapTrackRisingSceneActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b5dfu);
    OpLdx(cpu, SCENE_SECOND_OBJECT);
    WorldSceneCopyPositionToObject(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_AGE));
    if (!cpu->zero) {
        OpCmpValue(cpu, 100u);
        if (!cpu->carry) {
            OpLdx(cpu, SCENE_SECOND_OBJECT);
            OpLsrA(cpu);
            OpLsrA(cpu);
            OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
            OpIncA(cpu);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpAbs(cpu, SCENE_TRAVEL));
            OpSta(memory, cpu, WorldSceneObjectField(cpu, 13u));
            OpStepMem(memory, cpu, OpAbs(cpu, SCENE_AGE), 1);
        }
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86b60cu);
}

Lufia2ExecutionResult Lufia2WorldMapStartRisingSceneActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b60du);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86b636u)};
    OpLdx(cpu, SCENE_SECOND_OBJECT);
    OpLoadA(cpu, 3u);
    if (!CallTimedChild(&scene, 0x86b612u, 0x86e0b9u))
        return scene.result;
    OpLdy(cpu, OpRead16(memory, WorldSceneObjectField(cpu, 13u)));
    OpWrite16(memory, OpAbs(cpu, SCENE_TRAVEL), cpu->y);
    OpLdx(cpu, SCENE_EXTRA_OBJECT);
    OpStz(memory, cpu, WorldSceneObjectField(cpu, 15u));
    OpLoadA(cpu, 12u);
    if (!CallTimedChild(&scene, 0x86b621u, 0x86e0b9u))
        return scene.result;
    static const uint16_t values[] = {0u, 0x80u, 0x70u};
    for (unsigned field = 0u; field < 3u; ++field) {
        OpLdy(cpu, values[field]);
        OpWrite16(memory, WorldSceneObjectField(cpu, (uint8_t)(5u + field * 2u)), cpu->y);
    }
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_AGE), 1);
    return scene.result;
}

static bool FinishTimedChild(TimedScene *scene, uint16_t landing) {
    const uint8_t low = Pull8(scene->memory, scene->cpu);
    const uint8_t high = Pull8(scene->memory, scene->cpu);
    const uint16_t target = (uint16_t)(((high << 8) | low) + 1u);
    if (target == landing)
        return true;
    scene->result = ExecutionHandoff(scene->cpu, 0x860000u | target);
    return false;
}

static Lufia2ExecutionResult TimedFade(const Lufia2Memory *memory,
                                      Lufia2CpuState *cpu,
                                      uint32_t entry, uint32_t site,
                                      uint8_t rate, bool default_rate) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, entry);
    TimedScene scene = {memory, cpu, NULL, NULL, ExecutionReturned(site + 3u)};
    if (!default_rate)
        OpLoadA(cpu, rate);
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    if (default_rate)
        OpLoadA(cpu, rate);
    WorldSceneStartFade(memory, cpu, 0xc0u);
    FinishTimedChild(&scene, (uint16_t)(site + 3u));
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapStopTimedSceneSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return TimedFade(memory, cpu, 0x86b637u, 0x86b637u, 0x10u, true);
}

static bool SetPairedAnimation(TimedScene *scene) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    OpWrite16(memory, OpDp(cpu, DP_ANIMATION), cpu->y);
    OpLdx(cpu, SCENE_FIRST_OBJECT);
    OpStz(memory, cpu, WorldSceneObjectField(cpu, 15u));
    if (!CallTimedChild(scene, 0x86bd91u, 0x86e0b9u))
        return false;
    OpLdx(cpu, SCENE_SECOND_OBJECT);
    OpStz(memory, cpu, WorldSceneObjectField(cpu, 15u));
    OpLda(memory, cpu, OpDp(cpu, DP_ANIMATION));
    return CallTimedChild(scene, 0x86bd9bu, 0x86e0b9u);
}

Lufia2ExecutionResult Lufia2WorldMapStartPairedScenePulse(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bd47u);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86bd76u)};
    OpLoadA(cpu, 0x28u);
    OpLdy(cpu, 0x2cu);
    SimulateJsrFrame(memory, cpu, 0xbd4eu);
    if (!SetPairedAnimation(&scene) || !FinishTimedChild(&scene, 0xbd4fu))
        return scene.result;
    OpLdy(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PULSE_POSITION), cpu->y);
    OpStz(memory, cpu, OpAbs(cpu, SCENE_PULSE_BANK));
    OpLdy(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PULSE_DISTANCE), cpu->y);
    OpStz(memory, cpu, OpAbs(cpu, SCENE_PULSE_FLAGS));
    OpStz(memory, cpu, OpAbs(cpu, SCENE_ORBIT_ANGLE));
    OpLdy(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, SCENE_ORBIT_RADIUS + 2u), cpu->y);
    OpLdy(cpu, 0x100u);
    OpWrite16(memory, OpAbs(cpu, SCENE_ORBIT_RADIUS), cpu->y);
    OpLdy(cpu, 0x40u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PULSE_LIMIT), cpu->y);
    return scene.result;
}

static Lufia2ExecutionResult PairedAnimation(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *context, uint32_t entry,
                                            uint8_t animation) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, entry);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86bd9eu)};
    OpLoadA(cpu, animation);
    OpLdy(cpu, (uint16_t)(animation + 4u));
    SetPairedAnimation(&scene);
    return scene.result;
}

Lufia2ExecutionResult Lufia2WorldMapSetPairedSceneAnimation27(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return PairedAnimation(memory, cpu, child, context, 0x86bd77u, 0x27u);
}

Lufia2ExecutionResult Lufia2WorldMapSetPairedSceneAnimation26(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return PairedAnimation(memory, cpu, child, context, 0x86bd7eu, 0x26u);
}

Lufia2ExecutionResult Lufia2WorldMapSetPairedSceneAnimation25(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return PairedAnimation(memory, cpu, child, context, 0x86bd85u, 0x25u);
}

Lufia2ExecutionResult Lufia2WorldMapSetSceneSoundMode2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return TimedFade(memory, cpu, 0x86be2au, 0x86be2cu, 2u, false);
}

Lufia2ExecutionResult Lufia2WorldMapSetSceneSoundMode1(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    return TimedFade(memory, cpu, 0x86c1ccu, 0x86c1ceu, 1u, false);
}

Lufia2ExecutionResult Lufia2WorldMapStepTrailingSceneParticles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bee6u);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86bf3au)};
    if (!CallTimedChild(&scene, 0x86bee6u, 0x86be7au))
        return scene.result;
    OpLda(memory, cpu, OpAbs(cpu, SCENE_FRAME));
    OpAndValue(cpu, 7u);
    if (cpu->zero) {
        static const uint16_t objects[] = {SCENE_FIRST_OBJECT, SCENE_SECOND_OBJECT,
                                           SCENE_FIRST_OBJECT + 7u * 0x1du};
        for (unsigned object = 0u; object < 3u; ++object) {
            OpLdx(cpu, OpRead16(memory, OpAbs(cpu, objects[object] + 2u)));
            OpLdy(cpu, OpRead16(memory, OpAbs(cpu, objects[object] + 5u)));
            if (!CallTimedChild(&scene, 0x86bef6u + 9u * object, 0x86bf3bu))
                return scene.result;
        }
    }
    OpLdx(cpu, SCENE_PARTICLE_OBJECTS + 2u * 0x1du);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 13u);
    OpSta(memory, cpu, OpDp(cpu, DP_ANIMATION));
    do {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, 5u));
        if (!cpu->negative) {
            OpSepWidths(cpu, 0x20u);
            OpLda(memory, cpu, WorldSceneObjectField(cpu, 17u));
            OpCmpValue(cpu, 8u);
            OpRepWidths(cpu, 0x20u);
            if (cpu->carry) {
                OpLoadA(cpu, 0xffffu);
                OpSta(memory, cpu, WorldSceneObjectField(cpu, 5u));
            }
        }
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x1du);
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_ANIMATION), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0xbf6au);
    CallTimedChild(&scene, 0x86bf37u, 0x86bd9fu);
    return scene.result;
}

static bool SpawnOrbitParticle(TimedScene *scene, unsigned object) {
    const Lufia2Memory *memory = scene->memory;
    Lufia2CpuState *cpu = scene->cpu;
    const uint16_t source = (uint16_t)(SCENE_FIRST_OBJECT + object * 0x1du);
    if (!CallTimedChild(scene, 0x86c154u + object * 20u, 0x86c1b4u))
        return false;
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, source + 2u)));
    OpWrite16(memory, WorldSceneObjectField(cpu, 2u), cpu->y);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, source + 5u)));
    OpWrite16(memory, WorldSceneObjectField(cpu, 5u), cpu->y);
    OpStz(memory, cpu, WorldSceneObjectField(cpu, 15u));
    OpLoadA(cpu, 0x24u);
    return CallTimedChild(scene, 0x86c165u + object * 20u, 0x86e0b9u);
}

Lufia2ExecutionResult Lufia2WorldMapStepOrbitingSceneParticles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86c0acu);
    TimedScene scene = {memory, cpu, child, context, ExecutionReturned(0x86c1b3u)};
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_AGE), 1);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCENE_FRAME)));
    OpCpy(cpu, 0x180u);
    if (cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, SCENE_AGE));
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x40u);
        bool expand = cpu->negative != 0u;
        if (cpu->zero) {
            OpStepMem(memory, cpu, OpAbs(cpu, SCENE_AGE), -1);
            expand = true;
        }
        if (expand) {
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, SCENE_TRAVEL));
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x80u);
            OpSta(memory, cpu, OpAbs(cpu, SCENE_TRAVEL));
            OpSepWidths(cpu, 0x20u);
        }
    }
    OpLda(memory, cpu, OpAbs(cpu, SCENE_AGE));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_VECTOR_ANGLE));
    OpLdy(cpu, 0x800u);
    OpWrite16(memory, OpAbs(cpu, SCENE_VECTOR_RADIUS), cpu->y);
    if (!CallTimedChild(&scene, 0x86c0e0u, 0x86a417u))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_X));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x1000u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_ORBIT_RADIUS));
    OpSepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_ORBIT_ANGLE), 1);
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_ORBIT_ANGLE), 1);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_ORBIT_ANGLE));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_VECTOR_ANGLE));
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCENE_ORBIT_RADIUS)));
    OpWrite16(memory, OpAbs(cpu, SCENE_VECTOR_RADIUS), cpu->y);
    if (!CallTimedChild(&scene, 0x86c102u, 0x86a417u))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_X_LOW));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_VECTOR_X + 1u));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_FIRST_OBJECT + 2u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_X_LOW));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_VECTOR_X + 1u)));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_SECOND_OBJECT + 2u));
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_Y));
    OpAslA(cpu);
    const uint16_t vector = OpRead16(memory, OpDp(cpu, DP_VECTOR_Y));
    const uint16_t half = (uint16_t)((vector >> 1) | (cpu->carry ? 0x8000u : 0u));
    cpu->carry = vector & 1u;
    Write8(memory, DirectAddress(cpu, DP_VECTOR_Y + 1u), (uint8_t)(half >> 8));
    Write8(memory, DirectAddress(cpu, DP_VECTOR_Y), (uint8_t)half);
    SetNz16(cpu, half);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_TRAVEL + 1u));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_Y));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_VECTOR_Y));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0u)));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_FIRST_OBJECT + 5u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_Y));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_VECTOR_Y)));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0u)));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_SECOND_OBJECT + 5u));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_ORBIT_ANGLE));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_VECTOR_ANGLE));
    if (!CallTimedChild(&scene, 0x86c146u, 0x86a417u))
        return scene.result;
    OpRepWidths(cpu, 0x20u);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_FRAME));
    OpAndValue(cpu, 7u);
    if (cpu->zero) {
        if (!SpawnOrbitParticle(&scene, 0u) || !SpawnOrbitParticle(&scene, 1u))
            return scene.result;
    }
    OpLdx(cpu, SCENE_ORBIT_PARTICLES);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 20u);
    OpSta(memory, cpu, OpDp(cpu, DP_ANIMATION));
    do {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, 5u));
        if (!cpu->negative) {
            OpLda(memory, cpu, WorldSceneObjectField(cpu, 4u));
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x100u);
            OpSta(memory, cpu, WorldSceneObjectField(cpu, 4u));
            OpSepWidths(cpu, 0x20u);
            OpLda(memory, cpu, WorldSceneObjectField(cpu, 6u));
            OpAdcValue(cpu, 0u);
            OpSta(memory, cpu, WorldSceneObjectField(cpu, 6u));
            OpLda(memory, cpu, WorldSceneObjectField(cpu, 17u));
            OpCmpValue(cpu, 6u);
            OpRepWidths(cpu, 0x20u);
            if (cpu->carry) {
                OpLoadA(cpu, 0xffffu);
                OpSta(memory, cpu, WorldSceneObjectField(cpu, 5u));
            }
        }
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x1du);
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_ANIMATION), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return scene.result;
}
