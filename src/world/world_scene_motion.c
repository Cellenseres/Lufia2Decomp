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

enum {
    SCENE_PHASE_CALLBACK = WRAM_FIELD_STREAMED_ROW_SOURCE,
    SCENE_PHASE_SPEED = SCENE_BURST_AGE,
    SCENE_PHASE_TRAVEL = SCENE_PULSE_AGE,
    SCENE_PARTICLE_EMISSION = SCENE_PULSE_TEMPLATE,
    SCENE_PARTICLE_VARIANT = SCENE_PULSE_TEMPLATE + 1u,
    SCENE_PHASE_OBJECT = WRAM_WORLD_MAP_OBJECT_TABLE + SCENE_OBJECT_SIZE,
    SCENE_PHASE_MARKER = WRAM_WORLD_MAP_OBJECT_TABLE + 21u * SCENE_OBJECT_SIZE,
    SCENE_OBJECT_Y_FRACTION = 4u,
    DP_PHASE_WINDOW = 0x63u,
    DP_PHASE_ANIMATION = 2u,
    DP_PHASE_SIGN = 0u
};

static void AlignPhaseSceneObjects(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t horizontal[] = {
        WRAM_WORLD_MAP_OBJECT_TABLE + 2u * SCENE_OBJECT_SIZE + SCENE_OBJECT_X,
        WRAM_WORLD_MAP_OBJECT_TABLE + 3u * SCENE_OBJECT_SIZE + SCENE_OBJECT_X,
        WRAM_WORLD_MAP_OBJECT_TABLE + 4u * SCENE_OBJECT_SIZE + SCENE_OBJECT_X,
        WRAM_WORLD_MAP_OBJECT_TABLE + 5u * SCENE_OBJECT_SIZE + SCENE_OBJECT_X,
        SCENE_PHASE_MARKER + SCENE_OBJECT_X};
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCENE_PHASE_OBJECT + SCENE_OBJECT_X)));
    for (unsigned object = 0u; object < 5u; ++object)
        OpWrite16(memory, OpAbs(cpu, horizontal[object]), cpu->y);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCENE_PHASE_OBJECT + SCENE_OBJECT_Y)));
    OpWrite16(memory,
              OpAbs(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + 2u * SCENE_OBJECT_SIZE +
                             SCENE_OBJECT_Y),
              cpu->y);
    OpWrite16(memory,
              OpAbs(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + 3u * SCENE_OBJECT_SIZE +
                             SCENE_OBJECT_Y),
              cpu->y);
}

Lufia2ExecutionResult Lufia2WorldMapAlignSceneGroup(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86ba2cu);
    AlignPhaseSceneObjects(memory, cpu);
    return ExecutionReturned(0x86ba47u);
}

Lufia2ExecutionResult
Lufia2WorldMapStopSceneParticleEmission(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b954u);
    OpLdy(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PARTICLE_EMISSION), cpu->y);
    return ExecutionReturned(0x86b95au);
}

Lufia2ExecutionResult
Lufia2WorldMapStartScenePhaseAnimation(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b95bu);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86b968u)};
    OpLdx(cpu, SCENE_PHASE_OBJECT);
    OpLoadA(cpu, 0x14u);
    if (!CallMotionChild(&motion, 0x86b960u, 0x86e0b9u))
        return motion.result;
    OpLoadA(cpu, 0x6bu);
    if (!CallMotionChild(&motion, 0x86b965u, 0x86d378u))
        return motion.result;
    return motion.result;
}

Lufia2ExecutionResult Lufia2WorldMapStartSceneFall(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b969u);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86b97du)};
    OpLdx(cpu, SCENE_PHASE_OBJECT);
    OpLoadA(cpu, 0x15u);
    if (!CallMotionChild(&motion, 0x86b96eu, 0x86e0b9u))
        return motion.result;
    OpLdx(cpu, 0xb9a3u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PHASE_CALLBACK), cpu->x);
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PHASE_SPEED), cpu->x);
    return motion.result;
}

