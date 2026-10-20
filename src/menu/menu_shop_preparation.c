#include "menu/menu_shop_transaction_internal.h"

enum {
    DP_SHOP_COUNTS = 4u,
    DP_SHOP_EQUIPPED_COUNT = 5u,
    DP_SHOP_WINDOW = 0x54u,
    DP_SHOP_WINDOW_HEIGHT = 0x56u,
    DP_SHOP_TEXT_BANK = 0x5fu,
    DP_SHOP_UPLOAD_FLAGS = 0x74u,
    SHOP_LIST_ITEMS_LONG = 0x7e93e0u
};

Lufia2ExecutionResult Lufia2MenuShopCountEquippedItems(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page || !child ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82da13u);
    OpStz(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    SHOP_TX_CALL(0x82da1bu, 0x81f1c5u, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x12u)));
    SHOP_TX_CALL(0x82da22u, 0x829918u, 2u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DISPLAY_PRICE), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x0du));
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        SHOP_TX_CALL(0x82da30u, 0x82f6f2u, 2u);
        SHOP_TX_CALL(0x82da33u, 0x82f703u, 2u);
        OpStz(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER));
        do {
            SHOP_TX_CALL(0x82da38u, 0x829971u, 2u);
            OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_PARTY_EQUIPMENT_SELECTED_SLOT)));
            OpLda(memory, cpu, AbsoluteIndexedAddress(cpu,
                Read16Direct(memory, cpu, DP_SHOP_RECORD), cpu->y));
            OpAndValue(cpu, 0x01ffu);
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
            if (cpu->zero)
                OpStepMem(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS), 1);
            OpStepMem(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER), 1);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
            OpAndValue(cpu, 0xffu);
            OpCmp(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER));
        } while (!cpu->zero);
    }
    OpLda(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS));
    return ExecutionReturned(0x82da5bu);
}

Lufia2ExecutionResult Lufia2MenuShopPrepareItemPurchase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82d7bcu);
    OpRepWidths(cpu, 0x20u);
    SHOP_TX_CALL(0x82d7beu, 0x82e709u, 2u);
    PushY(memory, cpu);
    SHOP_TX_CALL(0x82d7c2u, 0x82da13u, 2u);
    OpPullX(memory, cpu);
    OpLda(memory, cpu, OpLongX(cpu, SHOP_LIST_ITEMS_LONG));
    OpAndValue(cpu, 0xffu);
    OpOraValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS)));
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS));
    OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_STATE));
    OpLda(memory, cpu, OpDp(cpu, DP_SHOP_EQUIPPED_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_STATE + 1u));
    OpLoadA(cpu, 99u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_STATE)));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_STATE + 1u)));
    OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
        SHOP_TX_CALL(0x82d7f6u, 0x82fc7du, 2u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SHOP_STOCK_LIMIT));
        OpSepWidths(cpu, 0x20u);
        OpCmp(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
        if (!cpu->carry)
            OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
    }
    OpRepWidths(cpu, 0x20u);
    SHOP_TX_CALL(0x82d808u, 0x82fb04u, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
    OpLoadA(cpu, 0x0486u);
    OpLdx(cpu, 0x1b0cu);
    SHOP_TX_CALL(0x82d814u, 0x8283ebu, 2u);
    OpLoadA(cpu, 0x0442u);
    OpLdx(cpu, 0x1e07u);
    SHOP_TX_CALL(0x82d81du, 0x82810eu, 2u);
    OpLoadA(cpu, 0x0602u);
    OpLdx(cpu, 0x1e03u);
    SHOP_TX_CALL(0x82d826u, 0x82810eu, 2u);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
    OpCmpValue(cpu, 3u);
    bool stock_empty = false;
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SHOP_STOCK_LIMIT));
        stock_empty = cpu->zero;
    }
    if (stock_empty) {
        OpLdy(cpu, 0x10u);
        OpLoadA(cpu, 0u);
    } else {
        OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
        if (cpu->zero) {
            OpLdy(cpu, 7u);
            OpLoadA(cpu, 0u);
        } else {
            OpLda(memory, cpu, OpAbs(cpu, SHOP_PURCHASE_MODE));
            OpCmpValue(cpu, 1u);
            OpLdy(cpu, cpu->zero ? 0x11u : 1u);
            OpLoadA(cpu, 1u);
        }
    }
    OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
    SHOP_TX_CALL(0x82d860u, 0x82de53u, 2u);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_TEXT_BANK));
    OpLdy(cpu, 0xcd41u);
    SHOP_TX_CALL(0x82d86au, 0x808878u, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_DISPLAY_PRICE)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE + 2u));
    SHOP_TX_CALL(0x82d877u, 0x8298c7u, 2u);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_TEXT_BANK));
    OpLdy(cpu, 0xcdaau);
    SHOP_TX_CALL(0x82d881u, 0x808878u, 3u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_MOVE_PHASE));
    OpLdx(cpu, 0x7eu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_WINDOW), cpu->x);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_WINDOW_HEIGHT), cpu->x);
    OpLdx(cpu, 6u);
    OpLdy(cpu, 7u);
    SHOP_TX_CALL(0x82d898u, 0x8289c4u, 2u);
    OpLoadA(cpu, 0x0bu);
    SHOP_TX_CALL(0x82d89du, 0x8293f6u, 2u);
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_SHOP_UPLOAD_FLAGS), 1u);
    return ExecutionReturned(0x82d8a4u);
}

Lufia2ExecutionResult Lufia2MenuShopPrepareItemSale(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82d970u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, 0x01ffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, 0xfe00u);
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
    SHOP_TX_CALL(0x82d98bu, 0x82da13u, 2u);
    OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
    OpAndValue(cpu, 0xffu);
    OpOraValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS)));
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_SHOP_COUNTS));
    OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_STATE));
    OpLda(memory, cpu, OpDp(cpu, DP_SHOP_EQUIPPED_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_STATE + 1u));
    OpRepWidths(cpu, 0x20u);
    SHOP_TX_CALL(0x82d9a6u, 0x82fb04u, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
    OpLoadA(cpu, 0x0486u);
    OpLdx(cpu, 0x1b0cu);
    SHOP_TX_CALL(0x82d9b2u, 0x8283ebu, 2u);
    OpLoadA(cpu, 0x0442u);
    OpLdx(cpu, 0x1e07u);
    SHOP_TX_CALL(0x82d9bbu, 0x82810eu, 2u);
    OpLoadA(cpu, 0x0602u);
    OpLdx(cpu, 0x1e03u);
    SHOP_TX_CALL(0x82d9c4u, 0x82810eu, 2u);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    SHOP_TX_CALL(0x82d9ccu, 0x82de53u, 2u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_TEXT_BANK));
    OpLdy(cpu, 0xcd41u);
    SHOP_TX_CALL(0x82d9dbu, 0x808878u, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_DISPLAY_PRICE)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE + 2u));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_TEXT_BANK));
    OpLdy(cpu, 0xcdaau);
    SHOP_TX_CALL(0x82d9efu, 0x808878u, 3u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_MOVE_PHASE));
    OpLdx(cpu, 0x7eu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_WINDOW), cpu->x);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_WINDOW_HEIGHT), cpu->x);
    OpLdx(cpu, 6u);
    OpLdy(cpu, 7u);
    SHOP_TX_CALL(0x82da06u, 0x8289c4u, 2u);
    OpLoadA(cpu, 0x0bu);
    SHOP_TX_CALL(0x82da0bu, 0x8293f6u, 2u);
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_SHOP_UPLOAD_FLAGS), 1u);
    return ExecutionReturned(0x82da12u);
}
