#ifndef LUFIA2_NMI_LISTED_DMA_STEPS_H
#define LUFIA2_NMI_LISTED_DMA_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_NMI_LISTED_DMA_API 1
#define LUFIA2_NMI_LISTED_DMA_MAX_OPCODES 2u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2NmiPrepareListedDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
