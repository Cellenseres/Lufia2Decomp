#ifndef LUFIA2_BATTLE_COMMAND_STEPS_H
#define LUFIA2_BATTLE_COMMAND_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_COMMAND_STEPS 1
#define LUFIA2_BATTLE_COMMAND_LOOP_OPCODES 3u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleCommandCallStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2BattleContinueCommandTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
