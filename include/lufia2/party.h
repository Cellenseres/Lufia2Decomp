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

#ifdef __cplusplus
}
#endif

#endif