static void StepSceneDeparture(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, SCENE_PHASE_OBJECT);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PHASE_SPEED));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x80u);
    OpCmpValue(cpu, 0x400u);
    if (cpu->carry)
        OpLoadA(cpu, 0x300u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PHASE_SPEED));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SCENE_PHASE_TRAVEL));
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PHASE_TRAVEL));
    ExchangeAccumulatorBytes(cpu);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_PHASE_SIGN));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
    OpIncA(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, SCENE_PLAYER_X));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_X));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PHASE_SPEED));
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y_FRACTION));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y_FRACTION));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y + 1u));
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y + 1u));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, SCENE_PHASE_MARKER);
    OpLoadA(cpu, 0x80u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_PHASE_SIGN)));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 7u));
    OpLoadA(cpu, 0x70u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_PHASE_SIGN));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 9u));
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCENE_PHASE_OBJECT + SCENE_OBJECT_Y)));
    OpWrite16(memory,
              OpAbs(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + 4u * SCENE_OBJECT_SIZE +
                             SCENE_OBJECT_Y),
              cpu->y);
    OpWrite16(memory,
              OpAbs(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + 5u * SCENE_OBJECT_SIZE +
                             SCENE_OBJECT_Y),
              cpu->y);
    AlignPhaseSceneObjects(memory, cpu);
}

Lufia2ExecutionResult Lufia2WorldMapStepSceneDeparture(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86ba48u);
    StepSceneDeparture(memory, cpu);
    return ExecutionReturned(0x86ba47u);
}

Lufia2ExecutionResult Lufia2WorldMapStartSceneDeparture(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b97eu);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86ba47u)};
    OpLdx(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + 4u * SCENE_OBJECT_SIZE);
    OpLoadA(cpu, 0x19u);
    if (!CallMotionChild(&motion, 0x86b983u, 0x86baa8u))
        return motion.result;
    OpLdx(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + 5u * SCENE_OBJECT_SIZE);
    OpLoadA(cpu, 0x1au);
    if (!CallMotionChild(&motion, 0x86b98bu, 0x86baa8u))
        return motion.result;
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PHASE_SPEED), cpu->x);
    OpWrite16(memory, OpAbs(cpu, SCENE_PHASE_TRAVEL), cpu->x);
    OpLdy(cpu, 0xba48u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PHASE_CALLBACK), cpu->y);
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_PARTICLE_VARIANT), -1);
    StepSceneDeparture(memory, cpu);
    return motion.result;
}

Lufia2ExecutionResult Lufia2WorldMapStepSceneFall(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b9a3u);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86b9e4u)};
    OpLdx(cpu, SCENE_PHASE_OBJECT);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PHASE_SPEED));
    cpu->carry = 0u;
    OpAdcValue(cpu, 2u);
    OpCmpValue(cpu, 0x40u);
    if (cpu->carry)
        OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PHASE_SPEED));
    cpu->carry = 0u;
    OpAdc(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y_FRACTION));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y_FRACTION));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y + 1u));
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y + 1u));
    OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SCENE_PLAYER_Y)));
    OpCmpValue(cpu, 0x13u);
    if (!cpu->zero)
        return motion.result;
    OpLoadA(cpu, 0x16u);
    if (!CallMotionChild(&motion, 0x86b9d3u, 0x86e0b9u))
        return motion.result;
    OpLdy(cpu, 0xb9e5u);
    OpWrite16(memory, OpAbs(cpu, SCENE_PHASE_CALLBACK), cpu->y);
    OpLoadA(cpu, 0x53u);
    if (!CallMotionChild(&motion, 0x86b9deu, 0x86d378u))
        return motion.result;
    OpStepMem(memory, cpu, OpAbs(cpu, SCENE_PARTICLE_EMISSION), -1);
    return motion.result;
}

Lufia2ExecutionResult Lufia2WorldMapStepSceneRebound(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b9e5u);
    OpLdx(cpu, SCENE_PHASE_OBJECT);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, DP_PHASE_SIGN));
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PHASE_SPEED));
    cpu->carry = 1u;
    OpSbcValue(cpu, 8u);
    if (cpu->negative) {
        OpStepMem(memory, cpu, OpDp(cpu, DP_PHASE_SIGN), -1);
        OpCmpValue(cpu, 0xffc0u);
        if (!cpu->carry) {
            OpLdy(cpu, 0xba16u);
            OpWrite16(memory, OpAbs(cpu, SCENE_PHASE_CALLBACK), cpu->y);
            OpLoadA(cpu, 0xffc0u);
        }
    }
    OpSta(memory, cpu, OpAbs(cpu, SCENE_PHASE_SPEED));
    cpu->carry = 0u;
    OpAdc(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y_FRACTION));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y_FRACTION));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y + 1u));
    OpAdc(memory, cpu, OpDp(cpu, DP_PHASE_SIGN));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y + 1u));
    return ExecutionReturned(0x86ba15u);
}

