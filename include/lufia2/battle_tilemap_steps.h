#ifndef LUFIA2_BATTLE_TILEMAP_STEPS_H
#define LUFIA2_BATTLE_TILEMAP_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_TILEMAP_SPANS 1

enum { LUFIA2_BATTLE_TILEMAP_SPAN_MAX_CELLS = 8u };

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleClearTilemapCells(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, unsigned max_cells);

#ifdef __cplusplus
}
#endif
#endif
