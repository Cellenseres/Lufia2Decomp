#include "menu/menu_shop_transaction_internal.h"

enum { DP_SHOP_ROW = 0u, SHOP_SCROLL_REQUEST = WRAM_MENU_DISPLAY_REQUESTS + 1u };

typedef enum ShopRowKind {
    SHOP_ROW_ITEMS, SHOP_ROW_EQUIPMENT, SHOP_ROW_SPELLS
} ShopRowKind;

static uint32_t SelectShopRowKind(
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

Lufia2ExecutionResult Lufia2MenuShopScrollUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dd18u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    if (cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        return ExecutionReturned(0x82ddadu);
    }
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
    ShopRowKind kind = SHOP_ROW_ITEMS;
    const uint32_t target = SelectShopRowKind(memory, cpu, 0x82dd27u,
        0x82dd32u, 0x82dd41u, 0x82dd50u, &kind);
    if (target)
        return ExecutionHandoff(cpu, target);
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), -1);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_ROW), cpu->x);
    switch (kind) {
    case SHOP_ROW_ITEMS:
        SHOP_TX_CALL(0x82dd3cu, 0x82dcddu, 2u);
        break;
    case SHOP_ROW_EQUIPMENT:
        SHOP_TX_CALL(0x82dd4bu, 0x82dcf4u, 2u);
        break;
    case SHOP_ROW_SPELLS:
        SHOP_TX_CALL(0x82dd5au, 0x82dd06u, 2u);
        SHOP_TX_CALL(0x82dd5du, 0x82e691u, 2u);
        break;
    }
    OpSepWidths(cpu, 0x20u);
    if (kind == SHOP_ROW_EQUIPMENT) {
        SHOP_TX_CALL(0x82dd7au, 0x82e522u, 2u);
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
        SHOP_TX_CALL(0x82dd82u, 0x868b55u, 3u);
        SHOP_TX_CALL(0x82dd86u, 0x82e586u, 2u);
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
        SHOP_TX_CALL(0x82dd8eu, 0x868b55u, 3u);
        SHOP_TX_CALL(0x82dd92u, 0x82e5e1u, 2u);
    } else {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
        SHOP_TX_CALL(0x82dd69u, 0x868b55u, 3u);
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
        SHOP_TX_CALL(0x82dd72u, 0x868b55u, 3u);
    }
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
    SHOP_TX_CALL(0x82dd9au, 0x868b55u, 3u);
    SHOP_TX_CALL(0x82dd9eu, 0x828f5eu, 2u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), -1);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82ddadu);
}

Lufia2ExecutionResult Lufia2MenuShopScrollDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82ddaeu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_OVERFLOW));
    if (!cpu->negative) {
        OpSepWidths(cpu, 0x20u);
        return ExecutionReturned(0x82de52u);
    }
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
    ShopRowKind kind = SHOP_ROW_ITEMS;
    const uint32_t target = SelectShopRowKind(memory, cpu, 0x82ddc0u,
        0x82ddcbu, 0x82dddeu, 0x82ddf1u, &kind);
    if (target)
        return ExecutionHandoff(cpu, target);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_ROW));
    switch (kind) {
    case SHOP_ROW_ITEMS:
        SHOP_TX_CALL(0x82ddd6u, 0x82dcddu, 2u);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), 1);
        break;
    case SHOP_ROW_EQUIPMENT:
        SHOP_TX_CALL(0x82dde9u, 0x82dcf4u, 2u);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), 1);
        break;
    case SHOP_ROW_SPELLS:
        SHOP_TX_CALL(0x82ddfcu, 0x82dd06u, 2u);
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_TOP_ROW), 1);
        SHOP_TX_CALL(0x82de02u, 0x82e691u, 2u);
        break;
    }
    OpSepWidths(cpu, 0x20u);
    if (kind == SHOP_ROW_EQUIPMENT) {
        SHOP_TX_CALL(0x82de1fu, 0x82e522u, 2u);
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
        SHOP_TX_CALL(0x82de27u, 0x868b55u, 3u);
        SHOP_TX_CALL(0x82de2bu, 0x82e586u, 2u);
        OpLoadA(cpu, 0x10u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
        SHOP_TX_CALL(0x82de33u, 0x868b55u, 3u);
        SHOP_TX_CALL(0x82de37u, 0x82e5e1u, 2u);
    } else {
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
        SHOP_TX_CALL(0x82de0eu, 0x868b55u, 3u);
        OpLoadA(cpu, 0x10u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
        SHOP_TX_CALL(0x82de17u, 0x868b55u, 3u);
    }
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
    SHOP_TX_CALL(0x82de3fu, 0x868b55u, 3u);
    SHOP_TX_CALL(0x82de43u, 0x829009u, 2u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SHOP_SCROLL_REQUEST));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_MENU_SCROLL_ROW), 1);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82de52u);
}
