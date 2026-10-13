#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/joypad.h"
#include "lufia2/world_map.h"
#include "system/wram.h"

enum {
    WORLD_DISPLAY = 0x2100u,
    WORLD_INTERRUPTS = 0x4200u,
    WORLD_HDMA = 0x420cu,
    WORLD_MAP_MODE = 0x11deu,
    WORLD_PLANE_DIRTY = 0x11ddu,
    WORLD_RESOURCE_DIRTY = 0x11e2u,
    WORLD_POSITION_X = 0x11e8u,
    WORLD_POSITION_Y = 0x11eau,
    WORLD_PLANE_XX = 0x1707u,
    WORLD_PLANE_XY = 0x1709u,
    WORLD_PLANE_YX = 0x170bu,
    WORLD_PLANE_YY = 0x170du,
    DP_WORLD_ROW_X = 0x58u,
    DP_WORLD_ROW_Y = 0x5au
};

static uint8_t WorldSceneEntry(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x86u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult WorldSceneUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void ResetWorldPlane(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x100u);
    OpWrite16(memory, OpAbs(cpu, WORLD_PLANE_XX), cpu->x);
    OpWrite16(memory, OpAbs(cpu, WORLD_PLANE_YY), cpu->x);
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, WORLD_PLANE_XY), cpu->x);
    OpWrite16(memory, OpAbs(cpu, WORLD_PLANE_YX), cpu->x);
}

Lufia2ExecutionResult Lufia2WorldMapBlankDisplay(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x86d3a5u);
    OpLoadA(cpu, 0x8fu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    OpSta(memory, cpu, OpAbs(cpu, WORLD_DISPLAY));
    return ExecutionReturned(0x86d3adu);
}

Lufia2ExecutionResult Lufia2WorldMapResetSceneState(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!WorldSceneEntry(cpu))
        return ExecutionHandoff(cpu, 0x86d2d6u);
    static const uint8_t cleared_scratch[] = {0x6au, 0x72u, 0x73u, 0x74u};
    static const uint16_t cleared_state[] = {
        0x11d8u, 0x1365u, 0x1710u, 0x1711u, 0x11dfu, 0x11e1u
    };
    for (unsigned field = 0; field < sizeof(cleared_scratch); ++field)
        OpStz(memory, cpu, OpDp(cpu, cleared_scratch[field]));
    for (unsigned field = 0; field < sizeof(cleared_state) / sizeof(uint16_t); ++field)
        OpStz(memory, cpu, OpAbs(cpu, cleared_state[field]));
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, 0x1702u), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, 0x16e7u));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    OpLdx(cpu, 0x1e2fu);
    OpWrite16(memory, OpAbs(cpu, 0x1716u), cpu->x);
    OpStz(memory, cpu, (uint16_t)(cpu->direct_page + cpu->x) | OP_DP_WRAP);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x170fu));
    OpLoadA(cpu, 0x62u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2101u));
    OpLoadA(cpu, 0xe0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2132u));
    OpLdy(cpu, 0xff00u);
    StoreYIndex(memory, cpu, 0x2126u);
    StoreYIndex(memory, cpu, 0x2128u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0x71u));
    OpSta(memory, cpu, OpAbs(cpu, 0x11dau));
    OpLdx(cpu, 0xcef6u);
    OpWrite16(memory, OpDp(cpu, 0x68u), cpu->x);
    OpLoadA(cpu, 0x86u);
    OpSta(memory, cpu, OpDp(cpu, 0x6au));
    return ExecutionReturned(0x86d32cu);
}

Lufia2ExecutionResult Lufia2WorldMapConfigureMode7(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!WorldSceneEntry(cpu))
        return ExecutionHandoff(cpu, 0x86ae1eu);
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2105u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x212du));
    OpLoadA(cpu, 0x11u);
    OpSta(memory, cpu, OpAbs(cpu, 0x212cu));
    OpStz(memory, cpu, OpAbs(cpu, 0x211au));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2130u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2131u));
    ResetWorldPlane(memory, cpu);
    return ExecutionReturned(0x86ae4cu);
}

Lufia2ExecutionResult Lufia2WorldMapUpdatePlane(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!WorldSceneEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x86a7ceu);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_MAP_MODE));
    if (!cpu->negative) {
        OpDecA(cpu);
        if (!cpu->zero) {
            ResetWorldPlane(memory, cpu);
            return ExecutionReturned(0x86a7e8u);
        }
        OpStz(memory, cpu, OpAbs(cpu, 0x1201u));
    }
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86a7ecu, 0x86a894u, 2u, 0x86u))
        return WorldSceneUnwound(0x86a7ecu);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_PLANE_DIRTY));
    if (cpu->zero && !CallChildWithFrame(memory, cpu, child, context,
            0x86a7f4u, 0x86a7f8u, 2u, 0x86u))
        return WorldSceneUnwound(0x86a7f4u);
    return ExecutionReturned(0x86a7f7u);
}

