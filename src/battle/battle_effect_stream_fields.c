#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_EFFECT_STREAM = 0xc3u,
    EFFECT_HORIZONTAL_VELOCITY = 0x1bu,
    EFFECT_VERTICAL_VELOCITY = 0x1du,
    EFFECT_ANGLE = 0x23u,
    EFFECT_BASE_ANGLE = 0x24u,
    EFFECT_DRAW_DIRTY = 0x26u,
    EFFECT_DRAW_VALUE = 0x27u,
    EFFECT_DRAW_MODE = 0x28u,
    EFFECT_DRAW_FLAG = 0x2au,
    EFFECT_DRAW_ATTRIBUTES = 0x2bu
};

static uint8_t EffectFieldContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void AdvanceEffectField(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
}

static Lufia2ExecutionResult OffsetEffectAngle(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t angle, uint32_t entry) {
    if (!EffectFieldContext(cpu))
        return ExecutionHandoff(cpu, entry);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsY(cpu, angle));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_ANGLE));
    OpRepWidths(cpu, 0x20u);
    AdvanceEffectField(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(entry + 19u);
}

static Lufia2ExecutionResult SetEffectDrawField(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t field, uint32_t entry) {
    if (!EffectFieldContext(cpu))
        return ExecutionHandoff(cpu, entry);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpAbsY(cpu, field));
    OpRepWidths(cpu, 0x20u);
    AdvanceEffectField(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    LoadA8(cpu, 1u);
    Write8(memory, OpAbsY(cpu, EFFECT_DRAW_DIRTY), 1u);
    return ExecutionReturned(entry + 20u);
}

static void ReadEffectVelocityWord(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t field) {
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    AdvanceEffectField(memory, cpu);
    AdvanceEffectField(memory, cpu);
    OpSta(memory, cpu, OpAbsY(cpu, field));
}

Lufia2ExecutionResult Lufia2BattleEffectSetVelocityWords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectFieldContext(cpu))
        return ExecutionHandoff(cpu, 0x81a295u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    ReadEffectVelocityWord(memory, cpu, EFFECT_HORIZONTAL_VELOCITY);
    ReadEffectVelocityWord(memory, cpu, EFFECT_VERTICAL_VELOCITY);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x81a2afu);
}

Lufia2ExecutionResult Lufia2BattleEffectOffsetBaseAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return OffsetEffectAngle(memory, cpu, EFFECT_BASE_ANGLE, 0x81a03au);
}

Lufia2ExecutionResult Lufia2BattleEffectRotateAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return OffsetEffectAngle(memory, cpu, EFFECT_ANGLE, 0x81a04eu);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDrawAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectDrawField(memory, cpu, EFFECT_DRAW_ATTRIBUTES, 0x81a35eu);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDrawValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectDrawField(memory, cpu, EFFECT_DRAW_VALUE, 0x81a373u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDrawMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectDrawField(memory, cpu, EFFECT_DRAW_MODE, 0x81a388u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDrawFlag(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SetEffectDrawField(memory, cpu, EFFECT_DRAW_FLAG, 0x81a3d4u);
}
