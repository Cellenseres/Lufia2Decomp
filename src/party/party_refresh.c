#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "system/wram.h"

enum {
    DP_ACTIVE_EQUIPMENT_SLOT = 0x22u,
    DP_ACTIVE_EQUIPMENT_MEMBER = 0x2au
};

static Lufia2ExecutionResult RefreshChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2PartySelectActiveEquipmentMember(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x829971u);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpDp(cpu, DP_ACTIVE_EQUIPMENT_SLOT));
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
    OpSta(memory, cpu, OpDp(cpu, DP_ACTIVE_EQUIPMENT_MEMBER));
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    return ExecutionReturned(0x829983u);
}

Lufia2ExecutionResult Lufia2PartyRefreshActiveEquipmentStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82e893u);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, DP_ACTIVE_EQUIPMENT_SLOT));
    do {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e897u, 0x829971u, 2u, cpu->program_bank))
            return RefreshChildUnwound(0x82e897u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e89au, 0x82f846u, 2u, cpu->program_bank))
            return RefreshChildUnwound(0x82e89au);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTIVE_EQUIPMENT_MEMBER)));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82e89fu, 0x81f4d5u, 3u, cpu->program_bank))
            return RefreshChildUnwound(0x82e89fu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_ACTIVE_EQUIPMENT_SLOT), 1);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
        OpAndValue(cpu, 0xffu);
        OpCmp(memory, cpu, OpDp(cpu, DP_ACTIVE_EQUIPMENT_SLOT));
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82e8b1u);
}
