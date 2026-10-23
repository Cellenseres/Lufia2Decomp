#ifndef LUFIA2_BATTLE_RIPPLE_STEPS_H
#define LUFIA2_BATTLE_RIPPLE_STEPS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Select the ripple phase and preserve the caller. */
Lufia2ExecutionResult Lufia2BattleRipplePrepareStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Write one original scroll byte and advance its counter. */
Lufia2ExecutionResult Lufia2BattleRippleRowStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Advance the phase and restore the caller. */
Lufia2ExecutionResult Lufia2BattleRippleFinishStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
