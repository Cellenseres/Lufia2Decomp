#ifndef LUFIA2_BATTLE_STATUS_STEPS_H
#define LUFIA2_BATTLE_STATUS_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_BATTLE_STATUS_STEPS_API 2
#define LUFIA2_BATTLE_STATUS_MAX_SYMBOLS 3u
#define LUFIA2_BATTLE_STATUS_MAX_OPCODES 30u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2BattleStatusStep(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu, uint32_t entry,
                                             unsigned max_symbols);
#ifdef __cplusplus
}
#endif
#endif
