#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "system/wram.h"

enum { EQUIPMENT_RECORD = 0x2au };

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
