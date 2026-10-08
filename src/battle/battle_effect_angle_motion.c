#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_EFFECT_STREAM = 0xc3u,
    DP_MOTION_ANGLE = 0x54u,
    DP_MOTION_HORIZONTAL = 0x56u,
    DP_MOTION_VERTICAL = 0x58u,
    DP_MOTION_SPEED = 0x5au,
    EFFECT_HORIZONTAL_VELOCITY = 0x1bu,
    EFFECT_VERTICAL_VELOCITY = 0x1du,
    EFFECT_HORIZONTAL_POSITION = 0x1fu,
    EFFECT_VERTICAL_POSITION = 0x21u,
    EFFECT_ANGLE = 0x23u
};

static uint8_t EffectMotionContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void EffectMotionBank(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7eu);
}

static void AdvanceEffectOperand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
}

static Lufia2ExecutionResult EffectMotionUnwind(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void ApplyEffectVelocity(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t subtract) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_HORIZONTAL_VELOCITY));
    cpu->carry = subtract;
    if (subtract)
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_MOTION_HORIZONTAL)));
    else
        OpAdc(memory, cpu, OpDp(cpu, DP_MOTION_HORIZONTAL));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_HORIZONTAL_VELOCITY));
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_VERTICAL_VELOCITY));
    cpu->carry = subtract;
    if (subtract)
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_MOTION_VERTICAL)));
    else
        OpAdc(memory, cpu, OpDp(cpu, DP_MOTION_VERTICAL));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_VERTICAL_VELOCITY));
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2BattleEffectAimAtPoint(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectMotionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x81a062u);
    EffectMotionBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    AdvanceEffectOperand(memory, cpu);
    AdvanceEffectOperand(memory, cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, EFFECT_HORIZONTAL_VELOCITY)));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, EFFECT_HORIZONTAL_POSITION)));
    OpSta(memory, cpu, OpDp(cpu, DP_MOTION_HORIZONTAL));
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    AdvanceEffectOperand(memory, cpu);
    AdvanceEffectOperand(memory, cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, EFFECT_VERTICAL_VELOCITY)));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, EFFECT_VERTICAL_POSITION)));
    OpSta(memory, cpu, OpDp(cpu, DP_MOTION_VERTICAL));
    OpSepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81a08au, 0x85dac7u, 3u, 0x81u))
        return EffectMotionUnwind(0x81a08au);
    OpLda(memory, cpu, OpDp(cpu, DP_MOTION_ANGLE));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_ANGLE));
    return ExecutionReturned(0x81a093u);
}

static void ReadEffectSpeed(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpDp(cpu, DP_MOTION_SPEED));
    AdvanceEffectOperand(memory, cpu);
    AdvanceEffectOperand(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_ANGLE));
    OpSta(memory, cpu, OpDp(cpu, DP_MOTION_ANGLE));
}

Lufia2ExecutionResult Lufia2BattleEffectAccelerateAtAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectMotionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x81a094u);
    EffectMotionBank(memory, cpu);
    ReadEffectSpeed(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81a0a9u, 0x85dd63u, 3u, 0x81u))
        return EffectMotionUnwind(0x81a0a9u);
    ApplyEffectVelocity(memory, cpu, 0u);
    return ExecutionReturned(0x81a0c3u);
}

Lufia2ExecutionResult Lufia2BattleEffectDecelerateAtAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectMotionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x81a0c4u);
    EffectMotionBank(memory, cpu);
    ReadEffectSpeed(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81a0d9u, 0x85dd63u, 3u, 0x81u))
        return EffectMotionUnwind(0x81a0d9u);
    ApplyEffectVelocity(memory, cpu, 1u);
    return ExecutionReturned(0x81a0f3u);
}

Lufia2ExecutionResult Lufia2BattleEffectDecelerateAtOffsetAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectMotionContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x81a0f4u);
    EffectMotionBank(memory, cpu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsY(cpu, EFFECT_ANGLE));
    OpSta(memory, cpu, OpDp(cpu, DP_MOTION_ANGLE));
    OpRepWidths(cpu, 0x20u);
    AdvanceEffectOperand(memory, cpu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpDp(cpu, DP_MOTION_SPEED));
    AdvanceEffectOperand(memory, cpu);
    AdvanceEffectOperand(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81a10eu, 0x85dd63u, 3u, 0x81u))
        return EffectMotionUnwind(0x81a10eu);
    ApplyEffectVelocity(memory, cpu, 1u);
    return ExecutionReturned(0x81a128u);
}
