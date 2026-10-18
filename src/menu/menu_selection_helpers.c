#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"
#include "system/wram.h"

enum { ROM_PARTY_HIGHLIGHTS = 0x8ee576u, DP_EXPERIENCE_DIFFERENCE = 8u };

Lufia2ExecutionResult Lufia2MenuBuildEquipmentAttributes(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82f9c9u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0u);
    do {
        OpRepWidths(cpu, 0x20u);
        OpPushX(memory, cpu);
        OpTxa(cpu);
        if (!MenuActionChild(&call, 0x82f9d0u, 0x82f893u, 2u, 0u))
            return call.result;
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, cpu->carry ? 5u : 7u);
        OpPullX(memory, cpu);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_EQUIPMENT_ATTRIBUTES));
        OpInx(cpu);
        OpCpx(cpu, 6u);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f9e7u);
}

Lufia2ExecutionResult Lufia2MenuHighlightPartySelection(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829984u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_PARTY_HIGHLIGHTS));
    OpLdx(cpu, 0x0f05u);
    OpLdy(cpu, 2u);
    if (!MenuActionChild(&call, 0x829995u, 0x8280cau, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82999au);
}

Lufia2ExecutionResult Lufia2MenuSubtractExperienceTotals(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82942fu);
    for (uint8_t byte = 0u; byte < 3u; ++byte) {
        OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(0x62u + byte)));
        if (!byte)
            cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, (uint16_t)(0x5fu + byte))));
        OpSta(memory, cpu, OpDp(cpu, (uint8_t)(DP_EXPERIENCE_DIFFERENCE + byte)));
    }
    return ExecutionReturned(0x829448u);
}
