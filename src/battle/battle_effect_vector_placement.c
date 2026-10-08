#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_EFFECT_STREAM = 0xc3u,
    DP_VECTOR_ANGLE = 0x54u,
    DP_VECTOR_HORIZONTAL = 0x56u,
    DP_VECTOR_VERTICAL = 0x58u,
    DP_VECTOR_SPEED = 0x5au,
    EFFECT_PARAMETER_BASE = 0x13u,
    EFFECT_HORIZONTAL_VECTOR = 0x1bu,
    EFFECT_VERTICAL_VECTOR = 0x1du,
    EFFECT_HORIZONTAL_POSITION = 0x1fu,
    EFFECT_VERTICAL_POSITION = 0x21u
};

static uint8_t EffectVectorContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void AdvanceVectorOperand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
}

static Lufia2ExecutionResult EffectVectorUnwind(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void ReadVectorParameter(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t parameter) {
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, parameter));
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, parameter));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, EFFECT_PARAMETER_BASE));
    OpSta(memory, cpu, OpDp(cpu, parameter));
    AdvanceVectorOperand(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectProjectFromPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectVectorContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x81a264u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_ANGLE));
    OpRepWidths(cpu, 0x20u);
    AdvanceVectorOperand(memory, cpu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpDp(cpu, DP_VECTOR_SPEED));
    AdvanceVectorOperand(memory, cpu);
    AdvanceVectorOperand(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81a27au, 0x85dd63u, 3u, 0x81u))
        return EffectVectorUnwind(0x81a27au);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_HORIZONTAL_POSITION));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_VECTOR_HORIZONTAL));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_HORIZONTAL_VECTOR));
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_VERTICAL_POSITION));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_VERTICAL_VECTOR));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x81a294u);
}

Lufia2ExecutionResult Lufia2BattleEffectVectorFromParameters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectVectorContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x81a31du);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpRepWidths(cpu, 0x20u);
    ReadVectorParameter(memory, cpu, DP_VECTOR_ANGLE);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    ReadVectorParameter(memory, cpu, DP_VECTOR_SPEED);
    OpSepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81a34bu, 0x85dd63u, 3u, 0x81u))
        return EffectVectorUnwind(0x81a34bu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_HORIZONTAL));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_HORIZONTAL_VECTOR));
    OpLda(memory, cpu, OpDp(cpu, DP_VECTOR_VERTICAL));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_VERTICAL_VECTOR));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x81a35du);
}
