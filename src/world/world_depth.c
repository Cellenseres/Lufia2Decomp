#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/world_map.h"

enum {
    DP_DEPTH_LOW = 0x00u,
    DP_DEPTH_HIGH = 0x02u,
    DP_DEPTH_DIVISOR = 0x04u,
    DP_DEPTH_SCALE = 0x06u,
    WORLD_DEPTH_DISTANCE = 0x11fcu,
    WORLD_DEPTH_TILT = 0x11ffu,
    WORLD_DEPTH_TILT_LIMIT = 0x40u,
    ROM_WORLD_DEPTH_SCALE = 0x97b226u
};

static uint8_t DepthEntry(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x86u && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult DepthUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void ReadDepthScale(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_WORLD_DEPTH_SCALE));
}

Lufia2ExecutionResult Lufia2WorldMapProjectTiltDistance(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!DepthEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x86a913u);
    OpStz(memory, cpu, OpDp(cpu, DP_DEPTH_LOW));
    OpStz(memory, cpu, OpDp(cpu, DP_DEPTH_HIGH));
    OpLda(memory, cpu, OpAbs(cpu, WORLD_DEPTH_TILT));
    OpAndValue(cpu, 0xffu);
    OpCmpValue(cpu, WORLD_DEPTH_TILT_LIMIT);
    if (cpu->carry)
        return ExecutionReturned(0x86a955u);
    ReadDepthScale(memory, cpu);
    PushAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_DEPTH_DIVISOR));
    OpLda(memory, cpu, OpAbs(cpu, WORLD_DEPTH_DISTANCE));
    OpSta(memory, cpu, OpDp(cpu, DP_DEPTH_LOW));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86a930u, 0x86a52fu, 2u, 0x86u))
        return DepthUnwound(0x86a930u);
    PullAccumulator16(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, 0xe0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_DEPTH_DISTANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_DEPTH_LOW + 1u)));
    OpSta(memory, cpu, OpDp(cpu, DP_DEPTH_HIGH));
    OpStz(memory, cpu, OpDp(cpu, DP_DEPTH_LOW));
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYH));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_DEPTH_DIVISOR));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86a952u, 0x86a5a9u, 2u, 0x86u))
        return DepthUnwound(0x86a952u);
    return ExecutionReturned(0x86a955u);
}

static void PrepareDepthReciprocal(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, DP_DEPTH_LOW + 1u)));
    OpWrite16(memory, OpDp(cpu, DP_DEPTH_DIVISOR), cpu->x);
    OpLdx(cpu, 0u);
    OpWrite16(memory, OpDp(cpu, DP_DEPTH_LOW), cpu->x);
    OpLdx(cpu, 1u);
    OpWrite16(memory, OpDp(cpu, DP_DEPTH_HIGH), cpu->x);
}

Lufia2ExecutionResult Lufia2WorldMapProjectTiltReciprocal(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!DepthEntry(cpu) || !child)
        return ExecutionHandoff(cpu, 0x86a956u);
    OpStz(memory, cpu, OpDp(cpu, DP_DEPTH_HIGH));
    OpStz(memory, cpu, OpDp(cpu, DP_DEPTH_LOW));
    OpLoadA(cpu, 0u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, WORLD_DEPTH_TILT_LIMIT);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WORLD_DEPTH_TILT)));
    if (cpu->zero || !cpu->carry)
        return ExecutionReturned(0x86a9a7u);
    OpWrite16(memory, OpDp(cpu, DP_DEPTH_HIGH), cpu->x);
    ReadDepthScale(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_DEPTH_SCALE));
    OpLda(memory, cpu, OpAbs(cpu, WORLD_DEPTH_TILT));
    ReadDepthScale(memory, cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_DEPTH_SCALE)));
    OpSta(memory, cpu, OpDp(cpu, DP_DEPTH_DIVISOR));
    OpStz(memory, cpu, OpDp(cpu, DP_DEPTH_DIVISOR + 1u));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86a983u, 0x86a5a9u, 2u, 0x86u))
        return DepthUnwound(0x86a983u);
    PrepareDepthReciprocal(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x86a994u, 0x86a5a9u, 2u, 0x86u))
        return DepthUnwound(0x86a994u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WORLD_DEPTH_DISTANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_DEPTH_LOW)));
    OpSta(memory, cpu, OpDp(cpu, DP_DEPTH_LOW + 1u));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, DP_DEPTH_LOW));
    OpStz(memory, cpu, OpDp(cpu, DP_DEPTH_HIGH + 1u));
    return ExecutionReturned(0x86a9a7u);
}
