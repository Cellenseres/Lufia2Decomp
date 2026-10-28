#include "lufia2/menu_scenario_items.h"
#include "menu/menu_screen_child_internal.h"
#include "system/wram.h"

static bool ScenarioItemChild(MenuActionCall *call, uint32_t site,
                               uint32_t target, uint8_t width) {
    return MenuScreenChild(call, site, target, 2u, width, 0u);
}

static bool DescribeScenarioItem(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    OpTax(cpu);
    if (!ScenarioItemChild(call, 0x82a900u, 0x82fc3fu, 0u))
        return false;
    if (!cpu->carry) {
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_ID));
        OpSepWidths(cpu, 0x20u);
        OpLdy(cpu, 0x030cu);
        OpLdx(cpu, 1u);
        if (!ScenarioItemChild(call, 0x82a910u, 0x82fd81u, 1u))
            return false;
    }
    OpSepWidths(cpu, 0x20u);
    return true;
}

static bool PageScenarioItems(MenuActionCall *call) {
    Lufia2CpuState *cpu = call->cpu;
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
    OpLsrA(cpu);
    if (!ScenarioItemChild(call, 0x82a8dau, 0x828ce9u, 1u))
        return false;
    return cpu->carry ||
        ScenarioItemChild(call, 0x82a8dfu, 0x82a918u, 1u);
}

Lufia2ExecutionResult Lufia2MenuRunScenarioItems(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82a88eu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0xffu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SECOND_SELECTION), cpu->x);
    if (!ScenarioItemChild(&call, 0x82a897u, 0x829b51u, 1u))
        return call.result;
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!ScenarioItemChild(&call, 0x82a89cu, 0x828b08u, 1u))
            return call.result;
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            break;
        OpCmpValue(cpu, 4u);
        if (cpu->zero) {
            if (!DescribeScenarioItem(&call))
                return call.result;
            OpLoadA(cpu, 0u);
            continue;
        }
        OpCmpValue(cpu, 6u);
        if (cpu->zero) {
            if (!PageScenarioItems(&call))
                return call.result;
        } else {
            OpCmpValue(cpu, 7u);
            if (cpu->zero) {
                if (!PageScenarioItems(&call))
                    return call.result;
            } else {
                OpCmpValue(cpu, 10u);
                if (cpu->zero) {
                    if (!ScenarioItemChild(&call, 0x82a8e4u, 0x82ad20u, 1u))
                        return call.result;
                } else {
                    OpCmpValue(cpu, 11u);
                    if (!cpu->zero) {
                        OpLoadA(cpu, 0u);
                        continue;
                    }
                    if (!ScenarioItemChild(&call, 0x82a8e9u, 0x82adb4u, 1u))
                        return call.result;
                }
            }
        }
        if (!ScenarioItemChild(&call, 0x82a8ecu, 0x828c43u, 1u))
            return call.result;
        OpLoadA(cpu, 8u);
        TestBitsDirect(memory, cpu, 0x74u, 1u);
        OpLoadA(cpu, 1u);
    }
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0342u);
    OpLdx(cpu, 0x1e13u);
    if (!ScenarioItemChild(&call, 0x82a8c7u, 0x8283b5u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    if (!ScenarioItemChild(&call, 0x82a8ccu, 0x8293c2u, 1u) ||
        !ScenarioItemChild(&call, 0x82a8cfu, 0x829b10u, 1u))
        return call.result;
    OpLoadA(cpu, 0x88u);
    TestBitsDirect(memory, cpu, 0x74u, 1u);
    return ExecutionReturned(0x82a8d6u);
}
