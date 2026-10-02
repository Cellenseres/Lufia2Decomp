#ifndef LUFIA2_ANCIENT_CAVE_H
#define LUFIA2_ANCIENT_CAVE_H

/* Ancient Cave generation and inventory transitions. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $84:8AF4: append eligible blue equipment to WMDATA; PB84/M0X0. */
Lufia2ExecutionResult Lufia2AncientCaveCarryBlueItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $84:890B: restore pre-Cave state and carry items out; PB84/M1X0.
 * Checkpoints precede PHP at 890B and the restored-state test at 8A38. */
Lufia2ExecutionResult Lufia2AncientCaveExit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $84:8888: clear the Cave party and give its initial items; PB84/M1X0. */
Lufia2ExecutionResult Lufia2AncientCaveResetParty(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $84:8B9C: restart after defeat; PB84/M1X0. Checkpoints before the
 * first LDA at8B9C and after the party reset returns at8BA5. */
Lufia2ExecutionResult Lufia2AncientCaveDefeat(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

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
