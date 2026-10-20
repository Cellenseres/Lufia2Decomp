#include "menu/menu_shop_transaction_internal.h"

Lufia2ExecutionResult Lufia2MenuShopPreviewEquipment(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dfc4u);
    SHOP_TX_CALL(0x82dfc4u, 0x82d8a5u, 2u);
    for (;;) {
        OpLoadA(cpu, 0x24u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_DIALOG_MODE));
        SHOP_TX_CALL(0x82dfccu, 0x829246u, 2u);
        if (cpu->carry) {
            cpu->carry = 0u;
            return ExecutionReturned(0x82dfd8u);
        }
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82dfd6u);
        }
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX)));
        OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ANIMATION));
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLdy(cpu, 0x8009u);
            SHOP_TX_CALL(0x82dfe6u, 0x82de53u, 2u);
            SHOP_TX_CALL(0x82dfe9u, 0x8297e3u, 2u);
            continue;
        }
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
        OpSta(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER));
        SHOP_TX_CALL(0x82dff3u, 0x829971u, 2u);
        OpRepWidths(cpu, 0x20u);
        SHOP_TX_CALL(0x82dff8u, 0x82f6f2u, 2u);
        SHOP_TX_CALL(0x82dffbu, 0x82f893u, 2u);
        OpSepWidths(cpu, 0x20u);
        if (cpu->carry) {
            OpLdy(cpu, 0x800au);
            SHOP_TX_CALL(0x82e005u, 0x82de53u, 2u);
            SHOP_TX_CALL(0x82e008u, 0x8297e3u, 2u);
            continue;
        }
        SHOP_TX_CALL(0x82e00du, 0x82e0f8u, 2u);
        cpu->carry = 0u;
        return ExecutionReturned(0x82e011u);
    }
}

Lufia2ExecutionResult Lufia2MenuShopEquipPurchase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82e0f8u);
    OpRepWidths(cpu, 0x20u);
    SHOP_TX_CALL(0x82e0fau, 0x82f97du, 2u);
    OpSepWidths(cpu, 0x20u);
    if (cpu->carry) {
        OpLdy(cpu, 0x9808u);
        SHOP_TX_CALL(0x82e104u, 0x82de53u, 2u);
        return ExecutionReturned(0x82e107u);
    }
    OpLda(memory, cpu, OpAbs(cpu, SHOP_PURCHASE_MODE));
    SHOP_TX_CALL(0x82e10bu, 0x82e290u, 2u);
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SHOP_SELECTED_MEMBER)));
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 3u);
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    SHOP_TX_CALL(0x82e11au, 0x868cf5u, 3u);
    SHOP_TX_CALL(0x82e11eu, 0x82de75u, 2u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SHOP_SELECTED_MEMBER));
    OpAndValue(cpu, 0xffu);
    for (unsigned shift = 0; shift < 3u; ++shift)
        OpAslA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x018au);
    OpLdx(cpu, 0x0407u);
    SHOP_TX_CALL(0x82e133u, 0x8283ebu, 2u);
    OpSepWidths(cpu, 0x20u);
    SHOP_TX_CALL(0x82e138u, 0x82994eu, 3u);
    SHOP_TX_CALL(0x82e13cu, 0x82e4d5u, 2u);
    return ExecutionReturned(0x82e13fu);
}

Lufia2ExecutionResult Lufia2MenuShopReceiveItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82e140u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    SHOP_TX_CALL(0x82e145u, 0x82fb1fu, 3u);
    if (!cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
        SHOP_TX_CALL(0x82e14eu, 0x82faa0u, 2u);
    } else {
        SHOP_TX_CALL(0x82e153u, 0x82fb94u, 2u);
        if (!cpu->carry) {
            OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
            OpAndValue(cpu, 0xffu);
            OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
            SHOP_TX_CALL(0x82e161u, 0x82f9e8u, 2u);
        }
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82e166u);
}
