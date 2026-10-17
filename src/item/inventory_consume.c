#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "system/wram.h"

enum {
    INVENTORY_SELECTED_SLOT = 0x0a08u,
    INVENTORY_QUANTITY = WRAM_BATTLE_ACTION_WORK_BYTE,
    INVENTORY_REQUEST_ID = WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS,
    INVENTORY_LAST_OFFSET = (WRAM_INVENTORY_PACKED_ITEMS_COUNT - 1u) * 2u,
    INVENTORY_ITEM_MASK = 0x01ffu,
    INVENTORY_QUANTITY_MASK = 0xfe00u
};

static uint8_t InventoryConsumeContext(const Lufia2CpuState *cpu,
                                       uint8_t either_accumulator_width) {
    return cpu->program_bank == 0x81u && !cpu->direct_page && !cpu->decimal &&
           !cpu->index_is_8_bit &&
           (either_accumulator_width || cpu->accumulator_is_8_bit) &&
           cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2InventoryRemoveQuantity(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    if (!InventoryConsumeContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x81f114u);
    OpSta(memory, cpu, OpAbs(cpu, INVENTORY_QUANTITY));
    OpLda(memory, cpu, OpAbs(cpu, INVENTORY_SELECTED_SLOT));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS + 1u));
    OpLsrA(cpu);
    if (cpu->zero) {
        TransferDirectToA(cpu);
        return ExecutionReturned(0x81f15au);
    }
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, INVENTORY_QUANTITY)));
    if (!cpu->zero && cpu->carry) {
        OpAslA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, INVENTORY_QUANTITY));
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS + 1u));
        OpAndValue(cpu, 1u);
        OpOra(memory, cpu, OpAbs(cpu, INVENTORY_QUANTITY));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS + 1u));
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS + 1u));
        OpLsrA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, INVENTORY_QUANTITY));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS + 1u));
    }
    OpLda(memory, cpu, OpAbs(cpu, INVENTORY_QUANTITY));
    return ExecutionReturned(0x81f158u);
}

Lufia2ExecutionResult Lufia2InventoryConsumeRequestedItem(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!child || !InventoryConsumeContext(cpu, 1u))
        return ExecutionHandoff(cpu, 0x81f15bu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpAndValue(cpu, INVENTORY_ITEM_MASK);
    OpSta(memory, cpu, OpAbs(cpu, INVENTORY_REQUEST_ID));
    OpLdy(cpu, WRAM_INVENTORY_PACKED_ITEMS_COUNT - 1u);
    OpLdx(cpu, INVENTORY_LAST_OFFSET);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        OpBitValue(cpu, INVENTORY_QUANTITY_MASK);
        if (!cpu->zero) {
            OpAndValue(cpu, INVENTORY_ITEM_MASK);
            OpCmp(memory, cpu, OpAbs(cpu, INVENTORY_REQUEST_ID));
            if (cpu->zero)
                break;
        }
        OpDex(cpu);
        OpDex(cpu);
        OpDey(cpu);
        if (cpu->negative) {
            OpSepWidths(cpu, 0x20u);
            TransferDirectToA(cpu);
            return ExecutionReturned(0x81f184u);
        }
    }
    OpSepWidths(cpu, 0x20u);
    OpTya(cpu);
    OpSta(memory, cpu, OpAbs(cpu, INVENTORY_SELECTED_SLOT));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID + 1u));
    OpLsrA(cpu);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x81f18fu, 0x81f114u, 3u,
                            0x81u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x81f18fu);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    return ExecutionReturned(0x81f193u);
}
