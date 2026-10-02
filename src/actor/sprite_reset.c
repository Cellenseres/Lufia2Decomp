#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2SpriteResetAllocations(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x30u);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x7cu);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, (WRAM_SPRITE_ALLOCATION_FLAGS & 0xffffu)));
        OpStz(memory, cpu, OpAbsX(cpu, ((WRAM_SPRITE_ALLOCATION_FLAGS + 2u) & 0xffffu)));
        OpLdx(cpu, (uint16_t)(cpu->x - 4u));
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83ab7bu);
}
