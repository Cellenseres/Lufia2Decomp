/* Packed item counts in the 96-slot inventory ($81:F057). */

#include "core/cpu_internal.h"
#include "lufia2/item.h"
#include "system/dp_scratch.h"

static Lufia2ExecutionResult ItemPossessionBitCore(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    And16(cpu, 0x01ffu);
    StoreADirect16(memory, cpu, DP_SCRATCH_A);
    LoadX16(cpu, 0);
    for (;;) {
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x97fda0u, cpu->x)));
        Compare16(cpu, cpu->accumulator, 0xffffu);
        if (cpu->zero) break;
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, DP_SCRATCH_A));
        if (cpu->zero) {
            LoadA16(cpu, cpu->x);
            for (unsigned i = 0; i < 5u; ++i) LsrA16(cpu);
            AslA16(cpu);
            TransferAToY(cpu);
            LoadA16(cpu, cpu->x);
            And16(cpu, 0x001fu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ed8c3u, cpu->x)));
            And16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x091eu, cpu->y));
            if (!cpu->zero) {
                LoadA16(cpu, 0);
                cpu->carry = 0;
            } else {
                LoadA16(cpu, 1);
                cpu->carry = 1;
            }
            return ExecutionReturned(cpu->carry ? 0x82fb8eu : 0x82fb89u);
        }
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0080u);
        if (cpu->zero) break;
    }
    LoadA16(cpu, 2);
    cpu->carry = 1;
    return ExecutionReturned(0x82fb93u);
}

Lufia2ExecutionResult Lufia2ItemCheckPossessionBit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82fb51u);
    return ItemPossessionBitCore(memory, cpu);
}

static void ItemPossessionBit(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xfb27u);
    (void)ItemPossessionBitCore(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $82:FB1F: count of item A; carry when missing. */
Lufia2ExecutionResult Lufia2ItemPossessionCount(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    And16(cpu, 0x01ffu);
    StoreADirect16(memory, cpu, DP_SCRATCH_A);
    PushY(memory, cpu);
    ItemPossessionBit(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    SetNz16(cpu, cpu->y);
    if (!cpu->carry) {
        LoadA16(cpu, 1);
        return ExecutionReturned(0x82fb2eu);
    }
    LoadX16(cpu, 0);
    do {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a8du, cpu->x));
        And16(cpu, 0x01ffu);
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, DP_SCRATCH_A));
        if (cpu->zero) {
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a8eu, cpu->x));
            LsrA16(cpu);
            And16(cpu, 0x007fu);
            cpu->carry = 0;
            return ExecutionReturned(0x82fb50u);
        }
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x00c0u);
    } while (!cpu->zero);
    LoadA16(cpu, 0);
    cpu->carry = 1;
    return ExecutionReturned(0x82fb47u);
}

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

/* $81:F0A2: add to a matching or empty slot. */
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
