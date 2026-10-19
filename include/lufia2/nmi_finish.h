#ifndef LUFIA2_NMI_FINISH_H
#define LUFIA2_NMI_FINISH_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2NmiPrepareFrameRestore(Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
