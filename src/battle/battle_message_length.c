#include <stdbool.h>

#include "core/cpu_ops.h"
#include "lufia2/battle.h"

/* $85:9A7D: trim trailing spaces, retaining the first byte, then count bytes. */
Lufia2ExecutionResult Lufia2BattleMeasureMessage(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, 0x22u));
    OpLdx(cpu, 0x1269u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        if (cpu->zero)
            break;
        OpInx(cpu);
    } while (true);
    do {
        OpDex(cpu);
        OpCpx(cpu, 0x1269u);
        if (cpu->zero)
            break;
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpCmpValue(cpu, 0x20u);
        if (!cpu->zero)
            break;
        OpStz(memory, cpu, OpAbsX(cpu, 0u));
    } while (true);
    OpLdx(cpu, 0x1269u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x22u), 1);
    } while (true);
    return ExecutionReturned(0x859aa9u);
}
