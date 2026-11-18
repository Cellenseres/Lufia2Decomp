#ifndef LUFIA2_BATTLE_WINDOW_STEPS_H
#define LUFIA2_BATTLE_WINDOW_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_WINDOW_STEPS 1
#define LUFIA2_WINDOW_BEGIN_OPCODES 15u
#define LUFIA2_WINDOW_ROW_HEAD_OPCODES 5u
#define LUFIA2_WINDOW_ROW_TAIL_OPCODES 7u

#ifdef __cplusplus
extern "C" {
#endif

/* Run a finite window or row phase before its original call or return. */
Lufia2ExecutionResult Lufia2BattleWindowStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif

#endif
