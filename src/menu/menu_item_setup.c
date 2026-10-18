#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"
#include "system/wram.h"

enum {
    ITEM_TEXT_POINTER = 0x5du,
    ITEM_TEXT_BANK = 0x5fu,
    ITEM_CURSOR_GRID = 0x54u,
    ITEM_CURSOR_STYLE = 0x56u,
    ROM_ITEM_SELECTION_TEXT = 0xd8f3u
};

static bool PlaceItemWindows(MenuActionCall *call) {
    static const uint16_t positions[] = {0x0342u, 0x035au, 0x0402u};
    static const uint16_t sizes[] = {0x0c03u, 0x1203u, 0x1e0bu};
    for (unsigned window = 0u; window < 3u; ++window) {
        OpLoadA(call->cpu, positions[window]);
        OpLdx(call->cpu, sizes[window]);
        if (!MenuActionChild(call, 0x829d79u + window * 9u, 0x82810eu, 2u, 0u))
            return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2MenuPrepareItemSelection(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829d55u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_WINDOW_REFRESH));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
    OpLdx(cpu, 0xfff0u);
    if (!MenuActionChild(&call, 0x829d5eu, 0x829214u, 3u, 1u))
        return call.result;
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, ITEM_TEXT_BANK));
    OpLdx(cpu, ROM_ITEM_SELECTION_TEXT);
    OpWriteX(memory, cpu, OpDp(cpu, ITEM_TEXT_POINTER), cpu->x);
    if (!MenuActionChild(&call, 0x829d6bu, 0x8293cfu, 2u, 1u) ||
        !MenuActionChild(&call, 0x829d6eu, 0x829dd3u, 2u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    if (!PlaceItemWindows(&call))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 4u);
    if (!MenuActionChild(&call, 0x829d92u, 0x82a318u, 2u, 1u))
        return call.result;
    OpLdx(cpu, 0xe0u);
    OpWriteX(memory, cpu, OpDp(cpu, ITEM_CURSOR_GRID), cpu->x);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, ITEM_CURSOR_STYLE), cpu->x);
    OpLdx(cpu, 6u);
    OpLdy(cpu, 7u);
    if (!MenuActionChild(&call, 0x829da5u, 0x8289c4u, 2u, 1u))
        return call.result;
    OpLdx(cpu, 2u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_MENU_ITEM_ROW));
    if (!MenuActionChild(&call, 0x829db0u, 0x8288cbu, 2u, 1u))
        return call.result;
    OpLdy(cpu, 0xf08au);
    if (!MenuActionChild(&call, 0x829db6u, 0x828c9cu, 2u, 1u))
        return call.result;
    OpLdx(cpu, 0x4800u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_TRACK), cpu->x);
    OpLdx(cpu, 0x60u);
    OpLdy(cpu, 0x5au);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW), cpu->y);
    cpu->carry = 0u;
    if (!MenuActionChild(&call, 0x829dc9u, 0x828c85u, 2u, 1u) ||
        !MenuActionChild(&call, 0x829dccu, 0x82ac84u, 2u, 1u) ||
        !MenuActionChild(&call, 0x829dcfu, 0x828704u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x829dd2u);
}

Lufia2ExecutionResult Lufia2MenuRefreshItemSelection(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829dd3u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 8u);
    if (!MenuActionChild(&call, 0x829dd6u, 0x868d47u, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x6au);
    OpLdx(cpu, 0x0a0bu);
    if (!MenuActionChild(&call, 0x829de2u, 0x82810eu, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    if (!MenuActionChild(&call, 0x829de7u, 0x829e2fu, 2u, 1u) ||
        !MenuActionChild(&call, 0x829deau, 0x82999bu, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0x10u);
    if (!MenuActionChild(&call, 0x829defu, 0x8293f6u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x829df2u);
}
