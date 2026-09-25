#ifndef LUFIA2_WORLD_MAP_H
#define LUFIA2_WORLD_MAP_H

/* World map NMI, streaming and region lookup. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $86:CEF6 world map NMI uploads, palette cycles, Mode 7. */
Lufia2ExecutionResult Lufia2WorldMapNmiUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:99BF world map edge streaming after a camera move. */
Lufia2ExecutionResult Lufia2WorldMapStreamEdges(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:9EDD world map region search; carry clear on a hit. */
Lufia2ExecutionResult Lufia2WorldMapRegionSearch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
