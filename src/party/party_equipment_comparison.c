#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "system/wram.h"

enum {
    DP_EQUIPMENT_MEMBER = 0x2au,
    MEMBER_EQUIPMENT_MODIFIERS = 0x86u,
    MEMBER_COMPARISON_MODIFIERS = 0x37u,
    ITEM_EQUIPMENT_MODIFIERS = WRAM_RECORD_BUFFER + 0x1eu,
    EQUIPMENT_MODIFIER_COUNT = 7u
};

static bool EquipmentComparisonReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void ApplyComparisonModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t source, bool remove) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_EQUIPMENT_MEMBER)));
    for (unsigned modifier = 0u; modifier < EQUIPMENT_MODIFIER_COUNT; ++modifier) {
        const uint16_t offset = (uint16_t)(2u * modifier);
        OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(source + offset)));
        cpu->carry = remove ? 1u : 0u;
        if (remove)
            OpSbcValue(cpu, OpReadM(memory, cpu,
                OpAbs(cpu, (uint16_t)(ITEM_EQUIPMENT_MODIFIERS + offset))));
        else
            OpAdc(memory, cpu, OpAbs(cpu, (uint16_t)(ITEM_EQUIPMENT_MODIFIERS + offset)));
        OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(MEMBER_COMPARISON_MODIFIERS + offset)));
    }
}

Lufia2ExecutionResult Lufia2PartyPrepareEquipmentComparison(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EquipmentComparisonReady(cpu))
        return ExecutionHandoff(cpu, 0x82f76bu);
    ApplyComparisonModifiers(memory, cpu, MEMBER_EQUIPMENT_MODIFIERS, false);
    return ExecutionReturned(0x82f7b3u);
}

Lufia2ExecutionResult Lufia2PartyAddEquipmentComparison(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EquipmentComparisonReady(cpu))
        return ExecutionHandoff(cpu, 0x82f7b4u);
    ApplyComparisonModifiers(memory, cpu, MEMBER_COMPARISON_MODIFIERS, false);
    return ExecutionReturned(0x82f7fcu);
}

Lufia2ExecutionResult Lufia2PartySubtractEquipmentComparison(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EquipmentComparisonReady(cpu))
        return ExecutionHandoff(cpu, 0x82f7fdu);
    ApplyComparisonModifiers(memory, cpu, MEMBER_COMPARISON_MODIFIERS, true);
    return ExecutionReturned(0x82f845u);
}
