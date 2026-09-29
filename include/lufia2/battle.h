#ifndef LUFIA2_BATTLE_H
#define LUFIA2_BATTLE_H

/* Battle script VM, NMI and frame upkeep. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $84:8BC7 encounter visual transition; frame-change waits are children. */
Lufia2ExecutionResult Lufia2BattleVisualTransition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $81:8821 Battle setup, main-loop and exit wrapper. */
Lufia2ExecutionResult Lufia2BattleEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $81:E835: character A to two battle glyph tiles; M1X0. */
Lufia2ExecutionResult Lufia2BattleGlyph(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

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

/* $81:E479 frame top edge at Y, row $09FC; M0X0. */
Lufia2ExecutionResult Lufia2BattleFrameTop(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E4AD frame side edges at Y; M0X0. */
Lufia2ExecutionResult Lufia2BattleFrameSides(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E542 frame row from template $01:X; M0X0. */
Lufia2ExecutionResult Lufia2BattleFrameRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E570 frame row ends from template $01:X; M0X0. */
Lufia2ExecutionResult Lufia2BattleFrameEnds(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E5C1 gauge block of tile A at X; M0X0. */
Lufia2ExecutionResult Lufia2BattleGaugeBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E604 gauge column of tile A at X; M0X0. */
Lufia2ExecutionResult Lufia2BattleGaugeColumn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:FB79 A = sprite byte of character A; M1X0. */
Lufia2ExecutionResult Lufia2CharacterSpriteByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:EC41 clear $7F:F000-$FFFF; M1X0. */
Lufia2ExecutionResult Lufia2BattleClearF000(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:FBA2 A = packed size of sprite A - 1; any M, X16. */
Lufia2ExecutionResult Lufia2SpriteSizePacked(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:FBDB $09FC-$09FF = box of character $09FA; M1X0. */
Lufia2ExecutionResult Lufia2CharacterSpriteBox(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:FCE2 X = $96 record of character A; M1X0. */
Lufia2ExecutionResult Lufia2CharacterSpritePointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E7D2 fill $09F2 x $09F3 words at $7E:X; M1X0. */
Lufia2ExecutionResult Lufia2BattleFillRect(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:EB34 palette $24 to $120F; M1X0. */
Lufia2ExecutionResult Lufia2BattlePaletteCopy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:EB62 palette $24 nibbles to $120F/$121F; M1X0. */
Lufia2ExecutionResult Lufia2BattlePaletteSplit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E503 window $09F2 x $09F3 at $7E:X, template $09F4; M1X0. */
Lufia2ExecutionResult Lufia2BattleWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E593 gauge panel at $7E:2D80; M1X0. */
Lufia2ExecutionResult Lufia2BattleGaugePanel(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:BD47 far call of $81:BD4B; M1X0. */
Lufia2ExecutionResult Lufia2BattleSpriteBlockFar(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:BE54 far call of $81:BE58; M1X0. */
Lufia2ExecutionResult Lufia2BattleTileBlockFar(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E3AE filled window $87E3 at X; M1X0. */
Lufia2ExecutionResult Lufia2BattleWindowE3AE(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E3CD filled window $87F9 at X; M1X0. */
Lufia2ExecutionResult Lufia2BattleWindowE3CD(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:BAE8 portraits of slots 3-0, D 0; M1X0. */
Lufia2ExecutionResult Lufia2BattlePortraits(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:BAFB portrait of slot A, DB $97, D 0; M1X0. */
Lufia2ExecutionResult Lufia2BattlePortrait(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:BB75 portrait upload of slot $11, pose $12, DB $97; M1X0. */
Lufia2ExecutionResult Lufia2BattlePortraitUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:B48B $22 = gray $22 blended to $24 by $13; M1X0. */
Lufia2ExecutionResult Lufia2BattleFadeColor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:B444 palette $11 grayed and faded by $13 to CGRAM buffer; M1X0. */
Lufia2ExecutionResult Lufia2BattlePaletteFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E405 frame of tiles $10F1 at $7E:X + $8C0; M1X0. */
Lufia2ExecutionResult Lufia2BattleTileFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:E3EC filled tile frame at $7E:X; M1X0. */
Lufia2ExecutionResult Lufia2BattleTileWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
