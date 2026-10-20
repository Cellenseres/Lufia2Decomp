#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "system/wram.h"

enum { DP_EQUIPMENT_KIND_MASK = 0x54u };

Lufia2ExecutionResult Lufia2ItemFindEquipmentKind(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82f6f2u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_RECORD_BUFFER + 0x14u));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_EQUIPMENT_KIND_MASK));
    OpLoadA(cpu, 0xffffu);
    do {
        OpIncA(cpu);
        OpLsrMem(memory, cpu, OpDp(cpu, DP_EQUIPMENT_KIND_MASK));
    } while (!cpu->carry);
    return ExecutionReturned(0x82f702u);
}
