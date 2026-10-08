#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_DRAW_STREAM = 0xc3u,
    EFFECT_THIRD_PARAMETER = 0x17u,
    EFFECT_TARGET = 0x25u,
    EFFECT_DRAW_DIRTY = 0x26u,
    EFFECT_DRAW_VALUE = 0x27u,
    EFFECT_DRAW_VARIANT = 0x29u,
    EFFECT_DRAW_FLAG = 0x2au,
    EFFECT_DRAW_ATTRIBUTES = 0x2bu,
    EFFECT_UNBOUND_TARGET = 0xffu
};

static uint8_t EffectRecordContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2BattleEffectClearDrawDirty(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x81a1ecu);
    OpSetDataBank(memory, cpu, 0x7eu);
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_DRAW_DIRTY));
    return ExecutionReturned(0x81a1f4u);
}

Lufia2ExecutionResult Lufia2BattleEffectClearTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x81a1f5u);
    OpSetDataBank(memory, cpu, 0x7eu);
    LoadA8(cpu, EFFECT_UNBOUND_TARGET);
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_TARGET));
    return ExecutionReturned(0x81a1feu);
}

Lufia2ExecutionResult Lufia2BattleEffectSetDrawVariantThree(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x81a567u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_DRAW_STREAM));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_DRAW_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_DRAW_VALUE));
    LoadA8(cpu, 3u);
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_DRAW_VARIANT));
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_DRAW_ATTRIBUTES));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_DRAW_FLAG));
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_DRAW_DIRTY));
    return ExecutionReturned(0x81a587u);
}

Lufia2ExecutionResult Lufia2BattleEffectUseThirdDrawParameter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x81a588u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_THIRD_PARAMETER));
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_DRAW_VALUE));
    LoadA8(cpu, 1u);
    OpSta(memory, cpu, OpAbsY(cpu, EFFECT_DRAW_DIRTY));
    return ExecutionReturned(0x81a597u);
}

Lufia2ExecutionResult Lufia2BattleEffectClearBackgroundUploadRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x81a696u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_BACKGROUND_UPLOAD_REQUEST));
    return ExecutionReturned(0x81a699u);
}
