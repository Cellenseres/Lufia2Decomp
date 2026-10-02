#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

static uint8_t WaitForFieldFlagClear(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t address) {
    for (unsigned polls = 0; polls < 65536u; ++polls) {
        OpLda(memory, cpu, address);
        if (cpu->zero)
            return 1u;
    }
    return 0u;
}

static Lufia2ExecutionResult BattleReturnChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame_size) {
    if (frame_size == 3u)
        SimulateJslFrame(memory, cpu, 0x83u, (uint16_t)(site + 3u));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    if (!child(context, cpu, target, site, frame_size)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, site + frame_size + 1u);
    return ExecutionReturned(site + frame_size + 1u);
}

Lufia2ExecutionResult Lufia2FieldBattleTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0x83ebu);
    if (!WaitForFieldFlagClear(memory, cpu, WRAM_FIELD_COLUMN_UPLOAD_COUNT_BYTES))
        return ExecutionHandoff(cpu, 0x8383ebu);
    OpStz(memory, cpu, OpDp(cpu, 0x74u));
    OpLoadA(cpu, 0x18u);
    Lufia2ExecutionResult result = BattleReturnChild(
        memory, cpu, child, context, 0x8383f5u, 0x8093feu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = BattleReturnChild(memory, cpu, child, context, 0x8383f9u, 0x838e66u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = BattleReturnChild(memory, cpu, child, context, 0x8383fdu, 0x83845bu, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, WRAM_FIELD_BATTLE_RESULT);
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
        OpCmpValue(cpu, 0xf1u);
        if (!cpu->zero)
            OpCmpValue(cpu, 0xf0u);
        if (cpu->zero) {
            result = BattleReturnChild(memory, cpu, child, context, 0x838414u, 0x848b9cu, 3u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            return ExecutionReturned(0x83845au);
        }
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x1fffu);
        cpu->stack = cpu->accumulator;
        OpSepWidths(cpu, 0x20u);
        return ExecutionHandoff(cpu, 0x83acefu);
    }
    OpLoadA(cpu, 0xf8u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_STATE);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, WRAM_FIELD_MOSAIC_ACCUMULATOR);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_TRANSITION_FLAGS));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, 0x88u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    if (!WaitForFieldFlagClear(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL)))
        return ExecutionHandoff(cpu, 0x83843eu);
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, WRAM_FIELD_BATTLE_SOURCE);
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        result = BattleReturnChild(memory, cpu, child, context, 0x83844du, 0x83b8bfu, 2u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0x0cu);
    result = BattleReturnChild(memory, cpu, child, context, 0x838456u, 0x80e722u, 3u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    return ExecutionReturned(0x83845au);
}
