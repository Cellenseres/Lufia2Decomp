#ifndef LUFIA2_MENU_SHOP_ROW_DISPATCH_H
#define LUFIA2_MENU_SHOP_ROW_DISPATCH_H

#include "menu/menu_shop_transaction_internal.h"

typedef enum ShopRowKind {
    SHOP_ROW_ITEMS, SHOP_ROW_EQUIPMENT, SHOP_ROW_SPELLS
} ShopRowKind;

static inline uint32_t SelectShopRowKind(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t site,
    uint32_t item_target, uint32_t equipment_target, uint32_t spell_target,
    ShopRowKind *kind) {
    const uint32_t previous_resume = cpu->resume_pc;
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    Lufia2ExecutionResult dispatch = Lufia2MenuDispatchInlineTable(memory, cpu);
    if (dispatch.pc == item_target)
        *kind = SHOP_ROW_ITEMS;
    else if (dispatch.pc == equipment_target)
        *kind = SHOP_ROW_EQUIPMENT;
    else if (dispatch.pc == spell_target)
        *kind = SHOP_ROW_SPELLS;
    else
        return dispatch.pc;
    cpu->resume_pc = previous_resume;
    return 0u;
}

#endif
