#include <stddef.h>

#include "menu/menu_shop_transaction_internal.h"

typedef Lufia2ExecutionResult (*ShopOperation)(
    const Lufia2Memory *, Lufia2CpuState *, Lufia2PushedChildCall, void *);

typedef struct ShopOperationRoute {
    uint32_t target;
    ShopOperation run;
} ShopOperationRoute;

static bool ShopOperationReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult ResolveShopOperation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t table_return) {
    SimulateJsrFrame(memory, cpu, table_return);
    return Lufia2MenuDispatchInlineTable(memory, cpu);
}

static Lufia2ExecutionResult RunShopOperation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint16_t table_return,
    const ShopOperationRoute *routes, size_t route_count) {
    const Lufia2ExecutionResult selected =
        ResolveShopOperation(memory, cpu, table_return);
    for (size_t i = 0; i < route_count; ++i) {
        if (selected.pc == routes[i].target)
            return routes[i].run(memory, cpu, child, context);
    }
    return selected;
}

Lufia2ExecutionResult Lufia2MenuSelectShopOperation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const ShopOperationRoute routes[] = {
        {0x82dea4u, Lufia2MenuShopSellItem},
        {0x82df45u, Lufia2MenuShopChoosePurchaseQuantity},
        {0x82e167u, Lufia2MenuShopLearnSpell}
    };
    if (!child || !ShopOperationReady(cpu))
        return ExecutionHandoff(cpu, 0x82dbc9u);
    return RunShopOperation(memory, cpu, child, context, 0xdbcbu,
        routes, sizeof(routes) / sizeof(routes[0]));
}

Lufia2ExecutionResult Lufia2MenuPurchaseShopSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const ShopOperationRoute routes[] = {
        {0x82e02cu, Lufia2MenuShopBuyItem},
        {0x82e0a9u, Lufia2MenuShopBuyExchangeUnits}
    };
    if (!child || !ShopOperationReady(cpu))
        return ExecutionHandoff(cpu, 0x82dfa5u);
    return RunShopOperation(memory, cpu, child, context, 0xdfa7u,
        routes, sizeof(routes) / sizeof(routes[0]));
}

Lufia2ExecutionResult Lufia2MenuPayShopSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ShopOperationReady(cpu))
        return ExecutionHandoff(cpu, 0x82e290u);
    const Lufia2ExecutionResult selected =
        ResolveShopOperation(memory, cpu, 0xe292u);
    if (selected.pc == 0x82983du)
        return Lufia2MenuSpendSelectionGold(memory, cpu);
    if (selected.pc == 0x82989du)
        return Lufia2MenuSpendExchangeBalance(memory, cpu);
    return selected;
}
