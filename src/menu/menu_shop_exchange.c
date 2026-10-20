#include "menu/menu_shop_transaction_internal.h"

enum { EXCHANGE_LIMIT_LOW = 0x967fu, EXCHANGE_LIMIT_HIGH = 0x98u };

static void SubtractExchangePrice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool spend) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_PRICE)));
    if (spend)
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE + 2u));
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_PRICE + 2u)));
    if (spend)
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE + 2u));
}

Lufia2ExecutionResult Lufia2MenuCreditExchangeBalance(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ShopTransactionReady(cpu))
        return ExecutionHandoff(cpu, 0x829867u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE));
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_PRICE)));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE + 2u));
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_PRICE + 2u)));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE + 2u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE));
    cpu->carry = 1u;
    OpSbcValue(cpu, EXCHANGE_LIMIT_LOW);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE + 2u));
    OpSbcValue(cpu, EXCHANGE_LIMIT_HIGH);
    if (cpu->carry) {
        OpLdx(cpu, EXCHANGE_LIMIT_LOW);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE), cpu->x);
        OpLoadA(cpu, EXCHANGE_LIMIT_HIGH);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_BALANCE + 2u));
        cpu->carry = 1u;
    }
    return ExecutionReturned(0x82989cu);
}

Lufia2ExecutionResult Lufia2MenuSpendExchangeBalance(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ShopTransactionReady(cpu))
        return ExecutionHandoff(cpu, 0x82989du);
    SubtractExchangePrice(memory, cpu, true);
    return ExecutionReturned(0x8298b4u);
}

Lufia2ExecutionResult Lufia2MenuCheckExchangeBalance(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ShopTransactionReady(cpu))
        return ExecutionHandoff(cpu, 0x8298b5u);
    SubtractExchangePrice(memory, cpu, false);
    return ExecutionReturned(0x8298c6u);
}

Lufia2ExecutionResult Lufia2MenuShopExchangeUnitValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82e0e2u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x018au);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x82e0f0u));
    return ExecutionReturned(0x82e0efu);
}

Lufia2ExecutionResult Lufia2MenuShopBuyExchangeUnits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ShopTransactionReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82e0a9u);
    SHOP_TX_CALL(0x82e0a9u, 0x829855u, 2u);
    if (!cpu->carry) {
        OpLdy(cpu, 0x8006u);
        cpu->carry = 1u;
        return ExecutionReturned(0x82e0e1u);
    }
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SHOP_QUANTITY));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_A));
    SHOP_TX_CALL(0x82e0b9u, 0x82e0e2u, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B));
    SHOP_TX_CALL(0x82e0bfu, 0x828000u, 3u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_PRICE));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_PRODUCT + 2u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EXCHANGE_PRICE + 2u));
    SHOP_TX_CALL(0x82e0d1u, 0x82983du, 2u);
    SHOP_TX_CALL(0x82e0d4u, 0x829867u, 3u);
    SHOP_TX_CALL(0x82e0d8u, 0x82de75u, 2u);
    cpu->carry = 0u;
    return ExecutionReturned(0x82e0dcu);
}
