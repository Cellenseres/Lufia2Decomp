#ifndef LUFIA2_BATTLE_RESULT_STEPS_H
#define LUFIA2_BATTLE_RESULT_STEPS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleResultStep(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
