#include "menu/menu_shop_transaction_internal.h"

enum { DP_SHOP_LIST = 0xfcu };

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