Lufia2ExecutionResult Lufia2WorldMapInstallGraphics(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!WorldSceneEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x86cd41u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86cd41u, 0x86d3a5u, 2u, 0x86u))
        return WorldSceneUnwound(0x86cd41u);
    OpStz(memory, cpu, OpAbs(cpu, WORLD_INTERRUPTS));
    OpStz(memory, cpu, OpAbs(cpu, WORLD_HDMA));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86cd4au, 0x86cdf5u, 2u, 0x86u))
        return WorldSceneUnwound(0x86cd4au);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_RESOURCE_DIRTY));
    if (cpu->zero && !CallChildWithFrame(memory, cpu, child, context,
            0x86cd52u, 0x86cd67u, 2u, 0x86u))
        return WorldSceneUnwound(0x86cd52u);
    OpStz(memory, cpu, OpAbs(cpu, WORLD_RESOURCE_DIRTY));
    static const uint32_t sites[] = {0x86cd58u, 0x86cd5bu, 0x86cd5eu};
    static const uint32_t targets[] = {0x86cd91u, 0x86cbf0u, 0x86cbbfu};
    for (unsigned phase = 0; phase < 3u; ++phase)
        if (!CallChildWithFrame(memory, cpu, child, context,
                sites[phase], targets[phase], 2u, 0x86u))
            return WorldSceneUnwound(sites[phase]);
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_INTERRUPTS));
    return ExecutionReturned(0x86cd66u);
}

Lufia2ExecutionResult Lufia2WorldMapResetScene(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!WorldSceneEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x8692a1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8692a1u, 0x86d3a5u, 2u, 0x86u))
        return WorldSceneUnwound(0x8692a1u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8692a4u, 0x86d2d6u, 2u, 0x86u))
        return WorldSceneUnwound(0x8692a4u);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x11e6u));
    OpStz(memory, cpu, OpAbs(cpu, 0x11ecu));
    OpStz(memory, cpu, OpAbs(cpu, 0x11efu));
    static const uint32_t setup_sites[] = {0x8692b2u, 0x8692b5u, 0x8692b8u};
    static const uint32_t setup_targets[] = {0x86cd41u, 0x86ae1eu, 0x86ad82u};
    for (unsigned phase = 0; phase < 3u; ++phase)
        if (!CallChildWithFrame(memory, cpu, child, context,
                setup_sites[phase], setup_targets[phase], 2u, 0x86u))
            return WorldSceneUnwound(setup_sites[phase]);
    OpLdy(cpu, 0u);
    StoreYIndex(memory, cpu, 0x1247u);
    StoreYIndex(memory, cpu, 0x1249u);
    static const uint32_t frame_sites[] = {
        0x8692c4u, 0x8692c7u, 0x8692cau, 0x8692cdu, 0x8692d0u
    };
    static const uint32_t frame_targets[] = {
        0x86995bu, 0x86a7ceu, 0x86a791u, 0x86e6c4u, 0x86e617u
    };
    for (unsigned phase = 0; phase < 5u; ++phase)
        if (!CallChildWithFrame(memory, cpu, child, context,
                frame_sites[phase], frame_targets[phase], 2u, 0x86u))
            return WorldSceneUnwound(frame_sites[phase]);
    OpStz(memory, cpu, OpAbs(cpu, 0x123fu));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, 0x11e7u));
    OpSta(memory, cpu, OpAbs(cpu, 0x11e5u));
    return ExecutionReturned(0x8692deu);
}

