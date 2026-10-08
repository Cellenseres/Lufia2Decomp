#include "core/child_call.h"
#include "core/cpu_ops.h"
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
