#ifndef LUFIA2_MENU_COMMAND_H
#define LUFIA2_MENU_COMMAND_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2MenuConsumeCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
