#ifndef LUFIA2_PARTY_H
#define LUFIA2_PARTY_H

/* Party members: experience and stats. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $81:F9E9 experience for level $09FE of member $09FA; M1X0. */
Lufia2ExecutionResult Lufia2PartyExperienceForLevel(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F4D5 derived stats of block X; any width. */
Lufia2ExecutionResult Lufia2PartyDerivedStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C261 capsule monster stats when $0A7F is 7; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleLoadStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C515 forms of present capsules; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleSetForms(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C2FD all capsules to level 1; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleReset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C352 present flags = A, capsule 0; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleSetAll(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:CD83 capsule level up; carry clear if gained; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleLevelUp(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:CE23 capsule experience range of the level; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleExperienceRange(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
