#ifndef LUFIA2_MENU_TILE_CLEAR_INTERNAL_H
#define LUFIA2_MENU_TILE_CLEAR_INTERNAL_H

#include <stdbool.h>
#include "lufia2/menu.h"

void ClearMenuTileRectangle(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            bool both_layers);

#endif
