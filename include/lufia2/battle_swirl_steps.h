#ifndef LUFIA2_BATTLE_SWIRL_STEPS_H
#define LUFIA2_BATTLE_SWIRL_STEPS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleSwirlHdmaPrepareStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleSwirlHdmaStoreStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleSwirlHdmaAdvanceStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
