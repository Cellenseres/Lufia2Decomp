#ifndef LUFIA2_BATTLE_TARGET_NAME_SETUP_H
#define LUFIA2_BATTLE_TARGET_NAME_SETUP_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_TARGET_NAME_SETUP 1

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleTargetNameSetupStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
