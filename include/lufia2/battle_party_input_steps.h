#ifndef LUFIA2_BATTLE_PARTY_INPUT_STEPS_H
#define LUFIA2_BATTLE_PARTY_INPUT_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_PARTY_INPUT_API 1
#define LUFIA2_BATTLE_PARTY_INPUT_MAX_OPCODES 5u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattlePartyInputStep(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
