#include "menu/menu_shop_row_dispatch.h"

enum {
    DP_SHOP_LIST = 0xfcu,
    SHOP_ITEM_ID_MASK = 0x01ffu,
    SHOP_INFO_SPELL_FLAG = 0x0200u
};

static bool ShopListReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2MenuShopSelectSpell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ShopListReady(cpu))
        return ExecutionHandoff(cpu, 0x82e6fcu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION)));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SHOP_LIST));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    cpu->carry = 0u;
    return ExecutionReturned(0x82e708u);
}

Lufia2ExecutionResult Lufia2MenuShopSelectItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ShopListReady(cpu))
        return ExecutionHandoff(cpu, 0x82e709u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
    OpAslA(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SHOP_LIST));
    OpAndValue(cpu, 0x01ffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    cpu->carry = 0u;
    return ExecutionReturned(0x82e717u);
}

Lufia2ExecutionResult Lufia2MenuShopSelectInventoryItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x82e718u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
    OpAslA(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, SHOP_ITEM_ID_MASK);
    cpu->carry = cpu->zero;
    return ExecutionReturned(cpu->zero ? 0x82e728u : 0x82e726u);
}

Lufia2ExecutionResult Lufia2MenuShopReadSelectedRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dc49u);
    ShopRowKind kind = SHOP_ROW_ITEMS;
    const uint32_t target = SelectShopRowKind(memory, cpu, 0x82dc49u,
        0x82dc54u, 0x82dc5au, 0x82dc60u, &kind);
    if (target)
        return ExecutionHandoff(cpu, target);
    OpRepWidths(cpu, 0x20u);
    if (kind == SHOP_ROW_ITEMS) {
        SHOP_TX_CALL(0x82dc56u, 0x82e718u, 2u);
        return ExecutionReturned(0x82dc59u);
    }
    if (kind == SHOP_ROW_EQUIPMENT) {
        SHOP_TX_CALL(0x82dc5cu, 0x82e709u, 2u);
        return ExecutionReturned(0x82dc5fu);
    }
    SHOP_TX_CALL(0x82dc62u, 0x82e6fcu, 2u);
    OpOraValue(cpu, SHOP_INFO_SPELL_FLAG);
    return ExecutionReturned(0x82dc68u);
}
