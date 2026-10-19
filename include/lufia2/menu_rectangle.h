#ifndef LUFIA2_MENU_RECTANGLE_H
#define LUFIA2_MENU_RECTANGLE_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2MenuClearLayerRectangleStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
