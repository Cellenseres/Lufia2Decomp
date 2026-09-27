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

/* $81:B264 active mask of enemies (A bit 7) or party; M1X0. */
Lufia2ExecutionResult Lufia2BattleActiveMask(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:B2B5 X = battler record of target mask A; M1X0. */
Lufia2ExecutionResult Lufia2BattleTargetRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:B2DB X = $1499 + 7 * target of mask A; M1X0. */
Lufia2ExecutionResult Lufia2BattleTargetSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:B505 A = $CA + (A - $CA) * $29 / 256; M1X0. */
Lufia2ExecutionResult Lufia2BattleBlend(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:B54A colour $15 to gray, level in $17; exits M0. */
Lufia2ExecutionResult Lufia2ColorToGray(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:B5A3 hide every sprite of the OAM buffer; M1X0. */
Lufia2ExecutionResult Lufia2BattleHideOam(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:B974 palette A from $24:Y to $7F:F1DB; M1X0. */
Lufia2ExecutionResult Lufia2BattleLoadPalette(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:B9AF battle palettes to the CGRAM buffer; M1X0. */
Lufia2ExecutionResult Lufia2BattleCommitPalettes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:C2C0 16 bytes $B401 to $123C; M1X0. */
Lufia2ExecutionResult Lufia2BattleCopyC2C0(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:C2D0 clear $7E:2000-$27FF; M1X0. */
Lufia2ExecutionResult Lufia2BattleClear2000(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:C2E3 fill $7E:2800-$2FFF with $2100; M1X0. */
Lufia2ExecutionResult Lufia2BattleFill2800(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:C2FB clear $7E:3000-$37FF; M1X0. */
Lufia2ExecutionResult Lufia2BattleClear3000(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:C30E clear $7E:3800-$3FFF; M1X0. */
Lufia2ExecutionResult Lufia2BattleClear3800(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:C5CF X = record of target mask A, A nonzero; M1X0. */
Lufia2ExecutionResult Lufia2BattleTargetPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:BE58 $02 x $03 2x2 tiles at $7E:$08; M1X0. */
Lufia2ExecutionResult Lufia2BattleTileBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:BD4B $02 x $03 sprites at $7E:$08, A = count; M1X0. */
Lufia2ExecutionResult Lufia2BattleSpriteBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