Lufia2ExecutionResult Lufia2WorldMapSplitSceneGroup(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86ba16u);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86ba47u)};
    OpLdx(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + 2u * SCENE_OBJECT_SIZE);
    OpLoadA(cpu, 0x17u);
    if (!CallMotionChild(&motion, 0x86ba1bu, 0x86baa8u))
        return motion.result;
    OpLdx(cpu, WRAM_WORLD_MAP_OBJECT_TABLE + 3u * SCENE_OBJECT_SIZE);
    OpLoadA(cpu, 0x18u);
    if (!CallMotionChild(&motion, 0x86ba23u, 0x86baa8u))
        return motion.result;
    OpLdy(cpu, 0xba2cu);
    OpWrite16(memory, OpAbs(cpu, SCENE_PHASE_CALLBACK), cpu->y);
    AlignPhaseSceneObjects(memory, cpu);
    return motion.result;
}

Lufia2ExecutionResult
Lufia2WorldMapResetSceneObjectAnimation(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86baa8u);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86baadu)};
    OpStz(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_VARIANT));
    if (!CallMotionChild(&motion, 0x86baaau, 0x86e0b9u))
        return motion.result;
    return motion.result;
}

Lufia2ExecutionResult Lufia2WorldMapSpawnSceneParticle(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86baaeu);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86baebu)};
    OpPushX(memory, cpu);
    OpLdx(cpu, SCENE_PARTICLES);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 14u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    bool available;
    do {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y));
        available = cpu->negative;
        if (available)
            break;
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, SCENE_OBJECT_SIZE);
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->zero);
    if (available) {
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
        OpStz(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_VARIANT));
        OpLda(memory, cpu, OpDp(cpu, DP_PHASE_ANIMATION));
        if (!CallMotionChild(&motion, 0x86bae5u, 0x86e0b9u))
            return motion.result;
    }
    OpSepWidths(cpu, 0x20u);
    OpPullX(memory, cpu);
    return motion.result;
}

static Lufia2ExecutionResult LoadPhaseSceneWindow(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context, unsigned index) {
    static const uint16_t entries[] = {0xbaecu, 0xbafau, 0xbb01u, 0xbb08u, 0xbb0fu};
    static const uint16_t records[] = {0xbb16u, 0xbb24u, 0xbb32u, 0xbb40u, 0xbb4eu};
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x860000u | entries[index]);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86baf9u)};
    OpLdy(cpu, records[index]);
    OpLoadA(cpu, (uint16_t)index);
    if (!CallMotionChild(&motion, 0x86baf1u, 0x86c54bu))
        return motion.result;
    OpLoadA(cpu, 0x8eu);
    if (!CallMotionChild(&motion, 0x86baf6u, 0x86d378u))
        return motion.result;
    return motion.result;
}

Lufia2ExecutionResult Lufia2WorldMapLoadScenePhaseWindow0(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    return LoadPhaseSceneWindow(memory, cpu, child, context, 0u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadScenePhaseWindow1(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    return LoadPhaseSceneWindow(memory, cpu, child, context, 1u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadScenePhaseWindow2(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    return LoadPhaseSceneWindow(memory, cpu, child, context, 2u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadScenePhaseWindow3(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    return LoadPhaseSceneWindow(memory, cpu, child, context, 3u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadScenePhaseWindow4(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    return LoadPhaseSceneWindow(memory, cpu, child, context, 4u);
}

static void MovePhaseSceneParticles(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, SCENE_PHASE_OBJECT);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, SCENE_PLAYER_X)));
    OpWrite16(memory, WorldSceneObjectField(cpu, SCENE_OBJECT_X), cpu->y);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, SCENE_PARTICLES);
    OpLoadA(cpu, 14u);
    OpSta(memory, cpu, OpDp(cpu, DP_PARTICLE_COUNT));
    do {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y));
        if (!cpu->negative) {
            OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y_FRACTION));
            cpu->carry = 1u;
            OpSbcValue(cpu, 0x1c0u);
            OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y_FRACTION));
            OpSepWidths(cpu, 0x20u);
            OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y + 1u));
            OpSbcValue(cpu, 0u);
            OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y + 1u));
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_X));
            cpu->carry = 1u;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SCENE_PLAYER_X)));
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x80u);
            OpCmpValue(cpu, 0x108u);
            bool outside = cpu->carry;
            if (!outside) {
                OpLda(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y));
                cpu->carry = 1u;
                OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SCENE_PLAYER_Y)));
                cpu->carry = 0u;
                OpAdcValue(cpu, 0x70u);
                OpCmpValue(cpu, 0xe8u);
                outside = cpu->carry;
            }
            if (outside) {
                OpLoadA(cpu, 0xffffu);
                OpSta(memory, cpu, WorldSceneObjectField(cpu, SCENE_OBJECT_Y));
            }
        }
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, SCENE_OBJECT_SIZE);
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_PARTICLE_COUNT), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
}

