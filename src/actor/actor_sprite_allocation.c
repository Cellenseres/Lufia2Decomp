#include "sprite_allocation_internal.h"
#include "lufia2/actor.h"

Lufia2ExecutionResult Lufia2SpriteReserveAllocation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || !cpu->index_is_8_bit ||
        cpu->decimal || cpu->direct_page || cpu->program_bank != 0x83u ||
        cpu->stack < 0x1f04u || cpu->stack > 0x1ffcu || !A8(cpu))
        return ExecutionHandoff(cpu, 0x83ab7cu);
    SpriteAllocateSlotsBody(memory, cpu);
    return ExecutionReturned(cpu->carry ? 0x83aba7u : 0x83abcbu);
}
