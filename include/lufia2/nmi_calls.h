#ifndef LUFIA2_NMI_CALLS_H
#define LUFIA2_NMI_CALLS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2NmiBeginChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t pc);
Lufia2ExecutionResult Lufia2NmiReturnInterrupt(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2NmiInvokeCallback(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t pc);
Lufia2ExecutionResult Lufia2NmiReturnCallback(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t pc);

Lufia2ExecutionResult Lufia2NmiReturnEmptyInterrupt(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
