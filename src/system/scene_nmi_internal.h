#ifndef LUFIA2_SYSTEM_SCENE_NMI_INTERNAL_H
#define LUFIA2_SYSTEM_SCENE_NMI_INTERNAL_H

#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    SCENE_NMI_TARGET_OFFSET = 1u,
    SCENE_NMI_BANK_OFFSET = 3u,
};

static inline void Lufia2DisableSceneNmi(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(
        memory, cpu,
        OpDp(cpu, (uint8_t)(DP_NMI_STUB + SCENE_NMI_BANK_OFFSET)));
}

static inline void Lufia2InstallSceneNmi(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t target,
    uint8_t bank) {
    Lufia2DisableSceneNmi(memory, cpu);
    OpLdx(cpu, target);
    OpWriteX(
        memory, cpu,
        OpDp(cpu, (uint8_t)(DP_NMI_STUB + SCENE_NMI_TARGET_OFFSET)),
        cpu->x);
    LoadA8(cpu, bank);
    OpSta(
        memory, cpu,
        OpDp(cpu, (uint8_t)(DP_NMI_STUB + SCENE_NMI_BANK_OFFSET)));
}

#endif