Lufia2ExecutionResult Lufia2WorldMapUploadTilePlane(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x86u || cpu->index_is_8_bit || cpu->decimal ||
        cpu->direct_page || cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x86ad82u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_POSITION_X));
    for (unsigned shift = 0; shift < 4u; ++shift)
        OpLsrA(cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_WORLD_ROW_X));
    OpLda(memory, cpu, OpAbs(cpu, WORLD_POSITION_Y));
    for (unsigned shift = 0; shift < 4u; ++shift)
        OpLsrA(cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_WORLD_ROW_Y));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, DP_WORLD_ROW_X + 1u));
    OpStz(memory, cpu, OpDp(cpu, DP_WORLD_ROW_Y + 1u));
    OpLoadA(cpu, 0x40u);
    do {
        if (cpu->accumulator_is_8_bit)
            PushAccumulator8(memory, cpu);
        else
            PushAccumulator16(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x86ada7u, 0x86ac6cu, 2u, 0x86u))
            return WorldSceneUnwound(0x86ada7u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_WORLD_ROW_Y), 1);
        if (cpu->accumulator_is_8_bit)
            OpLoadA(cpu, Pull8(memory, cpu));
        else
            PullAccumulator16(memory, cpu);
        OpDecA(cpu);
    } while (!cpu->zero);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86adb0u, 0x86d3a5u, 2u, 0x86u))
        return WorldSceneUnwound(0x86adb0u);
    OpStz(memory, cpu, OpAbs(cpu, WORLD_INTERRUPTS));
    OpStz(memory, cpu, OpAbs(cpu, WORLD_HDMA));
    OpStz(memory, cpu, OpAbs(cpu, 0x2115u));
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, 0x2116u), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, 0x4370u));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, 0x4371u));
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpAbs(cpu, 0x4372u), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, 0x4374u));
    OpLdx(cpu, 0x4000u);
    OpWrite16(memory, OpAbs(cpu, 0x4375u), cpu->x);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, 0x420bu));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2115u));
    OpLoadA(cpu, 0x81u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_INTERRUPTS));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86adeau, 0x869a44u, 2u, 0x86u))
        return WorldSceneUnwound(0x86adeau);
    return ExecutionReturned(0x86adedu);
}

enum {
    WORLD_ACTIVE_OBJECT = 0x11e7u,
    WORLD_VIEW_REQUEST = 0x11e4u,
    WORLD_TARGET_MODE = 0x09dfu,
    WORLD_TARGET_STATE = 0x09e0u,
    WORLD_TARGET_PENDING = 0x09e1u,
    WORLD_TARGET_POSE = 0x09e2u,
    WORLD_SCENE_ACTIVE = 0x123fu,
    WORLD_SCENE_ANIMATION = 0x1240u,
    WORLD_SCENE_CONTROL = 0x1241u,
    WORLD_SCENE_PHASE = 0x1243u,
    WORLD_PLANE_ORIGIN = 0x16cfu,
    WORLD_PLANE_OFFSET = 0x16d3u,
    WORLD_DISPLAY_REQUEST = 0x16e7u,
    OBJECT_POSE = 0x0fu,
    OBJECT_ANIMATION = 0x10u,
    OBJECT_CONTROL = 0x18u,
    INPUT_FIRST = 0x46u,
    INPUT_SECOND = 0x47u,
    INPUT_EDGE = 0x4bu,
    SCENE_FRAME_BOUNDARY = 4096u
};

static uint8_t SceneCallerEntry(const Lufia2CpuState *cpu,
                                Lufia2PushedChildCall child) {
    return WorldSceneEntry(cpu) && cpu->data_bank == 0x86u &&
           !cpu->direct_page && child;
}

static uint8_t SceneChild(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          Lufia2PushedChildCall child, void *context,
                          uint32_t site, uint32_t target,
                          Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, 2u, 0x86u)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!WorldSceneEntry(cpu)) {
        *result = ExecutionHandoff(cpu, site + 3u);
        return 0u;
    }
    return 1u;
}

static uint32_t SceneObjectField(const Lufia2CpuState *cpu, uint8_t offset) {
    return DirectIndexedAddress(cpu, offset, cpu->x) | OP_DP_WRAP;
}

static void SaveSceneView(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t fields[][2] = {
        {0x1202u, 0x1210u}, {0x1204u, 0x1212u}, {0x11fcu, 0x1214u},
        {0x11feu, 0x1216u}, {0x1200u, 0x1218u}
    };
    for (unsigned field = 0u; field < sizeof(fields) / sizeof(fields[0]); ++field) {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, fields[field][0])));
        OpWriteX(memory, cpu, OpAbs(cpu, fields[field][1]), cpu->y);
    }
}

static void RestoreScenePalette(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint16_t source) {
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0xa6u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0x20u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, source));
        OpSta(memory, cpu, OpAbsX(cpu, 0x0320u));
        OpInx(cpu);
        OpInx(cpu);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

