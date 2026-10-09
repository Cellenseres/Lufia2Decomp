#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "system/wram.h"

enum {
    DP_EQUIPMENT_MEMBER = 0x2au,
    PARTY_EQUIPMENT_SLOTS = 0x66u,
    PARTY_EQUIPMENT_MODIFIERS = 0x86u,
    PARTY_EQUIPMENT_SLOT_COUNT = 6u,
    PARTY_EQUIPMENT_MODIFIER_COUNT = 7u,
    PARTY_EQUIPMENT_SELECTED_SLOT = 0x1523u,
    ITEM_EQUIPMENT_MODIFIERS = WRAM_RECORD_BUFFER + 0x1eu,
    PARTY_FIRST_RECORD = 0x0badu,
    PARTY_RECORD_SIZE = 0xbeu,
    PARTY_RECORD_COUNT = 7u
};

static Lufia2ExecutionResult EquipmentChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2PartySelectEquipmentSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f703u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, PARTY_EQUIPMENT_SLOTS);
    OpSta(memory, cpu, OpAbs(cpu, PARTY_EQUIPMENT_SELECTED_SLOT));
    OpTay(cpu);
    return ExecutionReturned(0x82f70fu);
}

Lufia2ExecutionResult Lufia2PartyLoadEquipmentItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f710u);
    const uint16_t member = Read16Direct(memory, cpu, DP_EQUIPMENT_MEMBER);
    OpLda(memory, cpu, AbsoluteIndexedAddress(cpu, member, cpu->y));
    if (cpu->zero) {
        cpu->carry = 1u;
        return ExecutionReturned(0x82f721u);
    }
    OpAndValue(cpu, 0x1ffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f71au, 0x81f1c5u, 3u, cpu->program_bank))
        return EquipmentChildUnwound(0x82f71au);
    cpu->carry = 0u;
    return ExecutionReturned(0x82f71fu);
}

Lufia2ExecutionResult Lufia2PartyAccumulateEquipmentModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f722u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_EQUIPMENT_MEMBER)));
    for (unsigned modifier = 0u; modifier < PARTY_EQUIPMENT_MODIFIER_COUNT; ++modifier) {
        const uint16_t offset = (uint16_t)(2u * modifier);
        OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(PARTY_EQUIPMENT_MODIFIERS + offset)));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, (uint16_t)(ITEM_EQUIPMENT_MODIFIERS + offset)));
        OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(PARTY_EQUIPMENT_MODIFIERS + offset)));
    }
    return ExecutionReturned(0x82f76au);
}

Lufia2ExecutionResult Lufia2PartyRebuildEquipmentModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f846u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f849u, 0x82f6a4u, 2u, cpu->program_bank))
        return EquipmentChildUnwound(0x82f849u);
    OpLoadA(cpu, 0u);
    do {
        PushAccumulator16(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f850u, 0x82f703u, 2u, cpu->program_bank))
            return EquipmentChildUnwound(0x82f850u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82f853u, 0x82f710u, 2u, cpu->program_bank))
            return EquipmentChildUnwound(0x82f853u);
        if (!cpu->carry && !CallChildWithFrame(memory, cpu, child, context,
                0x82f858u, 0x82f722u, 2u, cpu->program_bank))
            return EquipmentChildUnwound(0x82f858u);
        PullAccumulator16(memory, cpu);
        OpIncA(cpu);
        OpCmpValue(cpu, PARTY_EQUIPMENT_SLOT_COUNT);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x82f863u);
}

Lufia2ExecutionResult Lufia2PartyRebuildAllEquipmentStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82994eu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, PARTY_FIRST_RECORD);
    OpSta(memory, cpu, OpDp(cpu, DP_EQUIPMENT_MEMBER));
    OpLoadA(cpu, PARTY_RECORD_COUNT);
    do {
        PushAccumulator16(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x829959u, 0x82f846u, 2u, cpu->program_bank))
            return EquipmentChildUnwound(0x829959u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_EQUIPMENT_MEMBER)));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82995eu, 0x81f4d5u, 3u, cpu->program_bank))
            return EquipmentChildUnwound(0x82995eu);
        OpLda(memory, cpu, OpDp(cpu, DP_EQUIPMENT_MEMBER));
        cpu->carry = 0u;
        OpAdcValue(cpu, PARTY_RECORD_SIZE);
        OpSta(memory, cpu, OpDp(cpu, DP_EQUIPMENT_MEMBER));
        PullAccumulator16(memory, cpu);
        OpDecA(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x829970u);
}
