#include "core/child_call.h"
#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "system/wram.h"
#include "world/world_scene_internal.h"
#include "world/world_scene_resources_internal.h"

enum {
    SCENE_OBJECT_SIZE = 0x1du,
    SCENE_PARTICLES = WRAM_WORLD_MAP_OBJECT_TABLE + 7u * SCENE_OBJECT_SIZE,
    SCENE_OBJECT_X = 2u,
    SCENE_OBJECT_Y = 5u,
    SCENE_OBJECT_VARIANT = 15u,
    SCENE_OBJECT_FRAME = 17u,
    SCENE_PLAYER_X = WRAM_WORLD_MAP_PLAYER_OBJECT + 3u,
    SCENE_PLAYER_Y = WRAM_WORLD_MAP_PLAYER_OBJECT + 5u,
    SCENE_BURST_AGE = WRAM_FIELD_LAYER_SCROLL_Y + 2u,
    SCENE_PULSE_AGE = WRAM_FIELD_LAYER_SCROLL_Y + 4u,
    SCENE_PULSE_STEPS = WRAM_FIELD_LAYER_SCROLL_Y + 5u,
    SCENE_PULSE_TEMPLATE = WRAM_FIELD_LAYER_SCROLL_Y + 6u,
    SCENE_ORBIT_CENTER_X = WRAM_FIELD_STREAMED_ROW_SOURCE + 3u,
    SCENE_ORBIT_CENTER_Y = WRAM_FIELD_STREAMED_ROW_SOURCE + 6u,
    SCENE_ORBIT_ANGLE = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 1u,
    SCENE_ORBIT_RADIUS = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 2u,
    SCENE_TRAVEL_ANGLE = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 4u,
    SCENE_TRAVEL_DISTANCE = WRAM_FIELD_STREAMED_COLUMN_SOURCE + 6u,
    SCENE_VECTOR_PHASE = WRAM_FIELD_STREAMED_COLUMN_VRAM + 1u,
    SCENE_VECTOR_ANGLE = WRAM_FIELD_STREAMED_COLUMN_VRAM + 2u,
    SCENE_VECTOR_DISTANCE = WRAM_FIELD_STREAMED_COLUMN_VRAM + 3u,
    SCENE_PULSE_OBJECT_X = WRAM_WORLD_MAP_OBJECT_TABLE + 6u * SCENE_OBJECT_SIZE + 2u,
    SCENE_PULSE_WINDOW = WRAM_WORLD_SCENE_WINDOW_RECORD_STORAGE,
    DP_PARTICLE_COUNT = 4u,
    DP_BURST_COUNT = 0x63u,
    DP_POSITION_X = 4u,
    DP_POSITION_Y = 6u,
    DP_VECTOR_X = 8u,
    DP_VECTOR_Y = 11u
};

typedef struct SceneMotion {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} SceneMotion;

static bool CallMotionChild(SceneMotion *motion, uint32_t site, uint32_t target) {
    const uint8_t frame_size = target == 0x808299u ? 3u : 2u;
    if (!CallChildWithFrame(motion->memory, motion->cpu, motion->child, motion->context,
                            site, target, frame_size, 0x86u)) {
        motion->result = ExecutionReturned(site);
        motion->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceEntry(motion->cpu)) {
        motion->result = ExecutionHandoff(motion->cpu, site + frame_size + 1u);
        return false;
    }
    return true;
}

static bool FindInactiveSceneSprite(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint16_t count, bool carry_between_records) {
    OpLdy(cpu, count);
    if (carry_between_records)
        cpu->carry = 0u;
    do {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y));
        if (cpu->negative)
            return true;
        OpTxa(cpu);
        if (!carry_between_records)
            cpu->carry = 0u;
        OpAdcValue(cpu, SCENE_OBJECT_SIZE);
        OpTax(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return false;
}

