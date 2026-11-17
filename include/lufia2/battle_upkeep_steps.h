#ifndef LUFIA2_BATTLE_UPKEEP_STEPS_H
#define LUFIA2_BATTLE_UPKEEP_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_UPKEEP_STEPS 1

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleContinueFrameUpkeep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry,
    Lufia2PushedChildCall child, void *context);

#ifdef __cplusplus
}
#endif
#endif
