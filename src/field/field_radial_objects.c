#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "actor/actor_internal.h"
#include "system/system_internal.h"
#include "system/wram.h"
#include "lufia2/field.h"

enum {
    DP_RADIAL_COUNT = 0x00,
    DP_RADIAL_MODE = 0x01,
    DP_RADIAL_X = 0x02,
    DP_RADIAL_Y = 0x04,
    DP_RADIAL_ZOOM = 0x06,
    DP_RADIAL_INDEX = 0x07,
    DP_RADIAL_ANGLE_A = 0x08,
    DP_RADIAL_ANGLE_B = 0x09,
    DP_RADIAL_SPAWN_ID = 0x0a,
    DP_RADIAL_ACTION = 0x0e,
    DP_RADIAL_PARAMETER = 0x0f,
    RADIAL_ANGLE_STEP = 0x1719,
    RADIAL_ANGLE_A = 0x14d9,
    RADIAL_ANGLE_B = 0x14f9,
    RADIAL_ZOOM = 0x1579,
    RADIAL_OFFSET_ANGLE = 0x1599,
};

static void RadialObjectPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_X));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SCREEN_X));
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_Y));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SCREEN_Y));
    OpSepWidths(cpu, 0x20u);
}

static void RadialObjectMotion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t cleared[] = {
        0x15d9u, 0x15f9u, 0x1639u, 0x1659u, 0x1699u,
        0x16b9u, 0x1619u, 0x1679u, 0x16d9u};
    OpLda(memory, cpu, OpAbs(cpu, RADIAL_ANGLE_STEP));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_INDEX));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_MODE));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E1559));
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_ANGLE_A));
    OpSta(memory, cpu, OpAbsY(cpu, RADIAL_ANGLE_A));
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_ANGLE_B));
    OpSta(memory, cpu, OpAbsY(cpu, RADIAL_ANGLE_B));
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_ZOOM));
    OpSta(memory, cpu, OpAbsY(cpu, RADIAL_ZOOM));
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    cpu->carry = 0;
    OpAdcValue(cpu, 0xc0u);
    OpSta(memory, cpu, OpAbsY(cpu, RADIAL_OFFSET_ANGLE));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_RECOVERY_OBJECT_UPDATES));
    TransferDirectToA(cpu);
    for (unsigned index = 0; index < sizeof(cleared) / sizeof(cleared[0]); ++index)
        OpSta(memory, cpu, OpAbsY(cpu, cleared[index]));
}

static void RadialObjectParameters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_ACTION));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FDA2C));
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_PARAMETER));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_REQUEST));
}

Lufia2ExecutionResult Lufia2FieldSpawnRadialObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x83c729u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    OpLoadA(cpu, 1u);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WRDIVL));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVH));
    OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVB));
    OpStz(memory, cpu, OpDp(cpu, DP_RADIAL_INDEX));
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDDIVL));
    OpSta(memory, cpu, OpAbs(cpu, RADIAL_ANGLE_STEP));
    do {
        OpRepWidths(cpu, 0x10u);
        OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_SPAWN_ID));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x83c74bu, 0x83df87u, 3u, 0x83u)) {
            Lufia2ExecutionResult result = ExecutionReturned(0x83c74bu);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        OpSepWidths(cpu, 0x10u);
        RadialObjectPosition(memory, cpu);
        RadialObjectMotion(memory, cpu);
        RadialObjectParameters(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_RADIAL_INDEX));
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_RADIAL_INDEX));
        OpCmp(memory, cpu, OpDp(cpu, DP_RADIAL_COUNT));
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83c7c6u);
}
