#ifndef LUFIA2_BATTLE_COMMAND_TILES_H
#define LUFIA2_BATTLE_COMMAND_TILES_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_COMMAND_TILE_API 1
#define LUFIA2_BATTLE_SELECTED_COMMAND_TILE_API 1

#ifdef __cplusplus
extern "C" {
#endif

/* $81:CB99: prepare one command tile, before its renderer. */
Lufia2ExecutionResult Lufia2BattlePrepareCommandTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:CBB8: prepare the highlighted tile before its renderer. */
Lufia2ExecutionResult Lufia2BattlePrepareSelectedCommandTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
