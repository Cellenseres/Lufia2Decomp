#ifndef LUFIA2_CAVE_CAVE_INTERNAL_H
#define LUFIA2_CAVE_CAVE_INTERNAL_H

/* Ancient Cave floor generator internals ($83:9013-$83:9E30). */

#include "core/cpu_ops.h"
#include "cave/wram.h"
#include "lufia2/ancient_cave.h"

/* cave_random.c: random draws scaled through the PPU multiplier. */
void Lufia2CaveRandomBelow(                                   /* $83:9E1B */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveRandomIndex(                                   /* $83:9E11 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveRandomMean(                                    /* $83:9DE4 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);

/* cave_grid.c: the 16x16 room grid at $7F:EA00. */
void Lufia2CaveClearVisited(                                  /* $83:9B18 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveCountCells(                                    /* $83:9B27 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveStairOffset(                                   /* $83:9B3C */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveCellPosition(                                  /* $83:9B48 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveLinkRooms(                                     /* $83:9B62 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveMergeRoom(                                     /* $83:9BDD */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveFillRoom(                                      /* $83:9CA0 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveCellIndex(                                     /* $83:9CCB */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveBlockOffsetX(                                  /* $83:9CD6 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveTileOffsetY(                                   /* $83:9CF3 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CavePickCell(                                      /* $83:9D11 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveFindCell(                                      /* $83:9D23 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveQueueLink(                                     /* $83:9D39 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveSetUpperTile(                                  /* $83:9D46 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveTileOffsetY2(                                  /* $83:9D68 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveDrawBlock(                                     /* $83:9D88 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);

/* cave_objects.c: placements checked against the stair and object lists. */
void Lufia2CaveTileAt(                                        /* $83:99C8 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveNearStartOrPlaced(                             /* $83:99D3 */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveAddObject(                                     /* $83:9A1C */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveNearChest(                                     /* $83:9A6B */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveChestAt(                                       /* $83:9A9F */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);
void Lufia2CaveAddChest(                                      /* $83:9ABC */
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site);

/* ancient_cave.c: $83:9013; 0 = propagate (handoff or unwind). */
Lufia2ExecutionResult Lufia2CaveBuildFloor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#endif
