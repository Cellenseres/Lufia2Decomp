#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    PARTY_SAVED_LEVEL = 0x0eu,
    PARTY_SAVED_EXPERIENCE = 0x5fu,
    PARTY_NEXT_LEVEL_EXPERIENCE = 0x62u,
    PARTY_MAXIMUM_LEVEL = 99u,
    DP_SAVE_PARTY_LEVEL_MISMATCHES = 0x22u
};

Lufia2ExecutionResult Lufia2SaveFlagPartyLevelMismatch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned byte = 3u; byte > 0u; --byte) {
        OpLda(memory, cpu, OpAbsY(cpu,
            (uint16_t)(PARTY_SAVED_EXPERIENCE + byte - 1u)));
        OpCmp(memory, cpu, OpAbsY(cpu,
            (uint16_t)(PARTY_NEXT_LEVEL_EXPERIENCE + byte - 1u)));
        if (!cpu->zero)
            break;
    }
    if (cpu->carry) {
        OpLda(memory, cpu, OpAbsY(cpu, PARTY_SAVED_LEVEL));
        OpCmpValue(cpu, PARTY_MAXIMUM_LEVEL);
        if (!cpu->zero)
            OpStepMem(memory, cpu, OpDp(cpu, DP_SAVE_PARTY_LEVEL_MISMATCHES), 1);
    }
    return ExecutionReturned(0x85c953u);
}