static Lufia2ExecutionResult RunWorldScene(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu,
                                          Lufia2PushedChildCall child,
                                          void *context, uint8_t animation) {
    const uint32_t entry = animation == 3u ? 0x869f55u : 0x869f5cu;
    if (!SceneCallerEntry(cpu, child))
        return ExecutionHandoff(cpu, entry);
    Lufia2ExecutionResult result;
    OpLoadA(cpu, animation);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_SCENE_ANIMATION));
    if (animation != 3u)
        OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_SCENE_CONTROL));
    OpLda(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
    if (!SceneChild(memory, cpu, child, context, 0x869f69u, 0x86e709u, &result))
        return result;
    OpLda(memory, cpu, SceneObjectField(cpu, OBJECT_POSE));
    OpOraValue(cpu, 1u);
    OpSta(memory, cpu, SceneObjectField(cpu, OBJECT_POSE));
    OpLda(memory, cpu, OpAbs(cpu, WORLD_SCENE_ANIMATION));
    if (!SceneChild(memory, cpu, child, context, 0x869f75u, 0x86e0b9u, &result))
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WORLD_SCENE_CONTROL));
    OpSta(memory, cpu, SceneObjectField(cpu, OBJECT_CONTROL));
    unsigned frames = 0u;
    for (;;) {
        if (frames++ == SCENE_FRAME_BOUNDARY)
            return ExecutionHandoff(cpu, 0x869f7du);
        if (!SceneChild(memory, cpu, child, context, 0x869f7du, 0x86e11fu, &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
        if (!SceneChild(memory, cpu, child, context, 0x869f83u, 0x86e709u, &result))
            return result;
        OpLda(memory, cpu, SceneObjectField(cpu, OBJECT_ANIMATION));
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            break;
        if (!SceneChild(memory, cpu, child, context, 0x869f8cu, 0x86e1b9u, &result) ||
            !SceneChild(memory, cpu, child, context, 0x869f8fu, 0x86973eu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x869f92u, 0x86974fu, &result))
            return result;
    }
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_MAP_MODE));
    SaveSceneView(memory, cpu);
    OpLdx(cpu, 0xdc59u);
    OpLdy(cpu, 0u);
    if (!SceneChild(memory, cpu, child, context, 0x869fc0u, 0x869470u, &result) ||
        !SceneChild(memory, cpu, child, context, 0x869fc3u, 0x8694d4u, &result))
        return result;
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_SCENE_PHASE));
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, WORLD_PLANE_ORIGIN), cpu->y);
    frames = 0u;
    for (;;) {
        if (frames++ == SCENE_FRAME_BOUNDARY)
            return ExecutionHandoff(cpu, 0x869fd1u);
        if (!SceneChild(memory, cpu, child, context, 0x869fd1u, 0x8694d4u, &result))
            return result;
        if (cpu->carry)
            break;
        OpLoadA(cpu, 0u);
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbs(cpu, WORLD_SCENE_PHASE));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0xdc3bu));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x70u);
        OpSta(memory, cpu, OpAbs(cpu, WORLD_PLANE_OFFSET));
        OpStepMem(memory, cpu, OpAbs(cpu, WORLD_SCENE_PHASE), 1);
        if (!SceneChild(memory, cpu, child, context, 0x869fe9u, 0x86e11fu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x869fecu, 0x86a7ceu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x869fefu, 0x86a791u, &result) ||
            !SceneChild(memory, cpu, child, context, 0x869ff2u, 0x86e1b9u, &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, 0x1217u));
        OpCmpValue(cpu, 0x38u);
        if (!cpu->carry) {
            OpLdy(cpu, 0xb400u);
            OpLoadA(cpu, 0xa6u);
            if (!SceneChild(memory, cpu, child, context, 0x86a001u, 0x86a03bu, &result))
                return result;
        }
        if (!SceneChild(memory, cpu, child, context, 0x86a004u, 0x86973eu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a007u, 0x86974fu, &result))
            return result;
    }
    RestoreScenePalette(memory, cpu, 0xb400u);
    if (!SceneChild(memory, cpu, child, context, 0x86a024u, 0x86a0a2u, &result))
        return result;
    OpStz(memory, cpu, OpAbs(cpu, WORLD_SCENE_ACTIVE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_TARGET_PENDING));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_TARGET_STATE));
    OpLdx(cpu, 0x0800u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1204u), cpu->x);
    return ExecutionReturned(0x86a03au);
}

Lufia2ExecutionResult Lufia2WorldMapRunScene03(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    return RunWorldScene(memory, cpu, child, context, 3u);
}

Lufia2ExecutionResult Lufia2WorldMapRunScene06(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    return RunWorldScene(memory, cpu, child, context, 6u);
}

