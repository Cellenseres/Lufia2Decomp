#ifndef LUFIA2_BATTLE_TARGET_MARKER_STEPS_H
#define LUFIA2_BATTLE_TARGET_MARKER_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_TARGET_MARKER_API 1

#ifdef __cplusplus
extern "C" {
#endif

/* M1X0; stops before drawing a selected marker. */
Lufia2ExecutionResult Lufia2BattleVisitTargetMarkerSpan(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint32_t entry, unsigned records);

#ifdef __cplusplus
}
#endif
#endif
