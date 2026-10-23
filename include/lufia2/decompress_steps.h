#ifndef LUFIA2_DECOMPRESS_STEPS_H
#define LUFIA2_DECOMPRESS_STEPS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Original control group, preserving LoROM stream rollover. */
Lufia2ExecutionResult Lufia2DecompressControlStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* One symbol; high-bit symbols retain their flag boundary. */
Lufia2ExecutionResult Lufia2DecompressSymbolStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* One original MVN byte and its completed-copy suffix. */
Lufia2ExecutionResult Lufia2DecompressCopyByteStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