Lufia2ExecutionResult Lufia2WorldMapHandleSceneInput(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!SceneCallerEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x8696bcu);
    Lufia2ExecutionResult result;
    OpLda(memory, cpu, OpAbs(cpu, WORLD_POSITION_X));
    OpAndValue(cpu, 0x0fu);
    OpCmpValue(cpu, 8u);
    if (!cpu->zero)
        return ExecutionReturned(0x8696ceu);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_POSITION_Y));
    OpAndValue(cpu, 0x0fu);
    OpCmpValue(cpu, 8u);
    if (!cpu->zero)
        return ExecutionReturned(0x8696ceu);
    OpLda(memory, cpu, OpDp(cpu, INPUT_FIRST));
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, INPUT_SECOND));
        OpBitValue(cpu, 0x80u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WORLD_TARGET_PENDING));
            if (cpu->zero) {
                OpLoadA(cpu, 2u);
                OpSta(memory, cpu, OpAbs(cpu, WORLD_TARGET_MODE));
                OpSta(memory, cpu, OpAbs(cpu, WORLD_TARGET_POSE));
                OpSta(memory, cpu, OpAbs(cpu, WORLD_TARGET_STATE));
                if (!SceneChild(memory, cpu, child, context, 0x8696ebu, 0x86a1c5u, &result) ||
                    !SceneChild(memory, cpu, child, context, 0x8696eeu, 0x869f55u, &result))
                    return result;
            }
        }
    }
    OpLda(memory, cpu, OpDp(cpu, INPUT_SECOND));
    OpBitValue(cpu, 0x20u);
    if (!cpu->zero) {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, WORLD_VIEW_REQUEST));
    }
    OpLda(memory, cpu, OpDp(cpu, INPUT_SECOND));
    OpBitValue(cpu, 0x10u);
    if (!cpu->zero) {
        OpTestBits(memory, cpu, OpDp(cpu, INPUT_EDGE), 0u);
        if (!cpu->zero) {
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpAbs(cpu, 0x212cu));
            OpStz(memory, cpu, OpAbs(cpu, WORLD_DISPLAY_REQUEST));
            if (!SceneChild(memory, cpu, child, context, 0x86970eu, 0x86cbbfu, &result))
                return result;
            OpLoadA(cpu, 0x81u);
            OpSta(memory, cpu, OpAbs(cpu, 0x4200u));
            if (!SceneChild(memory, cpu, child, context, 0x869716u, 0x86d3aeu, &result))
                return result;
            unsigned frames = 0u;
            for (;;) {
                if (frames++ == SCENE_FRAME_BOUNDARY)
                    return ExecutionHandoff(cpu, 0x869719u);
                if (!SceneChild(memory, cpu, child, context, 0x869719u, 0x86974fu, &result))
                    return result;
                OpLda(memory, cpu, OpDp(cpu, INPUT_SECOND));
                OpBitValue(cpu, 0x10u);
                if (cpu->zero)
                    continue;
                OpTestBits(memory, cpu, OpDp(cpu, INPUT_EDGE), 0u);
                if (!cpu->zero)
                    break;
            }
            if (!SceneChild(memory, cpu, child, context, 0x869726u, 0x86cbf0u, &result))
                return result;
            OpLoadA(cpu, 0x11u);
            OpSta(memory, cpu, OpAbs(cpu, 0x212cu));
        }
    }
    OpLda(memory, cpu, OpAbs(cpu, WORLD_MAP_MODE));
    const uint32_t site = cpu->zero ? 0x869738u : 0x869733u;
    const uint32_t target = cpu->zero ? 0x86973cu : 0x86973du;
    if (!SceneChild(memory, cpu, child, context, site, target, &result))
        return result;
    return ExecutionReturned(0x86973bu);
}

enum {
    VEHICLE_SCENE_DEPTH = 0x11fcu,
    VEHICLE_SCENE_TARGET_DEPTH = 0x0100u,
    VEHICLE_SCENE_DEPTH_INCREASE = 0x0010u,
    VEHICLE_SCENE_DEPTH_DECREASE = 0x0020u,
    VEHICLE_SCENE_PALETTE = 0xb200u
};

static uint8_t StepVehicleSceneDepth(const Lufia2Memory *memory,
                                    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, VEHICLE_SCENE_DEPTH));
    OpCmpValue(cpu, VEHICLE_SCENE_TARGET_DEPTH);
    if (cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        return 1u;
    }
    if (cpu->carry) {
        OpSbcValue(cpu, VEHICLE_SCENE_DEPTH_DECREASE);
        OpCmpValue(cpu, VEHICLE_SCENE_TARGET_DEPTH);
        if (!cpu->carry)
            OpLoadA(cpu, VEHICLE_SCENE_TARGET_DEPTH);
    } else {
        OpAdcValue(cpu, VEHICLE_SCENE_DEPTH_INCREASE);
        OpCmpValue(cpu, VEHICLE_SCENE_TARGET_DEPTH);
        if (cpu->carry)
            OpLoadA(cpu, VEHICLE_SCENE_TARGET_DEPTH);
    }
    OpSta(memory, cpu, OpAbs(cpu, VEHICLE_SCENE_DEPTH));
    OpSepWidths(cpu, 0x20u);
    return 0u;
}

