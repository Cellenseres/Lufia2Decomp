#ifndef LUFIA2_BATTLE_COMMAND_STEPS_H
#define LUFIA2_BATTLE_COMMAND_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_COMMAND_STEPS 1
#define LUFIA2_BATTLE_COMMAND_INPUT_API 1
#define LUFIA2_BATTLE_COMMAND_INPUT_MAX_OPCODES 7u
#define LUFIA2_BATTLE_COMMAND_LOOP_OPCODES 3u

#ifdef __cplusplus
extern "C" {
#endif

/* $81:CBF1/CC07/CC1A/CC26: command input before calls or returns. */
Lufia2ExecutionResult Lufia2BattleCommandInputStep(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu, uint32_t entry);

Lufia2ExecutionResult Lufia2BattleCommandCallStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2BattleContinueCommandTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
