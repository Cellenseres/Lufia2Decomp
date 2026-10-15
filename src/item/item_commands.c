#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "system/wram.h"

enum {
    REQUESTED_ITEM = 0x09cfu,
    REQUESTED_QUANTITY = WRAM_MENU_ITEM_QUANTITY,
    DP_QUANTITY_BITS = 0x54u,
    DP_INVENTORY_ITEM = 0x56u,
    ROM_POSSESSION_MASKS = 0x8ed8c3u,
    INVENTORY_SLOT_BYTES = 0xc0u
};

static bool ItemRequestReady(const Lufia2CpuState *cpu, uint16_t stack_min) {
    return cpu->program_bank == 0x82u && !cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
           cpu->stack >= stack_min && cpu->stack <= 0x1ffcu;
}

static bool QuantitySlotReady(const Lufia2CpuState *cpu) {
    return ItemRequestReady(cpu, 0x1f08u) && cpu->x < INVENTORY_SLOT_BYTES &&
           !(cpu->x & 1u);
}

static void ReadQuantityDelta(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpAndValue(cpu, 0xfe00u);
    OpSta(memory, cpu, OpDp(cpu, DP_QUANTITY_BITS));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, 0x01ffu);
    OpSta(memory, cpu, OpDp(cpu, DP_INVENTORY_ITEM));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, 0xfe00u);
}

Lufia2ExecutionResult Lufia2ItemSubtractSlotQuantity(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    if (!QuantitySlotReady(cpu))
        return ExecutionHandoff(cpu, 0x82fad0u);
    ReadQuantityDelta(memory, cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_QUANTITY_BITS)));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, DP_QUANTITY_BITS));
        OpLda(memory, cpu, OpDp(cpu, DP_INVENTORY_ITEM));
        OpOra(memory, cpu, OpDp(cpu, DP_QUANTITY_BITS));
    }
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    return ExecutionReturned(0x82faf3u);
}

Lufia2ExecutionResult Lufia2ItemClearRequestedPossession(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu) {
    if (!ItemRequestReady(cpu, 0x1f08u))
        return ExecutionHandoff(cpu, 0x82fa77u);
    OpLda(memory, cpu, OpAbs(cpu, REQUESTED_ITEM));
    SimulateJsrFrame(memory, cpu, 0xfa7cu);
    const Lufia2ExecutionResult result = Lufia2ItemCheckPossessionBit(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        OpLoadA(cpu, 2u);
        cpu->carry = 1u;
        return ExecutionReturned(0x82fa9fu);
    }
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLoadA(cpu, 1u);
        cpu->carry = 1u;
        return ExecutionReturned(0x82fa9au);
    }
    OpLda(memory, cpu, OpLongX(cpu, ROM_POSSESSION_MASKS));
    OpLoadA(cpu,
            (uint16_t)(OpA(cpu) ^ OpReadM(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E091E))));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E091E));
    OpLoadA(cpu, 0u);
    cpu->carry = 0u;
    return ExecutionReturned(0x82fa95u);
}

Lufia2ExecutionResult Lufia2ItemRemoveRequested(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu) {
    if (!ItemRequestReady(cpu, 0x1f10u))
        return ExecutionHandoff(cpu, 0x82fa57u);
    SimulateJsrFrame(memory, cpu, 0xfa59u);
    Lufia2ExecutionResult result = Lufia2ItemClearRequestedPossession(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, REQUESTED_ITEM));
        SimulateJslFrame(memory, cpu, 0x82u, 0xfa65u);
        result = Lufia2ItemPossessionCount(memory, cpu);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        SimulateRtlFrame(memory, cpu);
        if (cpu->carry) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82fa76u);
        }
        OpLda(memory, cpu, OpAbs(cpu, REQUESTED_QUANTITY));
        SimulateJsrFrame(memory, cpu, 0xfa6du);
        result = Lufia2ItemSubtractSlotQuantity(memory, cpu);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        SimulateRtsFrame(memory, cpu);
    }
    OpStz(memory, cpu, OpAbs(cpu, REQUESTED_ITEM));
    cpu->carry = 0u;
    return ExecutionReturned(0x82fa74u);
}