static uint8_t VehicleSceneFrame(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 Lufia2PushedChildCall child, void *context,
                                 uint8_t palette,
                                 Lufia2ExecutionResult *result) {
    if (!SceneChild(memory, cpu, child, context,
                    palette ? 0x86a14fu : 0x86a0ffu, 0x86e11fu, result) ||
        !SceneChild(memory, cpu, child, context,
                    palette ? 0x86a152u : 0x86a102u, 0x86a7ceu, result) ||
        !SceneChild(memory, cpu, child, context,
                    palette ? 0x86a155u : 0x86a105u, 0x86a791u, result) ||
        !SceneChild(memory, cpu, child, context,
                    palette ? 0x86a158u : 0x86a108u, 0x86e1b9u, result))
        return 0u;
    if (palette) {
        OpLdy(cpu, VEHICLE_SCENE_PALETTE);
        OpLoadA(cpu, 0xa6u);
        if (!SceneChild(memory, cpu, child, context, 0x86a160u, 0x86a03bu, result))
            return 0u;
    }
    return SceneChild(memory, cpu, child, context,
                       palette ? 0x86a163u : 0x86a10bu, 0x86973eu, result) &&
           SceneChild(memory, cpu, child, context,
                       palette ? 0x86a166u : 0x86a10eu, 0x86974fu, result);
}

static Lufia2ExecutionResult RunVehicleScene(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu,
                                           Lufia2PushedChildCall child,
                                           void *context, uint8_t animation) {
    const uint32_t entry = animation == 3u ? 0x86a0b9u : 0x86a0c6u;
    if (!SceneCallerEntry(cpu, child))
        return ExecutionHandoff(cpu, entry);
    Lufia2ExecutionResult result;
    OpLdx(cpu, 0x0200u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1204u), cpu->x);
    if (animation == 7u)
        OpStz(memory, cpu, OpAbs(cpu, WORLD_TARGET_STATE));
    OpLoadA(cpu, animation);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_SCENE_ANIMATION));
    if (animation == 7u)
        OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_SCENE_CONTROL));
    unsigned frames = 0u;
    for (;;) {
        if (frames++ == SCENE_FRAME_BOUNDARY)
            return ExecutionHandoff(cpu, 0x86a0d9u);
        if (StepVehicleSceneDepth(memory, cpu))
            break;
        if (!VehicleSceneFrame(memory, cpu, child, context, 0u, &result))
            return result;
    }
    OpLda(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
    if (!SceneChild(memory, cpu, child, context, 0x86a118u, 0x86e709u, &result))
        return result;
    OpLda(memory, cpu, SceneObjectField(cpu, OBJECT_POSE));
    OpOraValue(cpu, 1u);
    OpSta(memory, cpu, SceneObjectField(cpu, OBJECT_POSE));
    OpLda(memory, cpu, SceneObjectField(cpu, OBJECT_ANIMATION));
    if (!SceneChild(memory, cpu, child, context, 0x86a123u, 0x86e0b9u, &result))
        return result;
    OpLdx(cpu, 0xdc6du);
    OpLdy(cpu, 0u);
    if (!SceneChild(memory, cpu, child, context, 0x86a12cu, 0x869470u, &result) ||
        !SceneChild(memory, cpu, child, context, 0x86a12fu, 0x8694d4u, &result))
        return result;
    OpLoadA(cpu, 0x17u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_SCENE_PHASE));
    frames = 0u;
    for (;;) {
        if (frames++ == SCENE_FRAME_BOUNDARY)
            return ExecutionHandoff(cpu, 0x86a137u);
        if (!SceneChild(memory, cpu, child, context, 0x86a137u, 0x8694d4u, &result))
            return result;
        if (cpu->carry)
            break;
        OpStepMem(memory, cpu, OpAbs(cpu, WORLD_SCENE_PHASE), -1);
        OpLoadA(cpu, 0u);
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbs(cpu, WORLD_SCENE_PHASE));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0xdc3bu));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x70u);
        OpSta(memory, cpu, OpAbs(cpu, WORLD_PLANE_OFFSET));
        if (!VehicleSceneFrame(memory, cpu, child, context, 1u, &result))
            return result;
    }
    RestoreScenePalette(memory, cpu, VEHICLE_SCENE_PALETTE);
    if (!SceneChild(memory, cpu, child, context, 0x86a183u, 0x86a0a2u, &result))
        return result;
    OpStz(memory, cpu, OpAbs(cpu, WORLD_MAP_MODE));
    OpLda(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
    if (!SceneChild(memory, cpu, child, context, 0x86a18cu, 0x86e709u, &result))
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WORLD_SCENE_ANIMATION));
    if (!SceneChild(memory, cpu, child, context, 0x86a192u, 0x86e0b9u, &result))
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WORLD_SCENE_CONTROL));
    OpSta(memory, cpu, SceneObjectField(cpu, OBJECT_CONTROL));
    OpLdy(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu, WORLD_PLANE_ORIGIN), cpu->y);
    frames = 0u;
    for (;;) {
        if (frames++ == SCENE_FRAME_BOUNDARY)
            return ExecutionHandoff(cpu, 0x86a1a0u);
        if (!SceneChild(memory, cpu, child, context, 0x86a1a0u, 0x86e11fu, &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
        if (!SceneChild(memory, cpu, child, context, 0x86a1a6u, 0x86e709u, &result))
            return result;
        OpLda(memory, cpu, SceneObjectField(cpu, OBJECT_ANIMATION));
        OpCmpValue(cpu, 7u);
        if (!cpu->zero)
            break;
        if (!SceneChild(memory, cpu, child, context, 0x86a1afu, 0x86e1b9u, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a1b2u, 0x86973eu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a1b5u, 0x86974fu, &result))
            return result;
    }
    OpLda(memory, cpu, OpAbs(cpu, WORLD_SCENE_CONTROL));
    OpCmpValue(cpu, 3u);
    if (cpu->zero &&
        !SceneChild(memory, cpu, child, context, 0x86a1c1u, 0x86a1ecu, &result))
        return result;
    return ExecutionReturned(0x86a1c4u);
}

Lufia2ExecutionResult Lufia2WorldMapRunVehicleScene03(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return RunVehicleScene(memory, cpu, child, context, 3u);
}

Lufia2ExecutionResult Lufia2WorldMapRunVehicleScene07(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return RunVehicleScene(memory, cpu, child, context, 7u);
}

Lufia2ExecutionResult Lufia2WorldMapWaitFade(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *context) {
    if (!SceneCallerEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86d397u);
    Lufia2ExecutionResult result;
    for (unsigned frames = 0u; frames < SCENE_FRAME_BOUNDARY; ++frames) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
        if (cpu->zero)
            return ExecutionReturned(0x86d3a4u);
        if (!SceneChild(memory, cpu, child, context, 0x86d39cu, 0x86973eu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86d39fu, 0x86974fu, &result))
            return result;
    }
    return ExecutionHandoff(cpu, 0x86d397u);
}

