/* Packed item counts in the 96-slot inventory ($81:F057). */

#include "core/cpu_internal.h"
#include "lufia2/item.h"

enum { INVENTORY_TABLE = 0x0a8du, INVENTORY_ITEM = 0x0a06u, INVENTORY_KEY = 0x09f2u };

static uint16_t Abs16X(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t offset) {
    return Read16AbsoluteIndexed(memory, cpu, offset, cpu->x);
}

/* $81:F057: A = count of item $0A06 in the inventory, 0 if none. */
Lufia2ExecutionResult Lufia2InventoryCount(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, INVENTORY_ITEM, 0) & 0x01ffu));
    StoreWordAbsolute(memory, cpu, INVENTORY_KEY, cpu->accumulator);
    LoadX16(cpu, 0x00beu);
    do {
        LoadA16(cpu, Abs16X(memory, cpu, INVENTORY_TABLE));
        cpu->zero = (cpu->accumulator & 0xfe00u) == 0;
        if (!cpu->zero) {
            And16(cpu, 0x01ffu);
            Compare16(cpu, cpu->accumulator, Read16AbsoluteIndexed(memory, cpu, INVENTORY_KEY, 0));
            if (cpu->zero) {
                LoadA16(cpu, Abs16X(memory, cpu, INVENTORY_TABLE));
                cpu->carry = cpu->accumulator & 1u;
                LoadA16(cpu, (uint16_t)(cpu->accumulator >> 1));
                SetAccumulatorWidth(cpu, 1);
                ExchangeAccumulatorBytes(cpu);
                return ExecutionReturned(0x81f084u);
            }
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 2u));
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    return ExecutionReturned(0x81f07cu);
}
