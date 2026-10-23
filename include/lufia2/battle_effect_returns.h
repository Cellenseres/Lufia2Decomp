#ifndef LUFIA2_BATTLE_EFFECT_RETURNS_H
#define LUFIA2_BATTLE_EFFECT_RETURNS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleEffectReserved23(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectReserved53(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
