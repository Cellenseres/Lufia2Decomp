#include "lufia2/item.h"
#include "menu/menu_action_internal.h"
#include "system/wram.h"

enum { INVENTORY_SWAP_VALUE = 0x54u, INVENTORY_NO_DISCARD = 0x0020u };

Lufia2ExecutionResult Lufia2InventoryReadSelectedItem(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!MenuActionReady(cpu, child, 0u))
        return ExecutionHandoff(cpu, 0x82fbeeu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    if (cpu->zero) {
        cpu->carry = 1u;
        return ExecutionReturned(0x82fc07u);
    }
    OpAndValue(cpu, 0x01ffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    if (!MenuActionChild(&call, 0x82fbfeu, 0x81f1c5u, 3u, 0u))
        return call.result;
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    cpu->carry = 0u;
    return ExecutionReturned(0x82fc05u);
}

Lufia2ExecutionResult Lufia2InventorySwapSelectedItems(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 0u))
        return ExecutionHandoff(cpu, 0x82fc08u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION)));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SECOND_SELECTION)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpSta(memory, cpu, OpDp(cpu, INVENTORY_SWAP_VALUE));
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpLda(memory, cpu, OpDp(cpu, INVENTORY_SWAP_VALUE));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    return ExecutionReturned(0x82fc1eu);
}

Lufia2ExecutionResult Lufia2InventoryDiscardSelectedItem(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!MenuActionReady(cpu, child, 0u))
        return ExecutionHandoff(cpu, 0x82fc1fu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!MenuActionChild(&call, 0x82fc1fu, 0x82fbeeu, 2u, 0u))
        return call.result;
    if (!cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x0du));
        OpBitValue(cpu, INVENTORY_NO_DISCARD);
        if (cpu->zero) {
            if (!MenuActionChild(&call, 0x82fc2cu, 0x82faf4u, 2u, 0u))
                return call.result;
            OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID)));
            if (!MenuActionChild(&call, 0x82fc32u, 0x82fc97u, 2u, 0u))
                return call.result;
            OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION)));
            OpStz(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
            cpu->carry = 0u;
            return ExecutionReturned(0x82fc3cu);
        }
    }
    cpu->carry = 1u;
    return ExecutionReturned(0x82fc3eu);
}
