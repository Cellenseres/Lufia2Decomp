#ifndef LUFIA2_FRAME_WAIT_H
#define LUFIA2_FRAME_WAIT_H

#include "lufia2/decomp.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2FrameWaitSaveRegistersStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2FrameWaitReturnStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif

#endif
