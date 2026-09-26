#ifndef LUFIA2_SYSTEM_H
#define LUFIA2_SYSTEM_H

/* Random numbers, sound queue and screen fade. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $80:8E9D decompress resource $54 to $7E/$7F:[$60]; any width. */
Lufia2ExecutionResult Lufia2DecompressResource(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:8378 $4E /= $51, A = remainder; any width. */
Lufia2ExecutionResult Lufia2Divide16(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:8299: A = (A.low * next random byte) >> 8. */
void Lufia2RandomScale(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:82C7: A.low = next random byte, same table and index. */
void Lufia2RandomByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $84:8766: defer APU command A to $17AC. */
void Lufia2QueueDeferredSound(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:86C1 screen fade into the $0583 brightness. */
Lufia2ExecutionResult Lufia2ScreenFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
