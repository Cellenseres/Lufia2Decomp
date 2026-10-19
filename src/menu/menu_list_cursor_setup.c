#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"
#include "system/wram.h"

enum {
    LIST_CURSOR_GRID = 0x54u,
    LIST_CURSOR_STYLE = 0x56u,
    LIST_TEXT_BANK = 0x5fu,
    LIST_FRAME_FLAGS = 0x74u
};

Lufia2ExecutionResult Lufia2MenuConfigureListCursor(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829c7du);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 5u);
    if (!MenuActionChild(&call, 0x829c80u, 0x82898du, 2u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MODE));
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        OpLoadA(cpu, 0x20u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, LIST_TEXT_BANK));
        OpLdy(cpu, 0xd57cu);
        if (!MenuActionChild(&call, 0x829c96u, 0x808878u, 3u, 1u))
            return call.result;
        OpLoadA(cpu, 8u);
        OpTestBits(memory, cpu, OpDp(cpu, LIST_FRAME_FLAGS), 1u);
    }
    OpLdx(cpu, 14u);
    OpWriteX(memory, cpu, OpDp(cpu, LIST_CURSOR_GRID), cpu->x);
    OpLdx(cpu, 6u);
    OpWriteX(memory, cpu, OpDp(cpu, LIST_CURSOR_STYLE), cpu->x);
    OpLdx(cpu, 9u);
    OpLdy(cpu, 10u);
    if (!MenuActionChild(&call, 0x829caeu, 0x8289ecu, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x829cb1u);
}

Lufia2ExecutionResult Lufia2MenuConfigureItemUseCursor(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829df3u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 231u);
    OpWriteX(memory, cpu, OpDp(cpu, LIST_CURSOR_GRID), cpu->x);
    OpLdx(cpu, 6u);
    OpWriteX(memory, cpu, OpDp(cpu, LIST_CURSOR_STYLE), cpu->x);
    OpLdx(cpu, 9u);
    OpLdy(cpu, 10u);
    if (!MenuActionChild(&call, 0x829e03u, 0x8289ecu, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x829e06u);
}

Lufia2ExecutionResult
Lufia2MenuConfigurePendingItemTargetCursor(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu,
                                           Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829e07u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0xe7u);
    OpWriteX(memory, cpu, OpDp(cpu, LIST_CURSOR_GRID), cpu->x);
    OpLdx(cpu, 0x0406u);
    OpWriteX(memory, cpu, OpDp(cpu, LIST_CURSOR_STYLE), cpu->x);
    OpLdx(cpu, 9u);
    OpLdy(cpu, 10u);
    if (!MenuActionChild(&call, 0x829e17u, 0x8289ecu, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x829e1au);
}

Lufia2ExecutionResult
Lufia2MenuConfigurePendingItemActionCursor(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu,
                                           Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829e1bu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0xeeu);
    OpWriteX(memory, cpu, OpDp(cpu, LIST_CURSOR_GRID), cpu->x);
    OpLdx(cpu, 0x0401u);
    OpWriteX(memory, cpu, OpDp(cpu, LIST_CURSOR_STYLE), cpu->x);
    OpLdx(cpu, 7u);
    OpLdy(cpu, 9u);
    if (!MenuActionChild(&call, 0x829e2bu, 0x8289ecu, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x829e2eu);
}
