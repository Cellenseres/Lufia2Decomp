#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_DRAW_STREAM = 0xc3u,
    DP_DRAW_FLAGS = 0u,
    DP_DRAW_PARAMETER_OFFSET = 0x54u,
    DRAW_PARAMETERS = 0x13u,
    DRAW_CHANGED = 0x26u,
    DRAW_VALUE = 0x27u,
    DRAW_MODE = 0x28u,
    DRAW_VARIANT = 0x29u,
    DRAW_FLAG = 0x2au,
    DRAW_ATTRIBUTES = 0x2bu
};

static uint8_t EffectDrawContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void MarkEffectDrawChanged(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsY(cpu, DRAW_CHANGED));
}

static void ReadPackedEffectDrawState(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t alternate) {
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_DRAW_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_DRAW_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_DRAW_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_DRAW_FLAGS));
    OpAndValue(cpu, 0xc0u);
    OpSta(memory, cpu, OpAbsY(cpu, DRAW_ATTRIBUTES));
    OpLda(memory, cpu, OpDp(cpu, DP_DRAW_FLAGS));
    OpAndValue(cpu, 1u);
    OpSta(memory, cpu, OpAbsY(cpu, DRAW_FLAG));
    OpLda(memory, cpu, OpDp(cpu, DP_DRAW_FLAGS));
    OpAndValue(cpu, 0x0eu);
    if (alternate)
        OpOraValue(cpu, 1u);
    OpSta(memory, cpu, OpAbsY(cpu, DRAW_MODE));
    OpLda(memory, cpu, OpDp(cpu, DP_DRAW_FLAGS));
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpLsrA(cpu);
    OpAndValue(cpu, 3u);
    OpSta(memory, cpu, OpAbsY(cpu, DRAW_VARIANT));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, DRAW_VALUE));
    MarkEffectDrawChanged(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectSetPackedDrawState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectDrawContext(cpu))
        return ExecutionHandoff(cpu, 0x81a12au);
    ReadPackedEffectDrawState(memory, cpu, 0u);
    return ExecutionReturned(0x81a161u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetAlternatePackedDrawState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectDrawContext(cpu))
        return ExecutionHandoff(cpu, 0x81a162u);
    ReadPackedEffectDrawState(memory, cpu, 1u);
    return ExecutionReturned(0x81a19bu);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDrawState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectDrawContext(cpu))
        return ExecutionHandoff(cpu, 0x81a19cu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_DRAW_STREAM));
    OpSta(memory, cpu, OpAbsY(cpu, DRAW_ATTRIBUTES));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_DRAW_STREAM)));
    const uint16_t fields[] = {DRAW_VALUE, DRAW_VARIANT, DRAW_FLAG};
    for (unsigned field = 0u; field < 3u; ++field) {
        OpInx(cpu);
        OpWriteX(memory, cpu, OpDp(cpu, DP_DRAW_STREAM), cpu->x);
        OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_DRAW_STREAM));
        OpSta(memory, cpu, OpAbsY(cpu, fields[field]));
    }
    OpInx(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_DRAW_STREAM), cpu->x);
    MarkEffectDrawChanged(memory, cpu);
    return ExecutionReturned(0x81a1c7u);
}

Lufia2ExecutionResult Lufia2BattleEffectCopyDrawValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectDrawContext(cpu))
        return ExecutionHandoff(cpu, 0x81a39du);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_DRAW_STREAM));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_DRAW_STREAM), 1);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_DRAW_PARAMETER_OFFSET));
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_DRAW_PARAMETER_OFFSET));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, DRAW_PARAMETERS));
    OpSta(memory, cpu, OpAbsY(cpu, DRAW_VALUE));
    MarkEffectDrawChanged(memory, cpu);
    return ExecutionReturned(0x81a3beu);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDrawVariant(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectDrawContext(cpu))
        return ExecutionHandoff(cpu, 0x81a3bfu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_DRAW_STREAM));
    OpSta(memory, cpu, OpAbsY(cpu, DRAW_VARIANT));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_DRAW_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    MarkEffectDrawChanged(memory, cpu);
    return ExecutionReturned(0x81a3d3u);
}
