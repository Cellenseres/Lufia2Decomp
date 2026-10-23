#ifndef LUFIA2_DECOMPRESS_STEPS_H
#define LUFIA2_DECOMPRESS_STEPS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Decode the reference header before its copy setup. */
Lufia2ExecutionResult Lufia2DecompressReferenceStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* Decode long offsets with original scratch and stack writes. */
Lufia2ExecutionResult Lufia2DecompressLongReferenceStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* Prepare the original ascending MVN copy. */
Lufia2ExecutionResult Lufia2DecompressSetupCopyStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* Consume a high-bit symbol's original control flag. */
Lufia2ExecutionResult Lufia2DecompressFlagStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

/* Advance a flagged symbol, preserving control-group rollover. */
Lufia2ExecutionResult Lufia2DecompressAdvanceStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

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
