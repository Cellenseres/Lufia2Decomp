#ifndef LUFIA2_BATTLE_TARGET_NAME_STEPS_H
#define LUFIA2_BATTLE_TARGET_NAME_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_TARGET_SELECTION_PHASES 1
#define LUFIA2_BATTLE_TARGET_INPUT_PHASES 1

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleClearTargetNameSpan(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t entry, unsigned bytes);

Lufia2ExecutionResult Lufia2BattleTargetCursorStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

Lufia2ExecutionResult Lufia2BattleTargetInputStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);

#ifdef __cplusplus
}
#endif
#endif
