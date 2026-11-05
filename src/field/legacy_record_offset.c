#include "core/cpu_ops.h"
#include "lufia2/legacy_fragments.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2FieldLegacyComputeRecordOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit > 1u || cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80d835u);

    OpAslA(cpu);
    OpAdc(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD4E6));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_B));
    OpAslA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_B));
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpTax(cpu);
    return ExecutionReturned(0x80d848u);
}
