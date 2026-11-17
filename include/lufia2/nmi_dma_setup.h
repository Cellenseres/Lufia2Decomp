#ifndef LUFIA2_NMI_DMA_SETUP_H
#define LUFIA2_NMI_DMA_SETUP_H

#include "lufia2/execution.h"

#define LUFIA2_NMI_DMA_SETUP 1
#define LUFIA2_NMI_DMA_SETUP_OPCODES 14u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2NmiPrepareUploadDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
