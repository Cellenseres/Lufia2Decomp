#ifndef LUFIA2_WORLD_SCENE_INTERNAL_H
#define LUFIA2_WORLD_SCENE_INTERNAL_H

#include "core/cpu_ops.h"
#include "system/wram.h"

static inline uint32_t WorldSceneObjectField(const Lufia2CpuState *cpu, uint8_t field) {
    return DirectIndexedAddress(cpu, field, cpu->x) | OP_DP_WRAP;
}

static inline void WorldSceneCopyPositionToObject(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_X_LOW)));
    OpWrite16(memory, WorldSceneObjectField(cpu, 0x02u), cpu->y);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, WRAM_WORLD_MAP_POSITION_Y)));
    OpWrite16(memory, WorldSceneObjectField(cpu, 0x05u), cpu->y);
}

#endif
