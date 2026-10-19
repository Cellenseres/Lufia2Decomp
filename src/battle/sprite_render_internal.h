#ifndef LUFIA2_BATTLE_SPRITE_RENDER_INTERNAL_H
#define LUFIA2_BATTLE_SPRITE_RENDER_INTERNAL_H

#include <stdbool.h>
#include "lufia2/execution.h"

enum {
    BATTLE_GRID_TILE = 0x00u,
    BATTLE_GRID_WIDTH = 0x02u,
    BATTLE_GRID_HEIGHT = 0x03u,
    BATTLE_GRID_ATTRIBUTES = 0x04u,
    BATTLE_GRID_Y = 0x05u,
    BATTLE_GRID_X = 0x06u,
    BATTLE_GRID_X_SIGN = 0x07u,
    BATTLE_GRID_WRITE_OFFSET = 0x08u,
    BATTLE_GRID_ROW_TILE = 0x11u,
    BATTLE_GRID_COLUMNS_LEFT = 0x13u,
    BATTLE_GRID_ROWS_LEFT = 0x14u,
    BATTLE_GRID_EMITTED = 0x15u,
    BATTLE_GRID_ROW_Y = 0x16u,
    BATTLE_GRID_COLUMN_X = 0x17u,
    BATTLE_GRID_COLUMN_X_HIGH = 0x18u,
    BATTLE_GRID_TILE_ROW_BASE = 0x19u,
    BATTLE_GRID_ROW_X = 0x1cu,
};

Lufia2ExecutionResult Lufia2BattleDrawSpriteGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool mirrored);
Lufia2ExecutionResult Lufia2BattleDrawTileGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleReadCharacterSpriteWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleReadSpriteCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#endif
