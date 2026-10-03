#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

static Lufia2ExecutionResult AreaChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target) {
    SimulateJslFrame(memory, cpu, 0x83u, (uint16_t)(site + 3u));
    if (!child(context, cpu, target, site, 3u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, site + 4u);
    return ExecutionReturned(site + 4u);
}

static void PrepareAreaDestination(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_TRANSITION_SOURCE_MAP));
    OpLda(memory, cpu, OpAbsX(cpu, 0xf006u));
    OpSta(memory, cpu, WRAM_FIELD_DESTINATION_X);
    OpLda(memory, cpu, OpAbsX(cpu, 0xf007u));
    OpSta(memory, cpu, WRAM_FIELD_DESTINATION_Y);
    OpLda(memory, cpu, OpAbsX(cpu, 0xf005u));
    OpSta(memory, cpu, WRAM_FIELD_DESTINATION_PARAMETERS);
    OpAndValue(cpu, 0xf0u);
    OpCmpValue(cpu, 0xf0u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E066A));
        for (unsigned shift = 0; shift < 4u; ++shift)
            OpAslA(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        OpLda(memory, cpu, WRAM_FIELD_DESTINATION_PARAMETERS);
        OpAndValue(cpu, 0x0fu);
        OpOra(memory, cpu, OpDp(cpu, 0x54u));
        OpSta(memory, cpu, WRAM_FIELD_DESTINATION_PARAMETERS);
    }
    OpLda(memory, cpu, WRAM_FIELD_FLAGS);
    OpOraValue(cpu, 0x80u);
    OpSta(memory, cpu, WRAM_FIELD_FLAGS);
    OpLda(memory, cpu, WRAM_FIELD_MAP_ID);
    OpSta(memory, cpu, WRAM_FIELD_DESTINATION_MAP);
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, WRAM_FIELD_RELOAD_FLAGS);
    OpLda(memory, cpu, OpAbsX(cpu, 0xf008u));
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpSta(memory, cpu, WRAM_FIELD_DESTINATION_MAP);
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, WRAM_FIELD_RELOAD_FLAGS);
    } else {
        OpLda(memory, cpu, WRAM_FIELD_DESTINATION_MAP);
        OpCmpValue(cpu, 0xf0u);
        if (cpu->zero) {
            OpLda(memory, cpu, WRAM_FIELD_RELOAD_FLAGS);
            OpAndValue(cpu, 0xfbu);
            OpSta(memory, cpu, WRAM_FIELD_RELOAD_FLAGS);
        }
    }
}

Lufia2ExecutionResult Lufia2FieldApplyAreaTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xb76eu);
    PrepareAreaDestination(memory, cpu);
    OpLda(memory, cpu, WRAM_FIELD_DESTINATION_PARAMETERS);
    OpAndValue(cpu, 0x0fu);
    Lufia2ExecutionResult result = ExecutionReturned(0x83b82du);
    if (cpu->zero) {
        OpLoadA(cpu, 8u);
        OpBit(memory, cpu, OpAbs(cpu, WRAM_FIELD_FLAGS));
        if (!cpu->zero) {
            OpLda(memory, cpu, WRAM_FIELD_DESTINATION_MAP);
            if (!cpu->zero) {
                OpLoadA(cpu, 0x18u);
                result = AreaChild(memory, cpu, child, context, 0x83b7fdu, 0x848766u);
            }
        }
    } else {
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpStz(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
            result = AreaChild(memory, cpu, child, context, 0x83b809u, 0x83ab4fu);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            TransferDirectToA(cpu);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
            TransferAToX(cpu);
            OpLda(memory, cpu, OpLongX(cpu, 0x83c1a5u));
            cpu->carry = 0;
            OpAdcValue(cpu, 0x79u);
            result = AreaChild(memory, cpu, child, context, 0x83b819u, 0x83d350u);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpLoadA(cpu, 0x32u);
            result = AreaChild(memory, cpu, child, context, 0x83b81fu, 0x83e033u);
        } else {
            OpCmpValue(cpu, 7u);
            if (cpu->zero)
                result = AreaChild(memory, cpu, child, context, 0x83b829u, 0x83b82fu);
        }
    }
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x83b82eu);
}
