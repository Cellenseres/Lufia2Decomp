#include "menu/menu_shop_transaction_internal.h"

enum { DP_SHOP_SPELL_OFFSET = 0u };

static bool SpellMemberReady(const Lufia2CpuState *cpu, uint8_t byte_mode) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit == byte_mode &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2MenuShopSetSpellMemberPose(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SpellMemberReady(cpu, 1u) || !child)
        return ExecutionHandoff(cpu, 0x82e6beu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_SPELL_OFFSET), cpu->y);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER)));
    SHOP_TX_CALL(0x82e6c2u, 0x829302u, 2u);
    OpAndValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x0bu)));
    if (cpu->zero) {
        OpLoadA(cpu, 2u);
    } else {
        OpRepWidths(cpu, 0x20u);
        SHOP_TX_CALL(0x82e6d0u, 0x829971u, 2u);
        OpLda(memory, cpu, OpDp(cpu, DP_SHOP_RECORD));
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SHOP_SPELL_OFFSET)));
        OpSta(memory, cpu, OpDp(cpu, DP_SHOP_RECORD));
        OpSepWidths(cpu, 0x20u);
        OpLdy(cpu, 0u);
        for (;;) {
            OpLda(memory, cpu, AbsoluteIndexedAddress(cpu,
                Read16Direct(memory, cpu, DP_SHOP_RECORD), cpu->y));
            OpAndValue(cpu, 0x7fu);
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
            if (cpu->zero) {
                OpLoadA(cpu, 3u);
                break;
            }
            OpIny(cpu);
            OpDex(cpu);
            if (cpu->zero) {
                OpLoadA(cpu, 0u);
                break;
            }
        }
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER)));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    SHOP_TX_CALL(0x82e6f7u, 0x868cf5u, 3u);
    return ExecutionReturned(0x82e6fbu);
}

Lufia2ExecutionResult Lufia2MenuShopPrepareSpellMembers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!SpellMemberReady(cpu, 0u) || !child)
        return ExecutionHandoff(cpu, 0x82e691u);
    SHOP_TX_CALL(0x82e691u, 0x82fbe5u, 2u);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
    SHOP_TX_CALL(0x82e698u, 0x82e6fcu, 2u);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
    SHOP_TX_CALL(0x82e6a0u, 0x81f414u, 3u);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER), cpu->x);
    do {
        OpLdx(cpu, 36u);
        OpLdy(cpu, 0x96u);
        SHOP_TX_CALL(0x82e6afu, 0x82e6beu, 2u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER), 1);
        OpLda(memory, cpu, OpDp(cpu, DP_SHOP_MEMBER));
        OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82e6bdu);
}
