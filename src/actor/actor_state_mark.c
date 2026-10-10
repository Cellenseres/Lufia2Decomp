#include "lufia2/actor.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

static void MarkStatePairs(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t base, uint16_t last_pair) {
    OpLdx(cpu, last_pair);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, base));
        OpOraValue(cpu, 0x40u);
        OpSta(memory, cpu, OpAbsX(cpu, base));
        OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(base + 1u)));
        OpOraValue(cpu, 0x40u);
        OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(base + 1u)));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
}

Lufia2ExecutionResult Lufia2ActorMarkAllState40(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x30u);
    MarkStatePairs(memory, cpu, WRAM_ACTOR_STATE, WRAM_ACTOR_STATE_COUNT - 2u);
    MarkStatePairs(memory, cpu, WRAM_OBJECT_STATE, WRAM_OBJECT_STATE_COUNT - 2u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83c7f7u);
}