static void RetireSceneSprites(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint8_t count, uint8_t last_frame, uint8_t counter) {
    OpLdx(cpu, SCENE_PARTICLES);
    OpLoadA(cpu, count);
    OpSta(memory, cpu, OpDp(cpu, counter));
    do {
        OpLdy(cpu, OpRead16(memory, WorldSceneObjectField(cpu, SCENE_OBJECT_Y)));
        if (!cpu->negative) {
            OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_FRAME));
            OpCmpValue(cpu, last_frame);
            if (cpu->carry) {
                OpLdy(cpu, 0xffffu);
                OpWrite16(memory, WorldSceneObjectField(cpu, SCENE_OBJECT_Y), cpu->y);
            }
        }
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, SCENE_OBJECT_SIZE);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, counter), -1);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2WorldMapStepSceneSparkSprites(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b51cu);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86b56cu)};
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PULSE_TEMPLATE));
    if (!cpu->zero) {
        OpLoadA(cpu, 8u);
        if (!CallMotionChild(&motion, 0x86b523u, 0x808299u))
            return motion.result;
        OpDecA(cpu);
        if (cpu->zero) {
            OpRepWidths(cpu, 0x20u);
            OpLdx(cpu, SCENE_PARTICLES);
            const bool available = FindInactiveSceneSprite(memory, cpu, 3u, true);
            OpSepWidths(cpu, 0x20u);
            if (available && !CallMotionChild(&motion, 0x86b545u, 0x86b56du))
                return motion.result;
        }
    }
    RetireSceneSprites(memory, cpu, 3u, 2u, DP_PARTICLE_COUNT);
    return motion.result;
}

Lufia2ExecutionResult Lufia2WorldMapSpawnSceneSparkSprite(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b56du);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86b59du)};
    OpPushX(memory, cpu);
    OpLoadA(cpu, 4u);
    if (!CallMotionChild(&motion, 0x86b570u, 0x808299u))
        return motion.result;
    OpPullX(memory, cpu);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_VARIANT));
    OpPushX(memory, cpu);
    OpLoadA(cpu, 2u);
    if (!CallMotionChild(&motion, 0x86b57au, 0x808299u))
        return motion.result;
    OpPullX(memory, cpu);
    OpLsrA(cpu);
    OpLoadA(cpu, cpu->carry ? 0x2eu : 0x2du);
    if (!CallMotionChild(&motion, 0x86b586u, 0x86e0b9u))
        return motion.result;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PLAYER_X));
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_X));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PLAYER_Y));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x21u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y));
    OpSepWidths(cpu, 0x20u);
    return motion.result;
}

static bool SpawnSceneBurstSprite(SceneMotion *motion) {
    const Lufia2Memory *memory = motion->memory;
    Lufia2CpuState *cpu = motion->cpu;
    OpStz(memory, cpu, OpAbs(cpu, SCENE_BURST_AGE));
    OpLoadA(cpu, 0x32u);
    if (!CallMotionChild(motion, 0x86bc1eu, 0x808299u))
        return false;
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x68u);
    OpSta(memory, cpu, OpDp(cpu, DP_POSITION_X));
    OpStz(memory, cpu, OpDp(cpu, DP_POSITION_X + 1u));
    OpLoadA(cpu, 0x28u);
    if (!CallMotionChild(motion, 0x86bc2bu, 0x808299u))
        return false;
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x64u);
    OpSta(memory, cpu, OpDp(cpu, DP_POSITION_Y));
    OpStz(memory, cpu, OpDp(cpu, DP_POSITION_Y + 1u));
    OpLdx(cpu, SCENE_PARTICLES);
    OpRepWidths(cpu, 0x20u);
    if (!FindInactiveSceneSprite(memory, cpu, 14u, false)) {
        OpSepWidths(cpu, 0x20u);
        return true;
    }
    static const uint8_t positions[] = {DP_POSITION_X, DP_POSITION_Y};
    static const uint16_t centers[] = {SCENE_PLAYER_X, SCENE_PLAYER_Y};
    static const uint16_t origins[] = {0x80u, 0x70u};
    static const uint8_t fields[] = {SCENE_OBJECT_X, SCENE_OBJECT_Y};
    for (unsigned axis = 0u; axis < 2u; ++axis) {
        OpLda(memory, cpu, OpDp(cpu, positions[axis]));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, centers[axis]));
        cpu->carry = 1u;
        OpSbcValue(cpu, origins[axis]);
        OpSta(memory, cpu, WorldSceneObjectField(cpu, fields[axis]));
    }
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_VARIANT));
    OpLoadA(cpu, 0x1fu);
    return CallMotionChild(motion, 0x86bc6du, 0x86e0b9u);
}

