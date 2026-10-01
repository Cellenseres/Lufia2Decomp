#ifndef LUFIA2_ANCIENT_CAVE_H
#define LUFIA2_ANCIENT_CAVE_H

/* Ancient Cave floor generation. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $83:9E31: next Ancient Cave floor; M1X0. */
Lufia2ExecutionResult Lufia2AncientCaveGenerateFloor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $8E:B847: generated room and object sections; X16, returns A8. */
Lufia2ExecutionResult Lufia2CaveBuildMapHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $83:9B44: packed room id to map-header coordinates; A8/X16. */
Lufia2ExecutionResult Lufia2CaveRoomHeaderCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
