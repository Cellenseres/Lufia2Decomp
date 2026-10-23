#ifndef LUFIA2_BATTLE_BACKGROUND_STEPS_H
#define LUFIA2_BATTLE_BACKGROUND_STEPS_H
#include "lufia2/execution.h"
#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2BattleDecodeBackgroundWordStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
#ifdef __cplusplus
}
#endif
#endif
