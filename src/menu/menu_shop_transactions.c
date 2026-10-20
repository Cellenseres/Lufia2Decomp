#include "menu/menu_shop_transaction_internal.h"

static void SelectShopInventorySlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
    OpAslA(cpu);
    OpTax(cpu);
}

Lufia2ExecutionResult Lufia2MenuShopSellItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dea4u);
    OpRepWidths(cpu, 0x20u);
    SelectShopInventorySlot(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpSepWidths(cpu, 0x20u);
    bool cannot_sell = cpu->zero;
    if (!cannot_sell) {
        SHOP_TX_CALL(0x82deb5u, 0x81f194u, 3u);
        OpBitValue(cpu, 0x20u);
        cannot_sell = !cpu->zero;
    }
    if (cannot_sell) {
        SHOP_TX_CALL(0x82debfu, 0x8297e3u, 2u);
        cpu->carry = 1u;
        return ExecutionReturned(0x82dec3u);
    }
    OpLoadA(cpu, 2u);
    SHOP_TX_CALL(0x82dec6u, 0x80953bu, 3u);
    SHOP_TX_CALL(0x82decau, 0x82d970u, 2u);
    uint8_t refresh = 0u;
    for (;;) {
        OpLoadA(cpu, refresh);
        SHOP_TX_CALL(0x82decfu, 0x828b08u, 2u);
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            SHOP_TX_CALL(0x82deeau, 0x82e22cu, 2u);
            refresh = 0u;
            continue;
        }
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            SHOP_TX_CALL(0x82def1u, 0x80953bu, 3u);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
            OpAslA(cpu);
            OpTay(cpu);
            OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, WRAM_INVENTORY_PACKED_ITEMS)));
            OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
            SHOP_TX_CALL(0x82df02u, 0x82fc97u, 2u);
            SelectShopInventorySlot(memory, cpu);
            OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
            SHOP_TX_CALL(0x82df0du, 0x82fad0u, 2u);
            OpSepWidths(cpu, 0x20u);
            SHOP_TX_CALL(0x82df12u, 0x829808u, 2u);
            SHOP_TX_CALL(0x82df15u, 0x82e433u, 2u);
            SHOP_TX_CALL(0x82df18u, 0x82de75u, 2u);
            break;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            break;
        OpCmpValue(cpu, 4u);
        if (cpu->zero) {
            OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
            OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_ID), cpu->x);
            OpLdx(cpu, 0u);
            OpLdy(cpu, 0x0b0fu);
            SHOP_TX_CALL(0x82df3fu, 0x82fd81u, 2u);
        }
        refresh = 0u;
    }
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 6u);
    SHOP_TX_CALL(0x82df23u, 0x82895bu, 2u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + 6u));
    OpStz(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + 7u));
    OpStz(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + 8u));
    cpu->carry = 0u;
    return ExecutionReturned(0x82df32u);
}

Lufia2ExecutionResult Lufia2MenuShopChoosePurchaseQuantity(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82df45u);
    OpLoadA(cpu, 2u);
    SHOP_TX_CALL(0x82df47u, 0x80953bu, 3u);
    SHOP_TX_CALL(0x82df4bu, 0x82d7bcu, 2u);
    uint8_t refresh = 0u;
    for (;;) {
        OpLoadA(cpu, refresh);
        SHOP_TX_CALL(0x82df50u, 0x828b08u, 2u);
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
            if (!cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, SHOP_PURCHASE_MODE));
                OpCmpValue(cpu, 1u);
                if (!cpu->zero) {
                    SHOP_TX_CALL(0x82dfaeu, 0x82e22cu, 2u);
                    refresh = 0u;
                    continue;
                }
            }
            refresh = 1u;
            continue;
        }
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
            if (cpu->zero) {
                refresh = 1u;
                continue;
            }
            OpLda(memory, cpu, OpAbs(cpu, SHOP_PURCHASE_MODE));
            SHOP_TX_CALL(0x82df93u, 0x82dfa5u, 2u);
            if (cpu->carry) {
                SHOP_TX_CALL(0x82df98u, 0x82de53u, 2u);
                SHOP_TX_CALL(0x82df9bu, 0x8297e3u, 2u);
                OpLoadA(cpu, 0x18u);
                SHOP_TX_CALL(0x82dfa0u, 0x829330u, 2u);
            }
            break;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero)
            break;
        OpCmpValue(cpu, 4u);
        if (cpu->zero) {
            OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
            OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DESCRIPTION_ID), cpu->x);
            OpLdx(cpu, 0u);
            OpLdy(cpu, 0x0b0fu);
            SHOP_TX_CALL(0x82dfbfu, 0x82fd81u, 2u);
        }
        refresh = 0u;
    }
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 7u);
    OpLdy(cpu, 6u);
    SHOP_TX_CALL(0x82df88u, 0x8289bcu, 2u);
    OpStz(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + 8u));
    cpu->carry = 0u;
    return ExecutionReturned(0x82df8fu);
}

Lufia2ExecutionResult Lufia2MenuShopBuyItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82e02cu);
    OpLda(memory, cpu, OpAbs(cpu, SHOP_PURCHASE_MODE));
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        SHOP_TX_CALL(0x82e033u, 0x829855u, 2u);
        if (!cpu->carry) {
            OpLdy(cpu, 0x8006u);
            cpu->carry = 1u;
            return ExecutionReturned(0x82e0a8u);
        }
    } else {
        SHOP_TX_CALL(0x82e03au, 0x8298b5u, 2u);
        if (!cpu->carry) {
            OpLdy(cpu, 0x8012u);
            cpu->carry = 1u;
            return ExecutionReturned(0x82e0a8u);
        }
    }
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0x0b84u));
    OpBitValue(cpu, 2u);
    OpSepWidths(cpu, 0x20u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            SHOP_TX_CALL(0x82e054u, 0x80953bu, 3u);
            SHOP_TX_CALL(0x82e058u, 0x82dfc4u, 2u);
            Push8(memory, cpu, PackStatus(cpu));
            OpLda(memory, cpu, OpAbs(cpu, SHOP_FLAGS));
            OpBitValue(cpu, 2u);
            if (cpu->zero) {
                OpRepWidths(cpu, 0x20u);
                OpLoadA(cpu, 0x0384u);
                OpLdx(cpu, 0x0702u);
                SHOP_TX_CALL(0x82e06bu, 0x8283ebu, 2u);
                OpSepWidths(cpu, 0x20u);
                OpLoadA(cpu, 8u);
                OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
            }
            UnpackStatus(cpu, Pull8(memory, cpu));
            if (!cpu->carry)
                return ExecutionReturned(0x82e0a8u);
        }
    }
    OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_STATE));
    if (cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY)));
        OpCpx(cpu, 0x60u);
        if (cpu->zero) {
            OpLdy(cpu, 0x8007u);
            cpu->carry = 1u;
            return ExecutionReturned(0x82e0a8u);
        }
    }
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    SHOP_TX_CALL(0x82e089u, 0x82895bu, 2u);
    SHOP_TX_CALL(0x82e08cu, 0x82e140u, 2u);
    OpLda(memory, cpu, OpAbs(cpu, SHOP_PURCHASE_MODE));
    SHOP_TX_CALL(0x82e092u, 0x82e290u, 2u);
    SHOP_TX_CALL(0x82e095u, 0x82de75u, 2u);
    cpu->carry = 0u;
    return ExecutionReturned(0x82e099u);
}
