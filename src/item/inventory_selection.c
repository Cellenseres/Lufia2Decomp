#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/item.h"

enum {
    DP_ITEM_PACKED_QUANTITY = 0x54u,
    DP_ITEM_IDENTIFIER = 0x56u,
    ITEM_IDENTIFIER_MASK = 0x01ffu,
    ITEM_QUANTITY_MASK = 0xfe00u,
    ITEM_QUANTITY_LIMIT = 99u,
    INVENTORY_BYTE_COUNT = 192u,
    ROM_ITEM_POSSESSION_MASKS = 0x8ed8c3u
};

static bool InventorySelectionSupported(const Lufia2CpuState *cpu) {
    return !cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult InventorySelectionUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2InventoryStorePackedSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!InventorySelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82f9e8u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_ITEM_PACKED_QUANTITY), cpu->y);
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpOra(memory, cpu, OpDp(cpu, DP_ITEM_PACKED_QUANTITY));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    return ExecutionReturned(0x82f9f1u);
}

Lufia2ExecutionResult Lufia2InventoryIncreaseSlotQuantity(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!InventorySelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82faa0u);
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpAndValue(cpu, ITEM_QUANTITY_MASK);
    OpSta(memory, cpu, OpDp(cpu, DP_ITEM_PACKED_QUANTITY));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, ITEM_IDENTIFIER_MASK);
    OpSta(memory, cpu, OpDp(cpu, DP_ITEM_IDENTIFIER));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, ITEM_QUANTITY_MASK);
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, DP_ITEM_PACKED_QUANTITY));
    OpSta(memory, cpu, OpDp(cpu, DP_ITEM_PACKED_QUANTITY));
    OpLsrA(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpCmpValue(cpu, ITEM_QUANTITY_LIMIT);
    if (cpu->carry) {
        OpLoadA(cpu, ITEM_QUANTITY_LIMIT);
        ExchangeAccumulatorBytes(cpu);
        OpAslA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_ITEM_PACKED_QUANTITY));
    }
    OpLda(memory, cpu, OpDp(cpu, DP_ITEM_IDENTIFIER));
    OpOra(memory, cpu, OpDp(cpu, DP_ITEM_PACKED_QUANTITY));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    return ExecutionReturned(0x82facfu);
}

Lufia2ExecutionResult Lufia2InventorySlotAtQuantityLimit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!InventorySelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82faf4u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS + 1u));
    OpLsrA(cpu);
    OpAndValue(cpu, 0x7fu);
    OpCmpValue(cpu, ITEM_QUANTITY_LIMIT);
    cpu->carry = cpu->zero;
    return ExecutionReturned(cpu->zero ? 0x82fb01u : 0x82fb03u);
}

Lufia2ExecutionResult Lufia2InventoryFindEmptySlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!InventorySelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82fb94u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        if (cpu->zero) {
            cpu->carry = false;
            return ExecutionReturned(0x82fba6u);
        }
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, INVENTORY_BYTE_COUNT);
    } while (!cpu->zero);
    cpu->carry = true;
    return ExecutionReturned(0x82fba4u);
}

Lufia2ExecutionResult Lufia2InventoryRegisterPossession(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !InventorySelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82fa2eu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82fa31u, 0x82fb51u, 2u, cpu->program_bank))
        return InventorySelectionUnwound(0x82fa31u);
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        OpLoadA(cpu, 2u);
        cpu->carry = true;
        return ExecutionReturned(0x82fa56u);
    }
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        OpLoadA(cpu, 1u);
        cpu->carry = true;
        return ExecutionReturned(0x82fa51u);
    }
    OpLda(memory, cpu, OpLongX(cpu, ROM_ITEM_POSSESSION_MASKS));
    OpOra(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E091E));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E091E));
    OpLoadA(cpu, 0u);
    cpu->carry = false;
    return ExecutionReturned(0x82fa4cu);
}

Lufia2ExecutionResult Lufia2InventoryAddMenuItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !InventorySelectionSupported(cpu))
        return ExecutionHandoff(cpu, 0x82f9f2u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    OpAndValue(cpu, ITEM_IDENTIFIER_MASK);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82f9fbu, 0x82fa2eu, 2u, cpu->program_bank))
        return InventorySelectionUnwound(0x82f9fbu);
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x82fa06u, 0x82fb1fu, 3u, cpu->program_bank))
            return InventorySelectionUnwound(0x82fa06u);
        if (!cpu->carry) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82fa0cu, 0x82faf4u, 2u, cpu->program_bank))
                return InventorySelectionUnwound(0x82fa0cu);
            if (cpu->carry)
                return ExecutionReturned(0x82fa2du);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82fa14u, 0x82faa0u, 2u, cpu->program_bank))
                return InventorySelectionUnwound(0x82fa14u);
        } else {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82fa19u, 0x82fb94u, 2u, cpu->program_bank))
                return InventorySelectionUnwound(0x82fa19u);
            if (cpu->carry)
                return ExecutionReturned(0x82fa2du);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
            OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL)));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x82fa24u, 0x82f9e8u, 2u, cpu->program_bank))
                return InventorySelectionUnwound(0x82fa24u);
        }
    }
    OpWriteM(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL), 0u);
    cpu->carry = false;
    return ExecutionReturned(0x82fa2bu);
}
