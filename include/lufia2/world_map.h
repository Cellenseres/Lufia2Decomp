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

/* $86:E8CE world map sprite chain; entry M1X0. */
Lufia2ExecutionResult Lufia2WorldSpriteChain(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:A894 world map ground plane matrix tables; entry M1X0. */
Lufia2ExecutionResult Lufia2WorldMapPlane(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E295 appends object X to the visible list at Y when it overlaps the
 * screen. M0X0 only (else handed back). */
Lufia2ExecutionResult Lufia2WorldMapTestObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E287 runs the test for the objects counted in DP $22, stepping X by
 * $1D. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapTestObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E640 clears the flag words of the 32 object slots. M1X0 only. */
Lufia2ExecutionResult Lufia2WorldMapClearSlotFlags(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E650 hides all hardware sprites and clears the OAM high table.
 * Any entry widths, restored on exit. */
Lufia2ExecutionResult Lufia2WorldMapClearSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
