#ifndef LUFIA2_BATTLE_H
#define LUFIA2_BATTLE_H

/* Battle script VM, NMI and frame upkeep. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Battle transition. */
Lufia2ExecutionResult Lufia2BattleVisualTransition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* Battle lifecycle. */
Lufia2ExecutionResult Lufia2BattleEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* Battle setup. */
Lufia2ExecutionResult Lufia2BattleSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* Battle background setup. */
Lufia2ExecutionResult Lufia2BattleBackgroundPrepare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* Battle exit. */
Lufia2ExecutionResult Lufia2BattleExit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* Battle display setup. */
Lufia2ExecutionResult Lufia2BattleDisplaySetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* Battle main loop. */
Lufia2ExecutionResult Lufia2BattleMainLoop(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $81:890A: turn-list execution; M1X0, DB $97, DP zero, explicit children. */
Lufia2ExecutionResult Lufia2BattleExecuteTurns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:A79A: action preparation; M1X0, DB $97, DP zero, explicit children. */
Lufia2ExecutionResult Lufia2BattlePrepareAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:C600: status/HP tick; M1X0, DP zero, explicit effect children. */
Lufia2ExecutionResult Lufia2BattleStatusTick(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:C739: command collection; M1X0, DB $97, DP zero, explicit children.
 * Local RTS $CB76; nonlocal $8855 and malformed-selection BRK $C8BC hand off. */
Lufia2ExecutionResult Lufia2BattleCollectCommands(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:CC2E: member action selection; M1X0, DB $97, DP zero, explicit children.
 * Returns locally or hands off at original BRK/self-loop boundaries. */
Lufia2ExecutionResult Lufia2BattleChoosePartyAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:D12F/$81:D19A: start/resume an action submenu; M1X0, DB $97, DP zero. */
Lufia2ExecutionResult Lufia2BattleActionSubmenuStart(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);
Lufia2ExecutionResult Lufia2BattleActionSubmenuResume(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:D4E0: interactive target selection; M1X0, DB $97, DP zero. */
Lufia2ExecutionResult Lufia2BattleChooseTargets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:B8B1: target coordinates; M1X0, DB $97, DP zero. */
Lufia2ExecutionResult Lufia2BattleTargetCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:D920/D92C/D938/D948: five-byte cursor sprites; M1X0, DP zero. */
Lufia2ExecutionResult Lufia2BattleEnemyCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattlePartyCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEnemyMarkedCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattlePartyMarkedCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:D9D0/D975: command frame upkeep/confirmation; M1X0, DP zero. */
Lufia2ExecutionResult Lufia2BattleCommandFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);
Lufia2ExecutionResult Lufia2BattleConfirmCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:CB77: command selection; M1X0, DB $97, DP zero, explicit children. */
Lufia2ExecutionResult Lufia2BattleChooseCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:9236: carry is set when no party record is eligible. */
Lufia2ExecutionResult Lufia2BattlePartyStatusGate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:C240: prepare the next Battle frame. */
Lufia2ExecutionResult Lufia2BattlePrepareNextFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $85:96A2/$85:96B0: save and restore the Battle work span. */
Lufia2ExecutionResult Lufia2BattleSaveWorkArea(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleRestoreWorkArea(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:89E5: clear six Battle sprite offset pairs. */
Lufia2ExecutionResult Lufia2BattleClearSpriteOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:AB78: set the Battle transfer descriptor and request bit. */
Lufia2ExecutionResult Lufia2BattleStageTransfer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:9275: selected party turns; M1X0, DB $97, DP zero, count 1..4. */
Lufia2ExecutionResult Lufia2BattleQueuePartyTurns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
/* $81:C254: six enemy turns; M1X0, DB $97, DP zero. */
Lufia2ExecutionResult Lufia2BattleQueueEnemyTurns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
/* $81:C294: capsule turn; M1X0, DB $97, DP zero. */
Lufia2ExecutionResult Lufia2BattleQueueCapsuleTurn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:93B7: status snapshot/outcome; M1X0, DP zero, caller DB restored. */
Lufia2ExecutionResult Lufia2BattleCheckOutcome(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

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

/* Turn display: M1X0/native binary/DP0, DB maps low WRAM.
 * $81:E645 takes party slot X=0/2/4/6; each entry keeps child frames explicit. */
Lufia2ExecutionResult Lufia2BattleRefreshTurnDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2BattlePartyStatusRows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2BattlePartyStatusRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2BattleQueueTurnDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $81:D9E1 rewards; M1X0/native binary/DB97/DP0. Party IDs must be in 0A7B. */
Lufia2ExecutionResult Lufia2BattleResults(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:DD7F result-window setup; M1X0/native binary/DB81/DP0. */
Lufia2ExecutionResult Lufia2BattleResultWindowPrepare(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:DDE7 result line at Y in bank85; same contract, original DB restored. */
Lufia2ExecutionResult Lufia2BattleResultWindowLine(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:DE55 scroll result rows; M1X0/native binary/DB7E/DP0, DB stays7E. */
Lufia2ExecutionResult Lufia2BattleResultWindowScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:DE9E result confirmation; M1X0/native binary/DB81/DP0. */
Lufia2ExecutionResult Lufia2BattleResultWindowWait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:E4D1 party name for X=0/2/4/6; M1X0/native binary/DP0, DB restored. */
Lufia2ExecutionResult Lufia2BattlePartyName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:9CD7 action-window upload; M0X0/native binary/DP0, DB maps low WRAM. */
Lufia2ExecutionResult Lufia2BattleQueueActionWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:9CEE command-list upload; same entry contract, including DB7E. */
Lufia2ExecutionResult Lufia2BattleQueueListWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:BEBC command tiles for A; M1X0/native, DB preserved. */
Lufia2ExecutionResult Lufia2BattleCommandTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:BEED action menu tiles; same entry contract. */
Lufia2ExecutionResult Lufia2BattleActionMenuTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:DEF4 action window; M1X0/native binary/DP0, DB preserved. */
Lufia2ExecutionResult Lufia2BattleActionWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:DF0A windows for present party members; same entry contract. */
Lufia2ExecutionResult Lufia2BattlePartyWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:E16F clear action windows and queue uploads; M1X0/native binary/DB97/DP0. */
Lufia2ExecutionResult Lufia2BattleClearActionWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:BF3F inventory command list; M1X0, native binary, DB97, DP0. */
Lufia2ExecutionResult Lufia2BattleItemCommands(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:C031 member spell command list; same entry contract. */
Lufia2ExecutionResult Lufia2BattleSpellCommands(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

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

/* Battle display defaults. */
Lufia2ExecutionResult Lufia2BattleLoadDisplayDefaults(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu);

/* Clear the background tilemap. */
Lufia2ExecutionResult Lufia2BattleClearBackgroundTilemap(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu);

/* Reset the party tilemap. */
Lufia2ExecutionResult Lufia2BattleResetPartyTilemap(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu);

/* Clear the window tilemap. */
Lufia2ExecutionResult Lufia2BattleClearWindowTilemap(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu);

/* Clear tilemap $3800. */
Lufia2ExecutionResult Lufia2BattleClearTilemap3800(const Lufia2Memory *memory,
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

/* Status helpers: native binary mode, DP zero, A8 and X/Y16.
 * $85:9150 copies the NUL-terminated name at DB:X into $1269 and trims
 * trailing $10 glyphs. DB must map low WRAM; $1268 must not be $10 for an
 * empty name. $85:9173 appends phrase A.low (0..6) at length $1266.
 */
Lufia2ExecutionResult Lufia2BattleStatusName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleStatusPhrase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:91A1 sync / $85:91E0 clear the five party status markers; M1X0, DP 0. */
Lufia2ExecutionResult Lufia2BattleSyncStatusMarkers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleClearStatusMarkers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:D9C9 X = effect-work pointer for nonzero A.low mask; M1X0, DP 0,
 * native binary mode and DB mapping low WRAM. Zero retains a ROM self-loop.
 */
Lufia2ExecutionResult Lufia2BattleEffectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:DCA3 $63..$66 = $54 * $56 through the original hardware multiplier.
 * $85:DCEA A = A times a random 16-bit fraction. Both require M0X0, DP 0
 * and native binary mode; DCA3 also requires DB mapping the SNES registers.
 */
Lufia2ExecutionResult Lufia2BattleMultiply(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleRandomFraction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
