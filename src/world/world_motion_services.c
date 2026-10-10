#include "lufia2/world_map.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    DP_RECORD_OFFSET = 0x02,
    DP_RECORDS_LEFT = 0x04,
    DP_TARGET_X = 0x58,
    DP_TARGET_Y = 0x5a,
    MOTION_POINTERS = 0x1206,
    MOTION_VALUES = 0x121a,
    MOTION_MODE = 0x09e0,
    MOTION_STATE = 0x09e1,
    ARRIVAL_X = 0x09e3,
    ARRIVAL_Y = 0x09e4,
    ARRIVAL_FLAG = 0x123f,
    WORLD_DISPLAY_STATE = 0x11d9,
    MOTION_TERM_LOW = 0x1244,
    MOTION_TERM_HIGH = 0x1245
};

static uint8_t MotionWidths(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

static void CopyMotionValues(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t destinations[] = {
        0x11e8, 0x11ea, 0x1210, 0x1212, 0x1214, 0x1216, 0x1218};
    static const uint16_t copies[] = {
        0, 0, 0x1247, 0x1249, 0x11fc, 0x11fe, 0x1200};

    for (unsigned field = 0; field < 7u; ++field) {
        OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(field * 2u)));
        OpSta(memory, cpu, OpAbs(cpu, destinations[field]));
        if (copies[field])
            OpSta(memory, cpu, OpAbs(cpu, copies[field]));
    }
}

Lufia2ExecutionResult Lufia2WorldMapLoadMotionRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionWidths(cpu))
        return ExecutionHandoff(cpu, 0x869470u);
    OpRepWidths(cpu, 0x20u);
    PushY(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, DP_RECORD_OFFSET));
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, DP_RECORDS_LEFT));
    do {
        OpLdy(cpu, OpRead16(memory, OpAbsX(cpu, 0u)));
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpPushX(memory, cpu);
        OpLdx(cpu, OpRead16(memory, OpDp(cpu, DP_RECORD_OFFSET)));
        OpSta(memory, cpu, OpAbsX(cpu, MOTION_VALUES));
        OpTya(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, MOTION_POINTERS));
        OpInx(cpu);
        OpInx(cpu);
        OpWrite16(memory, OpDp(cpu, DP_RECORD_OFFSET), cpu->x);
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_RECORDS_LEFT), -1);
    } while (!cpu->zero);
    OpPullX(memory, cpu);
    if (!cpu->zero)
        CopyMotionValues(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8694d3u);
}

Lufia2ExecutionResult Lufia2WorldMapDisableDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionWidths(cpu))
        return ExecutionHandoff(cpu, 0x86973eu);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WORLD_DISPLAY_STATE));
    return ExecutionReturned(0x869747u);
}

Lufia2ExecutionResult Lufia2WorldMapSumMotionTerms(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionWidths(cpu))
        return ExecutionHandoff(cpu, 0x86994cu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E16CF));
    OpLda(memory, cpu, OpAbs(cpu, MOTION_TERM_LOW));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, MOTION_TERM_HIGH));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86995au);
}

Lufia2ExecutionResult Lufia2WorldMapRememberTilePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionWidths(cpu))
        return ExecutionHandoff(cpu, 0x869a44u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_TILE_X));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_SAVED_TILE_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_TILE_Y));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_SAVED_TILE_Y));
    return ExecutionReturned(0x869a50u);
}

static Lufia2ExecutionResult CheckWorldArrival(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->zero) {
        cpu->carry = 0u;
        return ExecutionReturned(0x869adau);
    }
    OpLda(memory, cpu, OpAbs(cpu, MOTION_STATE));
    if (!cpu->zero) {
        cpu->carry = 0u;
        return ExecutionReturned(0x869adau);
    }
    OpLda(memory, cpu, OpDp(cpu, DP_TARGET_X));
    OpCmp(memory, cpu, OpAbs(cpu, ARRIVAL_X));
    if (!cpu->zero) {
        cpu->carry = 0u;
        return ExecutionReturned(0x869adau);
    }
    OpLda(memory, cpu, OpDp(cpu, DP_TARGET_Y));
    OpCmp(memory, cpu, OpAbs(cpu, ARRIVAL_Y));
    if (!cpu->zero) {
        cpu->carry = 0u;
        return ExecutionReturned(0x869adau);
    }
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, ARRIVAL_FLAG));
    OpStz(memory, cpu, OpDp(cpu, 0u));
    cpu->carry = 1u;
    return ExecutionReturned(0x869ad8u);
}

Lufia2ExecutionResult Lufia2WorldMapCheckMovingArrival(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionWidths(cpu))
        return ExecutionHandoff(cpu, 0x869ab1u);
    OpLda(memory, cpu, OpAbs(cpu, MOTION_MODE));
    OpDecA(cpu);
    OpDecA(cpu);
    return CheckWorldArrival(memory, cpu);
}

Lufia2ExecutionResult Lufia2WorldMapCheckStoppedArrival(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionWidths(cpu))
        return ExecutionHandoff(cpu, 0x869ab8u);
    OpLda(memory, cpu, OpAbs(cpu, MOTION_MODE));
    return CheckWorldArrival(memory, cpu);
}

static void StartWorldFade(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t control) {
    OpLoadA(cpu, 0x10u);
    OpOraValue(cpu, control);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
}

Lufia2ExecutionResult Lufia2WorldMapStartFadeOut(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionWidths(cpu))
        return ExecutionHandoff(cpu, 0x86d37du);
    StartWorldFade(memory, cpu, 0xc0u);
    return ExecutionReturned(0x86d396u);
}

Lufia2ExecutionResult Lufia2WorldMapStartFadeIn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionWidths(cpu))
        return ExecutionHandoff(cpu, 0x86d381u);
    StartWorldFade(memory, cpu, 0x80u);
    return ExecutionReturned(0x86d38du);
}
