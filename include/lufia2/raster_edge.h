#ifndef LUFIA2_RASTER_EDGE_H
#define LUFIA2_RASTER_EDGE_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2RasterizeEdge(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Prepared continuation; the original parent entry remains unknown. */
Lufia2ExecutionResult Lufia2RasterizePreparedTriangle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#ifdef __cplusplus
}
#endif

#endif
