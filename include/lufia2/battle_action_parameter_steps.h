#ifndef LUFIA2_BATTLE_ACTION_PARAMETER_STEPS_H
#define LUFIA2_BATTLE_ACTION_PARAMETER_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_BATTLE_ACTION_PARAMETER_STEPS_API 1
#define LUFIA2_BATTLE_ACTION_PARAMETER_MAX_OPCODES 14u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2BattleActionParameterStep(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      uint32_t entry);
Lufia2ExecutionResult Lufia2BattleStatusRollContinueStep(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
