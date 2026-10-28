#ifndef LUFIA2_BATTLE_TIMED_CALLBACKS_H
#define LUFIA2_BATTLE_TIMED_CALLBACKS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleRunTimedCallbacks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleResumeTimedCallbacks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#ifdef __cplusplus
}
#endif
#endif
