#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_PARAMETER_STREAM = 0xc3u,
    EFFECT_PARAMETER_OFFSET = 0x15edu,
    EFFECT_PARAMETERS = 0x13u
};

static uint8_t EffectParameterContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void SelectEffectParameter(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_PARAMETER_STREAM));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_PARAMETER_STREAM), 1);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, EFFECT_PARAMETER_OFFSET));
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, EFFECT_PARAMETER_OFFSET));
    OpTax(cpu);
}

static void ReadEffectParameterWord(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_PARAMETER_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_PARAMETER_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_PARAMETER_STREAM), 1);
}

Lufia2ExecutionResult Lufia2BattleEffectSetParameterWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectParameterContext(cpu))
        return ExecutionHandoff(cpu, 0x81a3e9u);
    SelectEffectParameter(memory, cpu);
    ReadEffectParameterWord(memory, cpu);
    OpSta(memory, cpu, OpAbsX(cpu, EFFECT_PARAMETERS));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x81a40au);
}

Lufia2ExecutionResult Lufia2BattleEffectRandomizeParameterWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectParameterContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x81a431u);
    SelectEffectParameter(memory, cpu);
    OpLoadA(cpu, 0x80u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81a44au, 0x808299u, 3u, 0x81u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x81a44au);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpAslA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, EFFECT_PARAMETERS));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x81a454u);
}
