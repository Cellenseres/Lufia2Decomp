#ifndef LUFIA2_BATTLE_STATUS_CACHE_INTERNAL_H
#define LUFIA2_BATTLE_STATUS_CACHE_INTERNAL_H

#include "lufia2/battle.h"

void Lufia2BattleCaptureStatuses(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
void Lufia2BattleClearRecordStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
void Lufia2BattleUpdateStatusMarkers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t marker_bank);

#endif
