#include "menu/menu_shop_transaction_internal.h"

Lufia2ExecutionResult Lufia2MenuShopShowMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82de53u);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->y);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, 0x5fu));
    OpLdy(cpu, 0xce07u);
    SHOP_TX_CALL(0x82de61u, 0x808878u, 3u);
    OpLda(memory, cpu, OpDp(cpu, 1u));
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero) {
        OpLoadA(cpu, 8u);
        OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    }
    OpLda(memory, cpu, OpDp(cpu, 1u));
    SHOP_TX_CALL(0x82de71u, 0x829330u, 2u);
    return ExecutionReturned(0x82de74u);
}

Lufia2ExecutionResult Lufia2MenuShopFinishTransaction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82de75u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
        SHOP_TX_CALL(0x82de84u, 0x82fcc7u, 2u);
        OpSepWidths(cpu, 0x20u);
    }
    OpLoadA(cpu, 8u);
    SHOP_TX_CALL(0x82de8bu, 0x80953bu, 3u);
    OpLoadA(cpu, 0x7bu);
    SHOP_TX_CALL(0x82de91u, 0x80953bu, 3u);
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    SHOP_TX_CALL(0x82de9au, 0x82895bu, 2u);
    OpLdy(cpu, 0x9802u);
    SHOP_TX_CALL(0x82dea0u, 0x82de53u, 2u);
    return ExecutionReturned(0x82dea3u);
}

Lufia2ExecutionResult Lufia2MenuShopComputeQuantityPrice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82e20cu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_DISPLAY_PRICE)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_A), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B + 1u));
    SHOP_TX_CALL(0x82e21bu, 0x828000u, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT + 2u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE + 2u));
    return ExecutionReturned(0x82e22bu);
}

Lufia2ExecutionResult Lufia2MenuShopAdjustQuantity(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82e22cu);
    OpLda(memory, cpu, OpDp(cpu, 0x47u));
    OpBitValue(cpu, 8u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
        cpu->carry = 0u;
        OpAdcValue(cpu, 10u);
        OpCmp(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
        if (!cpu->negative)
            OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
    } else {
        OpBitValue(cpu, 4u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
            cpu->carry = 1u;
            OpSbcValue(cpu, 10u);
            OpCmpValue(cpu, 1u);
            if (cpu->negative)
                OpLoadA(cpu, 1u);
        } else {
            OpBitValue(cpu, 1u);
            if (!cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
                OpIncA(cpu);
                OpCmp(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
                if (!cpu->negative)
                    OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY_LIMIT));
            } else {
                OpBitValue(cpu, 2u);
                if (cpu->zero)
                    return ExecutionReturned(0x82e23eu);
                OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
                OpDecA(cpu);
                OpCmpValue(cpu, 1u);
                if (cpu->negative)
                    OpLoadA(cpu, 1u);
            }
        }
    }
    OpSta(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
    SHOP_TX_CALL(0x82e278u, 0x82e20cu, 2u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, 0x5fu));
    OpLdy(cpu, 0xcdaau);
    SHOP_TX_CALL(0x82e287u, 0x808878u, 3u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
    return ExecutionReturned(0x82e28fu);
}
