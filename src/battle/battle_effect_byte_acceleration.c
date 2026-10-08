#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_ACCELERATION_STREAM = 0xc3u,
    DP_ACCELERATION_ANGLE = 0x54u,
    DP_ACCELERATION_HORIZONTAL = 0x56u,
    DP_ACCELERATION_VERTICAL = 0x58u,
    DP_ACCELERATION_SPEED = 0x5au,
    EFFECT_HORIZONTAL_VELOCITY = 0x1bu,
    EFFECT_VERTICAL_VELOCITY = 0x1du,
    EFFECT_ANGLE = 0x23u,
    SPEED_CONSTANT = 0u,
    SPEED_FROM_STREAM = 1u,
    VELOCITY_ADD = 0u,
    VELOCITY_SUBTRACT = 1u
};

static void ApplyEffectAcceleration(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t subtract) {
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_HORIZONTAL_VELOCITY));
    cpu->carry = subtract;
    if (subtract)
        OpSbcValue(cpu, OpReadM(memory, cpu,
            OpDp(cpu, DP_ACCELERATION_HORIZONTAL)));
    else
        OpAdc(memory, cpu, OpDp(cpu, DP_ACCELERATION_HORIZONTAL));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_HORIZONTAL_VELOCITY));
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_VERTICAL_VELOCITY));
    cpu->carry = subtract;
    if (subtract)
        OpSbcValue(cpu, OpReadM(memory, cpu,
            OpDp(cpu, DP_ACCELERATION_VERTICAL)));
    else
        OpAdc(memory, cpu, OpDp(cpu, DP_ACCELERATION_VERTICAL));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_VERTICAL_VELOCITY));
    OpSepWidths(cpu, 0x20u);
}

static Lufia2ExecutionResult AccelerateEffect(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint32_t site, uint32_t end, uint8_t speed,
    uint8_t stream_speed, uint8_t subtract) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, entry);
    OpSetDataBank(memory, cpu, 0x7eu);
    if (stream_speed)
        OpLda(memory, cpu, DirectLongPointer(memory, cpu,
            DP_ACCELERATION_STREAM));
    else
        OpLoadA(cpu, speed);
    OpSta(memory, cpu, OpDp(cpu, DP_ACCELERATION_SPEED));
    OpStz(memory, cpu, OpDp(cpu, DP_ACCELERATION_SPEED + 1u));
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_ANGLE));
    OpSta(memory, cpu, OpDp(cpu, DP_ACCELERATION_ANGLE));
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, 0x85dd63u, 3u, 0x81u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpRepWidths(cpu, 0x20u);
    if (stream_speed)
        OpStepMem(memory, cpu, OpDp(cpu, DP_ACCELERATION_STREAM), 1);
    ApplyEffectAcceleration(memory, cpu, subtract);
    return ExecutionReturned(end);
}

Lufia2ExecutionResult Lufia2BattleEffectAccelerateByByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return AccelerateEffect(memory, cpu, child, context,
        0x81a69au, 0x81a6a9u, 0x81a6c5u, 0u, SPEED_FROM_STREAM, VELOCITY_ADD);
}

Lufia2ExecutionResult Lufia2BattleEffectDecelerateByByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return AccelerateEffect(memory, cpu, child, context,
        0x81a6c6u, 0x81a6d5u, 0x81a6f1u, 0u, SPEED_FROM_STREAM, VELOCITY_SUBTRACT);
}

Lufia2ExecutionResult Lufia2BattleEffectAccelerateOne(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return AccelerateEffect(memory, cpu, child, context,
        0x81a6f2u, 0x81a701u, 0x81a71bu, 1u, SPEED_CONSTANT, VELOCITY_ADD);
}

Lufia2ExecutionResult Lufia2BattleEffectAccelerateTwo(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return AccelerateEffect(memory, cpu, child, context,
        0x81a71cu, 0x81a72bu, 0x81a745u, 2u, SPEED_CONSTANT, VELOCITY_ADD);
}

Lufia2ExecutionResult Lufia2BattleEffectAccelerateThree(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return AccelerateEffect(memory, cpu, child, context,
        0x81a746u, 0x81a755u, 0x81a76fu, 3u, SPEED_CONSTANT, VELOCITY_ADD);
}

Lufia2ExecutionResult Lufia2BattleEffectAccelerateFour(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return AccelerateEffect(memory, cpu, child, context,
        0x81a770u, 0x81a77fu, 0x81a799u, 4u, SPEED_CONSTANT, VELOCITY_ADD);
}
