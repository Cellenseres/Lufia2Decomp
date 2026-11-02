#ifndef LUFIA2_BATTLE_EFFECT_TARGET_SCRIPTS_H
#define LUFIA2_BATTLE_EFFECT_TARGET_SCRIPTS_H

#include "lufia2/battle.h"

Lufia2ExecutionResult BattleSpawnTargetScripts(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry);

#endif
