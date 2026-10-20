#ifndef LUFIA2_WORLD_SCENE_OAM_INTERNAL_H
#define LUFIA2_WORLD_SCENE_OAM_INTERNAL_H

#include "core/child_call.h"
#include "system/wram.h"
#include "world/world_scene_resources_internal.h"

static inline bool WorldSceneStoreSpriteHighBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    Lufia2ExecutionResult *result) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_SCENE_SPRITE_INDEX));
    OpAndValue(cpu, 3u);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_WORLD_SCENE_SPRITE_INDEX));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_HIGH_BUFFER));
    const uint16_t pointer = (uint16_t)(0xe5dau + cpu->x);
    const uint16_t target = Read16Pair(memory,
        0x860000u | pointer, 0x860000u | (uint16_t)(pointer + 1u));
    if (!CallChildWithFrame(memory, cpu, child, context, site,
                            0x860000u | target, 2u, 0x86u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceEntry(cpu)) {
        *result = ExecutionHandoff(cpu, site + 3u);
        return false;
    }
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_OAM_HIGH_BUFFER));
    return true;
}

#endif
