#include "core/cpu_ops.h"
#include "lufia2/world_map.h"

enum { WORLD_CELL_BITS = 4u, WORLD_CELL_CENTER = 8u };

Lufia2ExecutionResult Lufia2WorldMapCellCenter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    (void)memory;
    if (cpu->program_bank != 0x86u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8691feu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    for (unsigned bit = 0u; bit < WORLD_CELL_BITS; ++bit)
        AslA16(cpu);
    OpAdcValue(cpu, WORLD_CELL_CENTER);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86920du);
}
