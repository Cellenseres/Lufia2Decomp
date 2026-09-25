#ifndef LUFIA2_TEXT_H
#define LUFIA2_TEXT_H

/* Text and cutscene script engine. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $80:9CB8 text engine step; codes and words stay LLE. */
Lufia2ExecutionResult Lufia2TextEngineStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
