/* Original spell-shop setup and purchase-price adjustment. */
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    SPELL_PRICE_TEXT_BANK = 0x5fu,
    SPELL_PRICE_UPLOAD_FLAGS = 0x74u
};

Lufia2ExecutionResult Lufia2AdjustPurchasePrice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x829918u);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_ADJUST_MODE));
    if (!cpu->zero) {
        uint8_t halve = 0;
        OpLdy(cpu, 0u);
        for (;;) {
            OpLda(memory, cpu, OpAbsY(cpu, WRAM_MENU_PARTY_FIRST_ID));
            OpCmpValue(cpu, 1u);
            if (cpu->zero) {
                OpRepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRICE_DISCOUNT_ITEM));
                OpAndValue(cpu, 0x1ffu);
                OpCmpValue(cpu, 0x167u);
                halve = cpu->zero;
                break;
            }
            OpIny(cpu);
            OpTya(cpu);
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
            if (cpu->zero)
                break;
        }
        if (!halve) {
            UnpackStatus(cpu, Pull8(memory, cpu));
            OpPullY(memory, cpu);
            return ExecutionReturned(0x82994du);
        }
    }
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    OpLsrA(cpu);
    OpAdcValue(cpu, 0u);
    OpTax(cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    return ExecutionReturned(0x82994du);
}

static Lufia2ExecutionResult SpellPriceUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SpellPriceChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, cpu->program_bank, (uint16_t)(site + 3u));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, target, site, frame);
}

#define SPELL_PRICE_CALL(site, target, frame) \
    do { \
        if (!SpellPriceChild(memory, cpu, child, context, site, target, frame)) \
            return SpellPriceUnwound(site); \
    } while (0)

Lufia2ExecutionResult Lufia2MenuSpellShopSetup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x82d905u);
    OpLdx(cpu, 0xffd0u);
    SPELL_PRICE_CALL(0x82d908u, 0x829214u, 3u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
    SPELL_PRICE_CALL(0x82d912u, 0x81f414u, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, (WRAM_RECORD_BUFFER + 0x10u))));
    SPELL_PRICE_CALL(0x82d919u, 0x829918u, 2u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_DISPLAY_PRICE), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_PURCHASE_PRICE), cpu->x);
    if (checkpoint)
        checkpoint(context, cpu, 0x82d922u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, SPELL_PRICE_TEXT_BANK));
    OpLdy(cpu, 0xcdcbu);
    SPELL_PRICE_CALL(0x82d92eu, 0x808878u, 3u);
    OpLdy(cpu, 5u);
    SPELL_PRICE_CALL(0x82d935u, 0x82de53u, 2u);
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 7u);
    SPELL_PRICE_CALL(0x82d93du, 0x82895bu, 2u);
    OpLdy(cpu, 0x7eu);
    OpLdx(cpu, 2u);
    OpStz(memory, cpu, OpAbsX(cpu, (WRAM_MENU_SPELL_WINDOW_STATE_2 - 2u)));
    OpStz(memory, cpu, OpAbsX(cpu, (WRAM_MENU_SPELL_WINDOW_STATE_2_ALT - 2u)));
    SPELL_PRICE_CALL(0x82d94cu, 0x82891eu, 2u);
    OpLoadA(cpu, 6u);
    OpLdx(cpu, 8u);
    SPELL_PRICE_CALL(0x82d954u, 0x82895bu, 2u);
    OpLdy(cpu, 0x85u);
    OpLdx(cpu, 3u);
    SPELL_PRICE_CALL(0x82d95du, 0x82891eu, 2u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MEMBER_COUNT));
    OpLoadA(cpu, 0x0bu);
    SPELL_PRICE_CALL(0x82d968u, 0x8293f6u, 2u);
    OpLoadA(cpu, 0x88u);
    OpTestBits(memory, cpu, OpDp(cpu, SPELL_PRICE_UPLOAD_FLAGS), 1u);
    return ExecutionReturned(0x82d96fu);
}