Lufia2ExecutionResult Lufia2WorldMapStepBurstSceneFrame(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bc0fu);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86bca5u)};
    if (!CallMotionChild(&motion, 0x86bc0fu, 0x86bbc4u))
        return motion.result;
    OpLda(memory, cpu, OpAbs(cpu, SCENE_BURST_AGE));
    OpCmpValue(cpu, 8u);
    if (cpu->carry && !SpawnSceneBurstSprite(&motion))
        return motion.result;
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_BURST_AGE), 1);
    RetireSceneSprites(memory, cpu, 14u, 8u, DP_BURST_COUNT);
    OpLdy(cpu, 0xbe24u);
    if (!CallMotionChild(&motion, 0x86bc9au, 0x86bd9fu))
        return motion.result;
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCENE_TRAVEL_DISTANCE)));
    if (!cpu->zero && !CallMotionChild(&motion, 0x86bca2u, 0x86bca6u))
        return motion.result;
    return motion.result;
}

static void MoveOrbitCenter(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t center, uint8_t vector) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, center));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, vector));
    OpSta(memory, cpu, OpAbs(cpu, center));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, center + 2u));
    OpAdc(memory, cpu, OpDp(cpu, vector + 2u));
    OpSta(memory, cpu, OpAbs(cpu, center + 2u));
}

static void PlaceOrbitSprites(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t centers[] = {SCENE_ORBIT_CENTER_X, SCENE_ORBIT_CENTER_Y};
    static const uint16_t players[] = {SCENE_PLAYER_X, SCENE_PLAYER_Y};
    static const uint8_t positions[] = {DP_POSITION_X, DP_POSITION_Y};
    static const uint8_t vectors[] = {DP_VECTOR_X + 1u, DP_VECTOR_Y + 1u};
    static const uint8_t fields[] = {SCENE_OBJECT_X, SCENE_OBJECT_Y};
    OpRepWidths(cpu, 0x20u);
    for (unsigned axis = 0u; axis < 2u; ++axis) {
        OpLda(memory, cpu, OpAbs(cpu, centers[axis] + 1u));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, players[axis]));
        OpSta(memory, cpu, OpDp(cpu, positions[axis]));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, vectors[axis]));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + fields[axis]));
    }
    for (unsigned axis = 0u; axis < 2u; ++axis) {
        OpLda(memory, cpu, OpDp(cpu, positions[axis]));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, vectors[axis])));
        OpSta(
            memory, cpu,
            OpAbs(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + SCENE_OBJECT_SIZE + fields[axis]));
    }
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2WorldMapAdvanceSceneOrbit(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bca6u);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86bd46u)};
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_ORBIT_ANGLE), 1);
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_ORBIT_ANGLE), 1);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_ORBIT_RADIUS));
    cpu->carry = 0u;
    OpAdcValue(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_ORBIT_RADIUS));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_TRAVEL_ANGLE));
    OpCmpValue(cpu, 0xb000u);
    if (!cpu->carry)
        OpAdcValue(cpu, 0xa0u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_TRAVEL_ANGLE));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_VECTOR_PHASE));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_TRAVEL_DISTANCE));
    cpu->carry = 0u;
    OpAdcValue(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_TRAVEL_DISTANCE));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_VECTOR_DISTANCE));
    OpSepWidths(cpu, 0x20u);
    if (!CallMotionChild(&motion, 0x86bcd8u, 0x86a417u))
        return motion.result;
    MoveOrbitCenter(memory, cpu, SCENE_ORBIT_CENTER_X, DP_VECTOR_X);
    MoveOrbitCenter(memory, cpu, SCENE_ORBIT_CENTER_Y, DP_VECTOR_Y);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_ORBIT_ANGLE));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_VECTOR_ANGLE));
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCENE_ORBIT_RADIUS)));
    OpWrite16(memory, OpAbs(cpu, SCENE_VECTOR_DISTANCE), cpu->y);
    if (!CallMotionChild(&motion, 0x86bd11u, 0x86a417u))
        return motion.result;
    PlaceOrbitSprites(memory, cpu);
    return motion.result;
}

