#ifndef LUFIA2_BATTLE_TARGET_RECORD_STEPS_H
#define LUFIA2_BATTLE_TARGET_RECORD_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_BATTLE_TARGET_RECORD_STEPS_API 1
#define LUFIA2_BATTLE_TARGET_RECORD_MAX_OPCODES 21u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2BattleTargetRecordStep(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu, uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
