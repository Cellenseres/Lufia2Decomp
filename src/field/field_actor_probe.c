#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum { ACTOR_PROBE_X = 0x8fu, ACTOR_PROBE_Y = 0x91u };

static bool ActorMatchesHorizontalProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned probe = 0; probe < 2u; ++probe) {
        OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
        OpCmpValue(cpu, 2u);
        if (cpu->carry) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
            OpIncA(cpu);
            OpCmp(memory, cpu, OpDp(cpu, ACTOR_PROBE_X));
            if (cpu->zero)
                return true;
        }
    }
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpCmp(memory, cpu, OpDp(cpu, ACTOR_PROBE_X));
    return cpu->zero != 0;
}

Lufia2ExecutionResult Lufia2FieldFindActorAtProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x83bac2u);
    OpLdx(cpu, 8u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        OpBitValue(cpu, 4u);
        if (!cpu->zero) {
            OpInx(cpu);
            OpCpx(cpu, WRAM_ACTOR_STATE_COUNT);
            continue;
        }
        if (ActorMatchesHorizontalProbe(memory, cpu)) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
            OpCmp(memory, cpu, OpDp(cpu, ACTOR_PROBE_Y));
            cpu->carry = 1;
            if (cpu->zero) {
                OpWriteX(memory, cpu, OpDp(cpu, DP_SCRATCH_C), cpu->x);
                return ExecutionReturned(0x83bb07u);
            }
        }
        OpInx(cpu);
        OpCpx(cpu, WRAM_ACTOR_STATE_COUNT);
    } while (!cpu->zero);
    cpu->carry = 0;
    return ExecutionReturned(0x83bb07u);
}