Lufia2ExecutionResult Lufia2WorldMapSelectSecondaryView(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SceneCallerEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86a3a2u);
    Lufia2ExecutionResult result;
    if (!SceneChild(memory, cpu, child, context, 0x86a3a2u, 0x86a38cu, &result))
        return result;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_TARGET_STATE));
    return ExecutionReturned(0x86a3aau);
}

Lufia2ExecutionResult Lufia2WorldMapRunScene05(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (!SceneCallerEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86a3abu);
    Lufia2ExecutionResult result;
    OpLda(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
    if (!SceneChild(memory, cpu, child, context, 0x86a3aeu, 0x86e709u, &result))
        return result;
    OpLda(memory, cpu, SceneObjectField(cpu, OBJECT_POSE));
    OpOraValue(cpu, 1u);
    OpSta(memory, cpu, SceneObjectField(cpu, OBJECT_POSE));
    OpLoadA(cpu, 5u);
    if (!SceneChild(memory, cpu, child, context, 0x86a3b9u, 0x86e0b9u, &result))
        return result;
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, SceneObjectField(cpu, OBJECT_CONTROL));
    for (unsigned frames = 0u; frames < SCENE_FRAME_BOUNDARY; ++frames) {
        if (!SceneChild(memory, cpu, child, context, 0x86a3c0u, 0x86e11fu, &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WORLD_ACTIVE_OBJECT));
        if (!SceneChild(memory, cpu, child, context, 0x86a3c6u, 0x86e709u, &result))
            return result;
        OpLda(memory, cpu, SceneObjectField(cpu, OBJECT_ANIMATION));
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpStz(memory, cpu, OpAbs(cpu, WORLD_TARGET_STATE));
            return ExecutionReturned(0x86a3e3u);
        }
        if (!SceneChild(memory, cpu, child, context, 0x86a3cfu, 0x86a7ceu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a3d2u, 0x86a791u, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a3d5u, 0x86e1b9u, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a3d8u, 0x86973eu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a3dbu, 0x86974fu, &result))
            return result;
    }
    return ExecutionHandoff(cpu, 0x86a3c0u);
}

