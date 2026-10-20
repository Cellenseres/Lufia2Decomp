#include "menu/menu_shop_transaction_internal.h"

enum { DP_SHOP_MEMBERS_LEFT = 0x26u, DP_SHOP_LIST = 0xfcu,
       PARTY_EQUIPMENT_PREVIEW = 0x48u };

static bool EquipmentMemberReady(const Lufia2CpuState *cpu, uint8_t byte_mode) {
    return cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit == byte_mode &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2MenuShopMarkEquipmentMember(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentMemberReady(cpu, 0u) || !child)
        return ExecutionHandoff(cpu, 0x82e569u);
    OpSepWidths(cpu, 0x20u);
    SHOP_TX_CALL(0x82e56bu, 0x829302u, 2u);
    OpAndValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x15u)));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0xffu);
    RolA8(cpu);
    OpAndValue(cpu, 1u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_PARTY_EQUIPMENT_SELECTED_SLOT));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SHOP_RECORD)));
    OpSta(memory, cpu, OpAbsX(cpu, PARTY_EQUIPMENT_PREVIEW));
    SHOP_TX_CALL(0x82e582u, 0x82f76bu, 2u);
    return ExecutionReturned(0x82e585u);
}

Lufia2ExecutionResult Lufia2MenuShopPrepareEquipmentMembers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentMemberReady(cpu, 1u) || !child)
        return ExecutionHandoff(cpu, 0x82e522u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_MEMBERS_LEFT));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_PARTY_EQUIPMENT_SELECTED_SLOT));
    SHOP_TX_CALL(0x82e52fu, 0x82fbe5u, 2u);
    OpTay(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SHOP_LIST));
    OpAndValue(cpu, 0x01ffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    SHOP_TX_CALL(0x82e53eu, 0x81f1c5u, 3u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x0du));
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        SHOP_TX_CALL(0x82e54au, 0x82f6f2u, 2u);
        SHOP_TX_CALL(0x82e54du, 0x82f703u, 2u);
    }
    OpLdx(cpu, 0u);
    do {
        OpPushX(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
        OpSta(memory, cpu, OpDp(cpu, DP_SHOP_RECORD));
        OpTxa(cpu);
        OpLsrA(cpu);
        OpTay(cpu);
        SHOP_TX_CALL(0x82e55cu, 0x82e569u, 2u);
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_SHOP_MEMBERS_LEFT), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82e568u);
}

Lufia2ExecutionResult Lufia2MenuShopLoadEquippedComparison(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentMemberReady(cpu, 0u) || !child)
        return ExecutionHandoff(cpu, 0x82e5acu);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SHOP_RECORD)));
    OpLda(memory, cpu, OpAbsY(cpu, PARTY_EQUIPMENT_PREVIEW));
    OpAndValue(cpu, 0xff00u);
    if (cpu->zero)
        return ExecutionReturned(0x82e5e0u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_PARTY_EQUIPMENT_SELECTED_SLOT)));
    OpLda(memory, cpu, AbsoluteIndexedAddress(cpu,
        Read16Direct(memory, cpu, DP_SHOP_RECORD), cpu->y));
    OpAndValue(cpu, 0x01ffu);
    if (cpu->zero)
        return ExecutionReturned(0x82e5e0u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    SHOP_TX_CALL(0x82e5c3u, 0x81f1c5u, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SHOP_RECORD)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, PARTY_EQUIPMENT_PREVIEW));
        OpAndValue(cpu, 0xffu);
        OpOraValue(cpu, 0x0200u);
        OpSta(memory, cpu, OpAbsX(cpu, PARTY_EQUIPMENT_PREVIEW));
    }
    SHOP_TX_CALL(0x82e5ddu, 0x82f7fdu, 2u);
    return ExecutionReturned(0x82e5e0u);
}

Lufia2ExecutionResult Lufia2MenuShopRefreshEquipmentMembers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentMemberReady(cpu, 1u) || !child)
        return ExecutionHandoff(cpu, 0x82e586u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_SHOP_MEMBERS_LEFT));
    OpLdx(cpu, 0u);
    do {
        OpPushX(memory, cpu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_MENU_PARTY_CHARACTER_OFFSET));
        OpSta(memory, cpu, OpDp(cpu, DP_SHOP_RECORD));
        SHOP_TX_CALL(0x82e599u, 0x82e5acu, 2u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SHOP_RECORD)));
        SHOP_TX_CALL(0x82e59eu, 0x81f481u, 3u);
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_SHOP_MEMBERS_LEFT), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82e5abu);
}
