/* Packed item counts in the 96-slot inventory ($81:F057). */

#include "core/cpu_internal.h"
#include "lufia2/item.h"

enum {
    INVENTORY_TABLE = 0x0a8du,
    INVENTORY_ITEM = 0x0a06u,
    INVENTORY_KEY = 0x09f2u,
    INVENTORY_REMAINDER = 0x09f4u,
    INVENTORY_PACKED_COUNT = 0x09f5u,
    INVENTORY_BYTES = 0x00c0u
};

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

/* $81:F0A2: merge the first matching item, otherwise use the first empty
   slot. A full table leaves the request intact. Quantity arithmetic wraps
   in eight bits; an excess over 99 remains packed in the request. */
Lufia2ExecutionResult Lufia2InventoryAdd(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    int found = 0;

    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, 0);
    do {
        LoadA16(cpu, Abs16X(memory, cpu, INVENTORY_TABLE));
        if (!cpu->zero) {
            LoadA16(cpu, (uint16_t)(cpu->accumulator ^
                Read16AbsoluteIndexed(memory, cpu, INVENTORY_ITEM, 0)));
            And16(cpu, 0x01ffu);
            found = cpu->zero;
            if (found)
                break;
        }
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, INVENTORY_BYTES);
    } while (!cpu->zero);

    if (!found) {                                      /* $81:F0BB */
        LoadX16(cpu, 0);
        do {
            LoadA16(cpu, Abs16X(memory, cpu, INVENTORY_TABLE));
            if (cpu->zero) {
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, INVENTORY_ITEM, 0));
                StoreAAbsolute16(memory, cpu, INVENTORY_TABLE, cpu->x);
                StoreWordAbsolute(memory, cpu, INVENTORY_ITEM, 0);
                SetAccumulatorWidth(cpu, 1);
                return ExecutionReturned(0x81f0d8u);
            }
            IncrementX16(cpu);
            IncrementX16(cpu);
            Compare16(cpu, cpu->x, INVENTORY_BYTES);
        } while (!cpu->zero);
        SetAccumulatorWidth(cpu, 1);
        return ExecutionReturned(0x81f0ccu);
    }

    SetAccumulatorWidth(cpu, 1);                       /* $81:F0D9 */
    LoadAAbsolute8(memory, cpu, INVENTORY_TABLE + 1u, cpu->x);
    cpu->carry = A8(cpu) & 1u;
    LoadA8(cpu, (uint8_t)(A8(cpu) >> 1));
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, INVENTORY_REMAINDER, 0));
    Compare8(cpu, A8(cpu), 0x64u);
    if (cpu->carry) {
        Sbc8(cpu, 0x63u);
        StoreAAbsolute8(memory, cpu, INVENTORY_REMAINDER, 0);
        LoadA8(cpu, 0x63u);
    } else {
        StoreZeroAbsolute8(memory, cpu, INVENTORY_REMAINDER, 0);
        StoreZeroAbsolute8(memory, cpu, INVENTORY_ITEM, 0);
    }
    AslA8(cpu);
    StoreAAbsolute8(memory, cpu, INVENTORY_PACKED_COUNT, 0);
    LoadA8(cpu, (uint8_t)(AbsoluteByte(memory, cpu, INVENTORY_TABLE + 1u, cpu->x) & 1u));
    Or8(cpu, AbsoluteByte(memory, cpu, INVENTORY_PACKED_COUNT, 0));
    StoreAAbsolute8(memory, cpu, INVENTORY_TABLE + 1u, cpu->x);
    AslAbsolute8(memory, cpu, INVENTORY_REMAINDER);
    LoadA8(cpu, (uint8_t)(AbsoluteByte(memory, cpu, INVENTORY_ITEM + 1u, 0) & 1u));
    Or8(cpu, AbsoluteByte(memory, cpu, INVENTORY_REMAINDER, 0));
    StoreAAbsolute8(memory, cpu, INVENTORY_ITEM + 1u, 0);
    return ExecutionReturned(0x81f113u);
}
