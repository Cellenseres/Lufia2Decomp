#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_RANDOM_VALUE = 0x00u,
    DP_PARAMETER_RANGE = 0x02u,
    DP_PARAMETER_OFFSET = 0x04u,
    DP_EFFECT_STREAM = 0xc3u,
    PARAMETER_DESCRIPTOR_TABLE = 0x81a25fu,
    PARAMETER_OFFSET_MASK = 0x7fu
};

static void ReadParameterRange(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_PARAMETER_RANGE));
}

static Lufia2ExecutionResult StoreCenteredParameter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, DP_RANDOM_VALUE));
    OpLda(memory, cpu, OpLongX(cpu, PARAMETER_DESCRIPTOR_TABLE));
    const uint8_t byte_parameter = cpu->negative;
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, PARAMETER_OFFSET_MASK);
    OpSta(memory, cpu, OpDp(cpu, DP_PARAMETER_OFFSET));
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_PARAMETER_OFFSET));
    OpTax(cpu);
    if (byte_parameter)
        OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_PARAMETER_RANGE));
    if (!byte_parameter)
        OpAndValue(cpu, 0xffu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PARAMETER_RANGE));
    OpLda(memory, cpu, OpDp(cpu, DP_RANDOM_VALUE));
    if (!byte_parameter)
        OpAndValue(cpu, 0xffu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_PARAMETER_RANGE)));
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    if (!byte_parameter)
        OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(byte_parameter ? 0x81a25eu : 0x81a242u);
}

Lufia2ExecutionResult Lufia2BattleEffectRandomizeCenteredParameter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x81a1ffu);
    ReadParameterRange(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81a215u, 0x808299u, 3u, 0x81u)) {
        const Lufia2ExecutionResult result = {
            LUFIA2_EXECUTION_CHILD_UNWOUND, 0x81a215u
        };
        return result;
    }
    return StoreCenteredParameter(memory, cpu);
}