static bool StepPulseObject(SceneMotion *motion) {
    const Lufia2Memory *memory = motion->memory;
    Lufia2CpuState *cpu = motion->cpu;
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PULSE_STEPS));
    if (cpu->zero)
        return true;
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PULSE_AGE));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PULSE_OBJECT_X));
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PULSE_OBJECT_X));
    OpSepWidths(cpu, 0x20u);
    if (!CallMotionChild(motion, 0x86bdb9u, 0x86c2b3u))
        return false;
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_PULSE_AGE), 1);
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_PULSE_STEPS), -1);
    return true;
}

Lufia2ExecutionResult Lufia2WorldMapStepSceneCirclePulse(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bd9fu);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86bdf5u)};
    OpWrite16(memory, OpAbs(cpu, SCENE_PULSE_TEMPLATE), cpu->y);
    if (!StepPulseObject(&motion))
        return motion.result;
    OpLdx(cpu, SCENE_PULSE_WINDOW);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, 4u));
    if (cpu->zero) {
        OpLoadA(cpu, 10u);
        if (!CallMotionChild(&motion, 0x86bdcbu, 0x808299u))
            return motion.result;
        OpDecA(cpu);
        if (!cpu->zero)
            return motion.result;
        if (!CallMotionChild(&motion, 0x86bdd2u, 0x86bdf6u))
            return motion.result;
    }
    OpStepMem(memory, cpu, WorldSceneObjectField(cpu, 2u), 1);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, 5u));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, WorldSceneObjectField(cpu, 2u)));
    if (!cpu->carry) {
        cpu->carry = 0u;
        OpAdcValue(cpu, 12u);
        if (cpu->carry) {
            OpOraValue(cpu, 0x20u);
            OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
        } else {
            OpStz(memory, cpu, WorldSceneObjectField(cpu, 4u));
            OpStz(memory, cpu, WorldSceneObjectField(cpu, 2u));
        }
    }
    OpLoadA(cpu, 0u);
    OpStz(memory, cpu, OpDp(cpu, 0u));
    if (!CallMotionChild(&motion, 0x86bdf2u, 0x86c2d9u))
        return motion.result;
    return motion.result;
}

Lufia2ExecutionResult Lufia2WorldMapStartSceneCirclePulse(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86bdf6u);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86be23u)};
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCENE_PULSE_TEMPLATE)));
    OpLoadA(cpu, 0u);
    if (!CallMotionChild(&motion, 0x86bdfbu, 0x86c336u))
        return motion.result;
    OpLoadA(cpu, 0x28u);
    if (!CallMotionChild(&motion, 0x86be00u, 0x808299u))
        return motion.result;
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SCENE_PULSE_WINDOW));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PULSE_WINDOW));
    OpLoadA(cpu, 0x1eu);
    if (!CallMotionChild(&motion, 0x86be0du, 0x808299u))
        return motion.result;
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SCENE_PULSE_WINDOW + 1u));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PULSE_WINDOW + 1u));
    OpLoadA(cpu, 0x2cu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLdy(cpu, 0x1000u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PULSE_AGE), cpu->y);
    return motion.result;
}
