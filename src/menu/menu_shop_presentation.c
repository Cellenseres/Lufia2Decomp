#include "menu/menu_shop_row_dispatch.h"

enum {
    DP_SHOP_ROW_TILEMAP = 0x11u,
    DP_SHOP_ROW_COUNT = 0x15u,
    SHOP_ITEM_ROW_MODE = WRAM_BATTLE_PARTY_IDS + 1u
};

Lufia2ExecutionResult Lufia2MenuShopConfigureItemCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82d79du);
    OpLdx(cpu, 0x0077u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
    OpLdx(cpu, 0x0401u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x56u), cpu->x);
    OpLdx(cpu, 5u);
    OpLdy(cpu, 6u);
    SHOP_TX_CALL(0x82d7adu, 0x8289c4u, 2u);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 5u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    SHOP_TX_CALL(0x82d7b8u, 0x8289fau, 2u);
    return ExecutionReturned(0x82d7bbu);
}

Lufia2ExecutionResult Lufia2MenuRefreshShopPreview(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dc10u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
    ShopRowKind kind = SHOP_ROW_ITEMS;
    const uint32_t target = SelectShopRowKind(memory, cpu, 0x82dc13u,
        0x82dc27u, 0x82dc1eu, 0x82dc28u, &kind);
    if (target)
        return ExecutionHandoff(cpu, target);
    if (kind == SHOP_ROW_EQUIPMENT) {
        SHOP_TX_CALL(0x82dc1eu, 0x82e522u, 2u);
        SHOP_TX_CALL(0x82dc21u, 0x82e586u, 2u);
        SHOP_TX_CALL(0x82dc24u, 0x82e5e1u, 2u);
    } else if (kind == SHOP_ROW_SPELLS) {
        OpRepWidths(cpu, 0x20u);
        SHOP_TX_CALL(0x82dc2au, 0x82e691u, 2u);
        OpSepWidths(cpu, 0x20u);
        return ExecutionReturned(0x82dc2fu);
    }
    return ExecutionReturned(0x82dc27u);
}

static void PrepareShopListRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x3488u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_ROW_TILEMAP), cpu->x);
    OpLdx(cpu, 5u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_ROW_COUNT), cpu->x);
}

Lufia2ExecutionResult Lufia2MenuDrawShopList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dca3u);
    ShopRowKind kind = SHOP_ROW_ITEMS;
    const uint32_t target = SelectShopRowKind(memory, cpu, 0x82dca3u,
        0x82dcaeu, 0x82dcc1u, 0x82dccfu, &kind);
    if (target)
        return ExecutionHandoff(cpu, target);
    if (kind == SHOP_ROW_EQUIPMENT)
        return Lufia2MenuShopRows(memory, cpu);
    PrepareShopListRows(memory, cpu);
    if (kind == SHOP_ROW_ITEMS) {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, SHOP_ITEM_ROW_MODE));
        SHOP_TX_CALL(0x82dcbdu, 0x8284d5u, 2u);
        return ExecutionReturned(0x82dcc0u);
    }
    SHOP_TX_CALL(0x82dcd9u, 0x828636u, 2u);
    return ExecutionReturned(0x82dcdcu);
}
