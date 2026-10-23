#ifndef LUFIA2_BATTLE_INPUT_STEPS_H
#define LUFIA2_BATTLE_INPUT_STEPS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Process input after the ordinary frame wait. */
Lufia2ExecutionResult Lufia2BattleInputAfterWait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Resume pause handling after its frame wait. */
Lufia2ExecutionResult Lufia2BattlePauseAfterWait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