static bool StepPhaseSceneWindow(SceneMotion *motion) {
    const Lufia2Memory *memory = motion->memory;
    Lufia2CpuState *cpu = motion->cpu;
    OpLda(memory, cpu, WorldSceneObjectField(cpu, 8u));
    if (!cpu->zero) {
        cpu->carry = 1u;
        OpSbcValue(cpu, 8u);
        if (!cpu->carry)
            OpLoadA(cpu, 0u);
        OpSta(memory, cpu, WorldSceneObjectField(cpu, 8u));
    } else {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, 13u));
        OpAndValue(cpu, 3u);
        if (cpu->zero) {
            OpLdy(cpu, OpRead16(memory, WorldSceneObjectField(cpu, 0u)));
            OpWrite16(memory, OpDp(cpu, DP_POSITION_X), cpu->y);
            OpLdy(cpu, OpRead16(memory, WorldSceneObjectField(cpu, 2u)));
            OpIny(cpu);
            OpIny(cpu);
            OpWrite16(memory, OpDp(cpu, DP_POSITION_Y), cpu->y);
            OpLoadA(cpu, 0x12u);
            OpSta(memory, cpu, OpDp(cpu, DP_PHASE_ANIMATION));
            if (!CallMotionChild(motion, 0x86b8deu, 0x86baaeu))
                return false;
        }
        OpStepMem(memory, cpu, WorldSceneObjectField(cpu, 13u), 1);
        OpLda(memory, cpu, WorldSceneObjectField(cpu, 13u));
        OpCmpValue(cpu, 20u);
        if (cpu->carry) {
            OpLda(memory, cpu, WorldSceneObjectField(cpu, 10u));
            cpu->carry = 1u;
            OpSbcValue(cpu, 8u);
            if (cpu->zero || !cpu->carry) {
                OpStz(memory, cpu, WorldSceneObjectField(cpu, 12u));
                OpLoadA(cpu, 0u);
            }
            OpSta(memory, cpu, WorldSceneObjectField(cpu, 10u));
        }
    }
    OpLda(memory, cpu, OpDp(cpu, DP_PHASE_WINDOW));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    return CallMotionChild(motion, 0x86b8fcu, 0x86c4e0u);
}

static bool FinishPhaseSceneFrame(SceneMotion *motion) {
    const Lufia2Memory *memory = motion->memory;
    Lufia2CpuState *cpu = motion->cpu;
    OpLda(memory, cpu, OpAbs(cpu, SCENE_PARTICLE_EMISSION));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, SCENE_PARTICLE_VARIANT));
        OpLsrA(cpu);
        OpLoadA(cpu, cpu->carry ? 0x1bu : 0x12u);
        OpSta(memory, cpu, OpDp(cpu, DP_PHASE_ANIMATION));
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_SCROLL_Y));
        OpAndValue(cpu, 3u);
        if (cpu->zero) {
            OpLdy(cpu, 0x82u);
            OpWrite16(memory, OpDp(cpu, DP_POSITION_X), cpu->y);
            OpLdy(cpu, 0x72u);
            OpWrite16(memory, OpDp(cpu, DP_POSITION_Y), cpu->y);
            if (!CallMotionChild(motion, 0x86b92cu, 0x86baaeu))
                return false;
        }
    }
    RetireSceneSprites(memory, cpu, 14u, 4u, DP_PARTICLE_COUNT);
    return true;
}

Lufia2ExecutionResult Lufia2WorldMapStepPhasedSceneFrame(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b857u);
    SceneMotion motion = {memory, cpu, child, context, ExecutionReturned(0x86b953u)};
    MovePhaseSceneParticles(memory, cpu);
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, DP_PHASE_WINDOW));
    do {
        OpLda(memory, cpu, OpDp(cpu, DP_PHASE_WINDOW));
        if (!CallMotionChild(&motion, 0x86b8b4u, 0x86c536u))
            return motion.result;
        OpLda(memory, cpu, WorldSceneObjectField(cpu, 12u));
        if (!cpu->zero && !StepPhaseSceneWindow(&motion))
            return motion.result;
        OpStepMem(memory, cpu, OpDp(cpu, DP_PHASE_WINDOW), -1);
    } while (!cpu->negative);
    OpLdx(cpu, 0xb909u);
    OpPushX(memory, cpu);
    const uint32_t target = 0x860000u | Read16Bank(memory, 0u, SCENE_PHASE_CALLBACK);
    if (!child(context, cpu, target, 0x86b907u, 2u)) {
        motion.result = ExecutionReturned(0x86b907u);
        motion.result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return motion.result;
    }
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b90au);
    if (!FinishPhaseSceneFrame(&motion))
        return motion.result;
    return motion.result;
}
