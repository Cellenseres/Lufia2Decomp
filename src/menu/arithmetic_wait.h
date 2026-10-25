#ifndef LUFIA2_MENU_ARITHMETIC_WAIT_H
#define LUFIA2_MENU_ARITHMETIC_WAIT_H

#include "core/cpu_internal.h"

static inline void WaitForArithmetic(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint32_t first_nop, unsigned count) {
    for (unsigned step = 0u; step < count; ++step)
        EmitExecutionCheckpoint(memory, cpu, first_nop + step);
}

#endif
