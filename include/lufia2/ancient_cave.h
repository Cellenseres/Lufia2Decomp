#ifndef LUFIA2_ANCIENT_CAVE_H
#define LUFIA2_ANCIENT_CAVE_H

/* Ancient Cave floor generation. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * $83:9E31 generate the next floor ($7F:E696); entered by JSL with
 * M1/X0 (A8, X/Y16). Floor 99 sets its fixed scene and returns at $83:9EA1. Other
 * floors pick the floor tables, run the music change $80:93FE (when $56
 * differs from $099D) and the map loader $83:B5D3 through `child` with
 * their exact JSL frames, build the floor ($83:9013) and return at
 * $83:9F3E after incrementing the floor. A $80:BFBC list-search handoff
 * inside the builder returns a boundary; an unwound child returns
 * LUFIA2_EXECUTION_CHILD_UNWOUND.
 */
Lufia2ExecutionResult Lufia2AncientCaveGenerateFloor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

#ifdef __cplusplus
}
#endif

#endif
