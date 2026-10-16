#ifndef LUFIA2_WORLD_SCENE_RESOURCES_INTERNAL_H
#define LUFIA2_WORLD_SCENE_RESOURCES_INTERNAL_H

#include "core/cpu_ops.h"

static inline bool WorldSceneResourceMode(const Lufia2CpuState *cpu, bool byte_mode) {
    return cpu->program_bank == 0x86u && cpu->data_bank == 0x86u &&
           cpu->accumulator_is_8_bit == byte_mode && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static inline bool WorldSceneResourceEntry(const Lufia2CpuState *cpu) {
    return WorldSceneResourceMode(cpu, true);
}

#endif
