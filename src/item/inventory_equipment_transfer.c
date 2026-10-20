#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "system/wram.h"

enum {
    EQUIPMENT_RECORD = 0x2au,
    TRANSFER_ITEM_ID = 0x11u,
    TRANSFER_INVENTORY_INDEX = 0x13u,
    EQUIPMENT_SLOT_COUNT = 6u,
    INVENTORY_CAPACITY = 96u,
    EQUIPMENT_ITEM_ID_MASK = 0x01ffu,
    PACKED_ITEM_ONE = 0x0200u
};

static bool EquipmentTransferReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x82u && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static uint32_t EquipmentSlot(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu) {
    return AbsoluteIndexedAddress(cpu,
        Read16Direct(memory, cpu, EQUIPMENT_RECORD), cpu->y);
}

static Lufia2ExecutionResult TransferUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

#define TRANSFER_CALL(site, target, frame) \
    do { \
        if (!CallChildWithFrame(memory, cpu, child, context, \
                                site, target, frame, 0x82u)) \
            return TransferUnwound(site); \
    } while (0)

Lufia2ExecutionResult Lufia2InventoryRemoveFlaggedEquipment(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentTransferReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82f8afu);
    OpLoadA(cpu, 0u);
    do {
        PushAccumulator16(memory, cpu);
        TRANSFER_CALL(0x82f8b3u, 0x82f893u, 2u);
        PullAccumulator16(memory, cpu);
        if (cpu->carry) {
            PushAccumulator16(memory, cpu);
            cpu->carry = 0u;
            TRANSFER_CALL(0x82f8bbu, 0x82f8dcu, 2u);
            PullAccumulator16(memory, cpu);
        }
        OpIncA(cpu);
        OpCmpValue(cpu, EQUIPMENT_SLOT_COUNT);
    } while (!cpu->zero);
    return ExecutionReturned(0x82f8c5u);
}

Lufia2ExecutionResult Lufia2InventoryUnequipItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentTransferReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82f8dcu);
    if (cpu->carry) {
        TRANSFER_CALL(0x82f8deu, 0x82f893u, 2u);
        if (cpu->carry) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82f924u);
        }
        OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    } else {
        TRANSFER_CALL(0x82f8eau, 0x82f703u, 2u);
        OpLda(memory, cpu, EquipmentSlot(memory, cpu));
        OpAndValue(cpu, EQUIPMENT_ITEM_ID_MASK);
    }
    if (cpu->zero) {
        cpu->carry = 1u;
        return ExecutionReturned(0x82f924u);
    }
    OpSta(memory, cpu, OpDp(cpu, TRANSFER_ITEM_ID));
    TRANSFER_CALL(0x82f8f6u, 0x82fb1fu, 3u);
    if (!cpu->carry) {
        TRANSFER_CALL(0x82f8fcu, 0x82faf4u, 2u);
        if (cpu->carry) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82f924u);
        }
    } else {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
        OpCmpValue(cpu, INVENTORY_CAPACITY);
        if (cpu->zero) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82f924u);
        }
        TRANSFER_CALL(0x82f90bu, 0x82fb94u, 2u);
        if (cpu->carry) {
            cpu->carry = 1u;
            return ExecutionReturned(0x82f924u);
        }
    }
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    cpu->carry = 0u;
    OpAdcValue(cpu, PACKED_ITEM_ONE);
    OpOraValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRANSFER_ITEM_ID)));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, EquipmentSlot(memory, cpu));
    cpu->carry = 0u;
    return ExecutionReturned(0x82f922u);
}

Lufia2ExecutionResult Lufia2InventoryEquipItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentTransferReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82f925u);
    OpWriteX(memory, cpu, OpDp(cpu, TRANSFER_INVENTORY_INDEX), cpu->x);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, EQUIPMENT_ITEM_ID_MASK);
    OpSta(memory, cpu, OpDp(cpu, TRANSFER_ITEM_ID));
    OpLda(memory, cpu, EquipmentSlot(memory, cpu));
    if (!cpu->zero) {
        TRANSFER_CALL(0x82f933u, 0x82fb1fu, 3u);
        if (!cpu->carry) {
            TRANSFER_CALL(0x82f939u, 0x82faf4u, 2u);
            if (cpu->carry) {
                cpu->carry = 1u;
                return ExecutionReturned(0x82f97cu);
            }
            OpLoadA(cpu, 1u);
            TRANSFER_CALL(0x82f96au, 0x82faa0u, 2u);
        } else {
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, TRANSFER_INVENTORY_INDEX)));
            TRANSFER_CALL(0x82f942u, 0x82faf4u, 2u);
            OpCmpValue(cpu, 1u);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
                OpCmpValue(cpu, INVENTORY_CAPACITY);
                if (cpu->zero) {
                    TRANSFER_CALL(0x82f952u, 0x82f9b6u, 2u);
                    cpu->carry = 0u;
                    return ExecutionReturned(0x82f97au);
                }
            }
            TRANSFER_CALL(0x82f957u, 0x82fb94u, 2u);
            if (cpu->carry) {
                cpu->carry = 1u;
                return ExecutionReturned(0x82f97cu);
            }
            OpLda(memory, cpu, EquipmentSlot(memory, cpu));
            OpOraValue(cpu, PACKED_ITEM_ONE);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        }
    }
    OpLda(memory, cpu, OpDp(cpu, TRANSFER_ITEM_ID));
    OpSta(memory, cpu, EquipmentSlot(memory, cpu));
    OpLoadA(cpu, 1u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, TRANSFER_INVENTORY_INDEX)));
    TRANSFER_CALL(0x82f976u, 0x82fad0u, 2u);
    cpu->carry = 0u;
    return ExecutionReturned(0x82f97au);
}

Lufia2ExecutionResult Lufia2InventoryReplaceEquipment(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EquipmentTransferReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82f97du);
    OpLda(memory, cpu, EquipmentSlot(memory, cpu));
    if (!cpu->zero) {
        TRANSFER_CALL(0x82f981u, 0x82fb1fu, 3u);
        if (!cpu->carry) {
            TRANSFER_CALL(0x82f987u, 0x82faf4u, 2u);
            if (cpu->carry) {
                cpu->carry = 1u;
                return ExecutionReturned(0x82f9b5u);
            }
            OpLoadA(cpu, 1u);
            TRANSFER_CALL(0x82f9aau, 0x82faa0u, 2u);
        } else {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_ITEM_QUANTITY));
            OpCmpValue(cpu, 0x60u);
            if (cpu->zero) {
                cpu->carry = 1u;
                return ExecutionReturned(0x82f9b5u);
            }
            TRANSFER_CALL(0x82f997u, 0x82fb94u, 2u);
            if (cpu->carry) {
                cpu->carry = 1u;
                return ExecutionReturned(0x82f9b5u);
            }
            OpLda(memory, cpu, EquipmentSlot(memory, cpu));
            OpOraValue(cpu, 0x0200u);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        }
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    OpSta(memory, cpu, EquipmentSlot(memory, cpu));
    cpu->carry = 0u;
    return ExecutionReturned(0x82f9b3u);
}

Lufia2ExecutionResult Lufia2InventorySwapEquipmentSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EquipmentTransferReady(cpu))
        return ExecutionHandoff(cpu, 0x82f9b6u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpAndValue(cpu, 0x01ffu);
    PushAccumulator16(memory, cpu);
    OpLda(memory, cpu, EquipmentSlot(memory, cpu));
    OpOraValue(cpu, 0x0200u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, EquipmentSlot(memory, cpu));
    return ExecutionReturned(0x82f9c8u);
}

#undef TRANSFER_CALL
