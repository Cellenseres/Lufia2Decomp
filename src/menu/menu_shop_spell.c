#include "menu/menu_shop_transaction_internal.h"

Lufia2ExecutionResult Lufia2MenuShopStoreLearnedSpell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82e1deu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_SHOP_RECORD));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x96u);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_RECORD));
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    do {
        const uint32_t spell = AbsoluteIndexedAddress(
            cpu, Read16Direct(memory, cpu, DP_SHOP_RECORD), cpu->y);
        OpLda(memory, cpu, spell);
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
            OpSta(memory, cpu, AbsoluteIndexedAddress(
                cpu, Read16Direct(memory, cpu, DP_SHOP_RECORD), cpu->y));
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER)));
            OpLoadA(cpu, 3u);
            OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
            SHOP_TX_CALL(0x82e1ffu, 0x868cf5u, 3u);
            break;
        }
        OpIny(cpu);
        OpCpy(cpu, 0x24u);
    } while (!cpu->zero);
    return ExecutionReturned(0x82e20bu);
}

Lufia2ExecutionResult Lufia2MenuShopLearnSpell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82e167u);
    OpLoadA(cpu, 2u);
    SHOP_TX_CALL(0x82e169u, 0x80953bu, 3u);
    SHOP_TX_CALL(0x82e16du, 0x82d8d4u, 2u);
    for (;;) {
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_DIALOG_MODE));
        SHOP_TX_CALL(0x82e175u, 0x829246u, 2u);
        if (cpu->carry)
            break;
        OpLoadA(cpu, 0u);
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbs(cpu, SHOP_SELECTED_MEMBER));
        OpTax(cpu);
        OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER), cpu->x);
        OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ANIMATION));
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            OpLdy(cpu, 0x800cu);
            SHOP_TX_CALL(0x82e1b6u, 0x82de53u, 2u);
            SHOP_TX_CALL(0x82e1b9u, 0x8297e3u, 2u);
            continue;
        }
        OpCmpValue(cpu, 3u);
        if (cpu->zero) {
            OpLdy(cpu, 0x800eu);
            SHOP_TX_CALL(0x82e1c1u, 0x82de53u, 2u);
            SHOP_TX_CALL(0x82e1c4u, 0x8297e3u, 2u);
            continue;
        }
        SHOP_TX_CALL(0x82e18eu, 0x829855u, 2u);
        if (cpu->carry) {
            SHOP_TX_CALL(0x82e193u, 0x82983du, 2u);
            SHOP_TX_CALL(0x82e196u, 0x829971u, 2u);
            SHOP_TX_CALL(0x82e199u, 0x82e1deu, 2u);
            SHOP_TX_CALL(0x82e1d9u, 0x82de75u, 2u);
        } else {
            OpLdy(cpu, 0x8006u);
            SHOP_TX_CALL(0x82e1ccu, 0x82de53u, 2u);
            SHOP_TX_CALL(0x82e1cfu, 0x8297e3u, 2u);
            OpLoadA(cpu, 0x18u);
            SHOP_TX_CALL(0x82e1d4u, 0x829330u, 2u);
        }
        break;
    }
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 6u);
    SHOP_TX_CALL(0x82e1a3u, 0x82895bu, 2u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + 6u));
    OpStz(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + 7u));
    OpStz(memory, cpu, OpAbs(cpu, MENU_SPRITE_ACTIVE + 8u));
    cpu->carry = 0u;
    return ExecutionReturned(0x82e1b2u);
}
