#ifndef LUFIA2_WORLD_FADE_INTERNAL_H
#define LUFIA2_WORLD_FADE_INTERNAL_H

#include "core/cpu_ops.h"
#include "system/wram.h"

static inline void WorldSceneStartFade(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t control) {
    OpOraValue(cpu, control);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
}

#endif
