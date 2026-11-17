#ifndef LUFIA2_BATTLE_EFFECT_BACKGROUND_STEPS_H
#define LUFIA2_BATTLE_EFFECT_BACKGROUND_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_EFFECT_BACKGROUND_STEPS 1
#define LUFIA2_EFFECT_BACKGROUND_HEADER_OPCODES 2u
#define LUFIA2_EFFECT_BACKGROUND_DESTINATION_OPCODES 6u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleEffectBackgroundStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2BattleEffectPrepareBackgroundDestination(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
