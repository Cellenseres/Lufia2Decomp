#ifndef LUFIA2_NMI_TILEMAP_STEPS_H
#define LUFIA2_NMI_TILEMAP_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_NMI_TILEMAP_LOAD_STEPS 1

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2NmiPrepareTilemapBlock(Lufia2CpuState *cpu,
    uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
