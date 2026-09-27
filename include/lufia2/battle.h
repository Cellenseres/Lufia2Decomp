#ifndef LUFIA2_BATTLE_H
#define LUFIA2_BATTLE_H

/* Battle script VM, NMI and frame upkeep. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $85:B452 battle script VM. */
Lufia2ExecutionResult Lufia2BattleScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:8DC5 battle NMI uploads, timers, HDMA. */
Lufia2ExecutionResult Lufia2BattleNmiUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:8A2F battle sprites and party tilemap. */
Lufia2ExecutionResult Lufia2BattleSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:ECF0 per-frame battle upkeep. */
Lufia2ExecutionResult Lufia2BattleFrameUpkeep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:ECDB first free $1A8F VRAM queue slot in Y. */
Lufia2ExecutionResult Lufia2BattleVramQueueSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:C129 IP skill table $7E:DF00 of member $1BE8; M1X0. */
Lufia2ExecutionResult Lufia2BattleIpSkills(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:DFA2 eight battle list rows from entry X - 2; M1X0. */
Lufia2ExecutionResult Lufia2BattleListRows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:C35F damage popups at $7E:4F0B for mode A; any width. */
Lufia2ExecutionResult Lufia2BattlePopups(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
