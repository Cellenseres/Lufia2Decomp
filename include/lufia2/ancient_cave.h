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

#ifdef __cplusplus
}
#endif

#endif
