#include "menu/menu_shop_transaction_internal.h"

enum { DP_SHOP_TEXT_BANK = 0x5fu, DP_SHOP_WINDOW = 0x54u,
       DP_SHOP_WINDOW_HEIGHT = 0x56u, DP_SHOP_UPLOAD_FLAGS = 0x74u };

Lufia2ExecutionResult Lufia2MenuShopPrepareEquipmentPreview(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82d8a5u);
    OpLdy(cpu, 3u);
    SHOP_TX_CALL(0x82d8a8u, 0x82de53u, 2u);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_TEXT_BANK));
    OpLdy(cpu, 0xd5abu);
    SHOP_TX_CALL(0x82d8b2u, 0x808878u, 3u);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_SHOP_UPLOAD_FLAGS), 1u);
    OpLdx(cpu, 0x85u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_WINDOW), cpu->x);
    OpLdx(cpu, 0x0406u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_WINDOW_HEIGHT), cpu->x);
    OpLdx(cpu, 7u);
    OpLdy(cpu, 8u);
    SHOP_TX_CALL(0x82d8cau, 0x8289ecu, 2u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MEMBER_COUNT));
    return ExecutionReturned(0x82d8d3u);
}

Lufia2ExecutionResult Lufia2MenuShopPrepareSpellPurchase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82d8d4u);
    OpRepWidths(cpu, 0x20u);
    SHOP_TX_CALL(0x82d8d6u, 0x82e6fcu, 2u);
    OpLoadA(cpu, 0x0486u);
    OpLdx(cpu, 0x1b0cu);
    SHOP_TX_CALL(0x82d8dfu, 0x8283ebu, 2u);
    OpLoadA(cpu, 0x0442u);
    OpLdx(cpu, 0x1e07u);
    SHOP_TX_CALL(0x82d8e8u, 0x82810eu, 2u);
    OpLoadA(cpu, 0x0602u);
    OpLdx(cpu, 0x1e03u);
    SHOP_TX_CALL(0x82d8f1u, 0x82810eu, 2u);
    OpSepWidths(cpu, 0x20u);
    SHOP_TX_CALL(0x82d8f6u, 0x82d905u, 2u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 7u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    SHOP_TX_CALL(0x82d901u, 0x8289fau, 2u);
    return ExecutionReturned(0x82d904u);
}
