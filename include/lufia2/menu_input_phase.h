#ifndef LUFIA2_MENU_INPUT_PHASE_H
#define LUFIA2_MENU_INPUT_PHASE_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2MenuPrepareCursorPoll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