Lufia2ExecutionResult Lufia2WorldMapCallScene03(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    if (!SceneCallerEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86a3e4u);
    Lufia2ExecutionResult result;
    if (!SceneChild(memory, cpu, child, context, 0x86a3e4u, 0x869f55u, &result))
        return result;
    return ExecutionReturned(0x86a3e7u);
}

enum {
    TILE_SCENE_VEHICLE_CLASS = 0x30u,
    TILE_SCENE_DP_X = 0x58u,
    TILE_SCENE_DP_Y = 0x5au,
    PAIRED_INPUT_FIRST_Y = WRAM_WORLD_MAP_OBJECT_TABLE + 19u * 0x1du + 5u,
    PAIRED_INPUT_SECOND_Y = WRAM_WORLD_MAP_OBJECT_TABLE + 20u * 0x1du + 5u
};

static void ReadSceneTileCoordinates(const Lufia2Memory *memory,
                                     Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_POSITION_X));
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, TILE_SCENE_DP_X));
    OpLda(memory, cpu, OpAbs(cpu, WORLD_POSITION_Y));
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, TILE_SCENE_DP_Y));
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2WorldMapRunTileScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SceneCallerEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86a3e8u);
    Lufia2ExecutionResult result;
    if (!SceneChild(memory, cpu, child, context, 0x86a3e8u, 0x86a376u, &result))
        return result;
    ReadSceneTileCoordinates(memory, cpu);
    if (!SceneChild(memory, cpu, child, context, 0x86a401u, 0x869a71u, &result))
        return result;
    OpCmpValue(cpu, TILE_SCENE_VEHICLE_CLASS);
    if (cpu->zero) {
        if (!SceneChild(memory, cpu, child, context, 0x86a40cu, 0x86a0c6u, &result))
            return result;
        return ExecutionReturned(0x86a40fu);
    }
    if (!SceneChild(memory, cpu, child, context, 0x86a408u, 0x86a0b9u, &result))
        return result;
    return ExecutionReturned(0x86a40bu);
}

static void ReadPairedInputDirection(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_BUTTONS_HELD + JOYPAD_HIGH_BYTE));
    OpBitValue(cpu, JOY_HIGH_UP);
    if (!cpu->zero) {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PAIR_PHASE));
        OpLda(memory, cpu, OpDp(cpu, DP_BUTTONS_HELD + JOYPAD_HIGH_BYTE));
    }
    OpBitValue(cpu, JOY_HIGH_DOWN);
    if (!cpu->zero) {
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PAIR_PHASE));
    }
}

static uint8_t AcceptPairedInput(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PAIR_PHASE));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_BUTTONS_HELD + JOYPAD_LOW_BYTE));
        OpBitValue(cpu, JOY_LOW_A);
        if (!cpu->zero) {
            OpTestBits(memory, cpu, OpDp(cpu, DP_BUTTONS_PRESSED + JOYPAD_LOW_BYTE), 0u);
            if (!cpu->zero)
                return 1u;
        }
    }
    OpLda(memory, cpu, OpDp(cpu, DP_BUTTONS_HELD + JOYPAD_HIGH_BYTE));
    OpBitValue(cpu, JOY_HIGH_B);
    if (cpu->zero)
        return 0u;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PAIR_PHASE));
    return 1u;
}

Lufia2ExecutionResult Lufia2WorldMapRunPairedObjectInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SceneCallerEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86a2e7u);
    Lufia2ExecutionResult result;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PAIR_PHASE));
    if (!SceneChild(memory, cpu, child, context, 0x86a2eau, 0x86a338u, &result))
        return result;
    OpLoadA(cpu, JOY_LOW_A);
    OpTestBits(memory, cpu, OpDp(cpu, DP_BUTTONS_PRESSED + JOYPAD_LOW_BYTE), 0u);
    for (unsigned frame = 0u; frame < SCENE_FRAME_BOUNDARY; ++frame) {
        ReadPairedInputDirection(memory, cpu);
        if (!SceneChild(memory, cpu, child, context, 0x86a307u, 0x86a338u, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a30au, 0x86e11fu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a30du, 0x86e1b9u, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a310u, 0x86973eu, &result) ||
            !SceneChild(memory, cpu, child, context, 0x86a313u, 0x86974fu, &result))
            return result;
        if (AcceptPairedInput(memory, cpu)) {
            OpLdy(cpu, 0xffffu);
            OpWrite16(memory, OpAbs(cpu, PAIRED_INPUT_FIRST_Y), cpu->y);
            OpWrite16(memory, OpAbs(cpu, PAIRED_INPUT_SECOND_Y), cpu->y);
            return ExecutionReturned(0x86a337u);
        }
    }
    return ExecutionHandoff(cpu, 0x86a2f1u);
}
