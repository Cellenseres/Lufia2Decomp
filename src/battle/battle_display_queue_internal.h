#ifndef LUFIA2_BATTLE_DISPLAY_QUEUE_INTERNAL_H
#define LUFIA2_BATTLE_DISPLAY_QUEUE_INTERNAL_H

#include "core/cpu_ops.h"
#include "system/wram.h"

static inline void WriteBattleDisplayBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t source, uint16_t destination, uint16_t size) {
    OpLoadA(cpu, source);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE + 2u));
    OpLoadA(cpu, destination);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE + 4u));
    OpLoadA(cpu, size);
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_VRAM_QUEUE));
}

#endif
