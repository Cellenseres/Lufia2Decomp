#ifndef LUFIA2_BATTLE_DISPLAY_STEPS_H
#define LUFIA2_BATTLE_DISPLAY_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_DISPLAY_STEPS 1
#define LUFIA2_BATTLE_DISPLAY_STEP_OPCODES 6u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleFinishDisplayBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
