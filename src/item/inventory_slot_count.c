#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2InventoryCountOccupiedSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82fb04u);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_INVENTORY_PACKED_ITEMS));
        if (!cpu->zero)
            OpIny(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 192u);
    } while (!cpu->zero);
    OpTya(cpu);
    OpCmpValue(cpu, 96u);
    if (cpu->zero) {
        cpu->carry = 0u;
        return ExecutionReturned(0x82fb1eu);
    }
    cpu->carry = 1u;
    return ExecutionReturned(0x82fb1cu);
}
