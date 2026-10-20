#include "menu/menu_shop_transaction_internal.h"

enum {
    DP_SHOP_ROW_TILEMAP = 0x11u,
    DP_SHOP_ROW_COUNT = 0x15u,
    SHOP_ROW_TILEMAP = 0x3708u,
    SHOP_ITEM_ROW_MODE = WRAM_BATTLE_PARTY_IDS + 1u
};

static bool ShopRowReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void PrepareShopRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, SHOP_ROW_TILEMAP);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_ROW_TILEMAP), cpu->x);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_ROW_COUNT), cpu->x);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2MenuShopDrawItemRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopRowReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dcddu);
    PrepareShopRow(memory, cpu);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SHOP_ITEM_ROW_MODE));
    SHOP_TX_CALL(0x82dceeu, 0x8284d5u, 2u);
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82dcf3u);
}

Lufia2ExecutionResult Lufia2MenuShopDrawSpellRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopRowReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82dd06u);
    PrepareShopRow(memory, cpu);
    SHOP_TX_CALL(0x82dd12u, 0x828636u, 2u);
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82dd17u);
}
