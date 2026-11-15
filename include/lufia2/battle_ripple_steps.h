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

/* Preserve the caller and enter the word loop in M0X1. */
Lufia2ExecutionResult Lufia2BattleRippleWordPrepareStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleRippleWordRowStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Restore the caller in M1X0, before the original RTS. */
Lufia2ExecutionResult Lufia2BattleRippleWordFinishStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Advance the phase and restore the caller. */
Lufia2ExecutionResult Lufia2BattleRippleFinishStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
