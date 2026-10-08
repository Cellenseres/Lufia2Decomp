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

/* $85:8A39 battle sprite pass with its setup; JSL, M1X0. */
/* Frame setup requires M8/X16, binary arithmetic, DP0 and S $1F10..$1FFC.
 * DB must map MMIO. All used child spans are checked before the initial clear.
 * Unsupported entries hand off unchanged before any CPU update or write. */
Lufia2ExecutionResult Lufia2BattleFrameSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:8AAF, $85:8AF4, $85:8B22 color tables to the work RAM port.
 * Native: M8/X16, S $1F02..$1FFC; $8AAF also needs a hardware DB. */
Lufia2ExecutionResult Lufia2BattleColorsInit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleColorsParty(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleColorsMonster(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:8B4B, $85:8BC0, $85:8C27, $85:8C98 sprite passes of the battle list;
 * JSL, M1X0. The first three require DP0, S $1F00..$1FFC;
 * variable OAM output must fit in $7E:2000..$FFFF. */
Lufia2ExecutionResult Lufia2BattleSpriteRecordsEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleSpriteSingleEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleSpriteMarkersEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);
/* Party renderers require M8/X16, binary arithmetic, DP0 and S $1F00..$1FFC.
 * Active fixed records have 1..16 columns and rows in their +12 word.
 * Sprite output fits $7E:2000..$FFFF; tile output stays $7E:2800..$3FFF.
 * Inactive records need no dimension check. Unsupported entries hand off
 * before writes, preserving the original for that state. */
Lufia2ExecutionResult Lufia2BattleSpritePartyEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:8D2E party tilemap at $7E:2800; JSL, M1X0. */
Lufia2ExecutionResult Lufia2BattlePartyTilemapEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:972E 15 x 16 tile grid from $3710; JSL, any M, X16. */
Lufia2ExecutionResult Lufia2BattleTileGridEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:9790 sixteen rising tile ids at X; JSR, M0X0. */
Lufia2ExecutionResult Lufia2BattleTileRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:8F4A shifts the 88-bit random register left; M8, any X/DP/DB, RTS.
 * Native selection keeps S=$1F00..$1FFC outside the register bytes. */
Lufia2ExecutionResult Lufia2BattleRandomBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:894A drifts six party offsets; JSL, M8/X16, binary mode,
 * S $1F00..$1FFC. */
Lufia2ExecutionResult Lufia2BattleDriftRecords(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:DE2A sine of the angle in $54 into A and $63; any widths. */
Lufia2ExecutionResult Lufia2BattleSineOfAngle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:DE1E cosine: the angle plus a quarter turn; any widths. */
Lufia2ExecutionResult Lufia2BattleCosineOfAngle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:DD63 velocity words $56/$58 from the angle $54 and speed $5A;
 * M8/X16, DP0, S1F00..1FFC; child return frames stay intact. JSL/RTL. */
Lufia2ExecutionResult Lufia2BattleVelocityOfAngle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:A736 fills 32 ripple bytes at $7E:4400; JSR, M8/X16.
 * The counter at uint16(DP+$33) must lie in low WRAM. Preserves X and
 * the bank restored from the caller's stack, including stack aliases. */
Lufia2ExecutionResult Lufia2BattleRippleRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:AA3D 84-word ripple table at $7E:40DE; JSR, M8/X16. */
Lufia2ExecutionResult Lufia2BattleRippleWords(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:A40B adds a stream word to a slot field; M8/X16, JSR. */
Lufia2ExecutionResult Lufia2BattleEffectAddToField(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:953F repeat counter of an effect slot; M8/X16, JSR. */
Lufia2ExecutionResult Lufia2BattleEffectRepeat(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:9169 saves the resume pointer and yields the effect slot for this frame.
 * PB81, M8/X16, DP0, S1F00..1FFA; any decimal/DB. Discards the opcode call
 * with PLX and returns at $81:8C59 to the dispatcher exit, not the next opcode. */
Lufia2ExecutionResult Lufia2BattleEffectYield(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:A598 slot velocity; M8/X16, DP0, S1F03..1FFC for the velocity child.
 * Leaves DB7E; slot Y may overlap scratch or the caller's frame. JSR/RTS. */
Lufia2ExecutionResult Lufia2BattleEffectVelocity(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:BCCC copies tile rows into the two buffers at $7E:X and $7E:X+$200;
 * M8/X16, binary, DP0, S=$1F00..$1FFC, JSL. DB must expose the multiplier.
 * The main output span must stay in $2000..$FFFF. Zero product copies
 * 256 rows; unsupported inputs retain the original entry before writes. */
Lufia2ExecutionResult Lufia2BattleBlitTileRows(
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

/* $81:EB34 loads 16 font rows for glyph word $24 into $120F, then
 * pads 16 zeros. M1X0, DP zero, native mode; restores Y and DB. */
Lufia2ExecutionResult Lufia2BattlePaletteCopy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:EB62 splits 16 font rows across $120F/$121F at a half-tile
 * boundary. M1X0, DP zero, native mode; restores Y and DB. */
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

/* $85:8F67 random status recovery / $85:9099 timed status expiry.
 * Scan five allies and six enemies. M1X0, DP zero, PB $85, native binary
 * mode. Caller DB is restored; names must be NUL-terminated. Children
 * retain original pushed frames, and messages retain all 45 frame waits.
 */
Lufia2ExecutionResult Lufia2BattleRecoverStatuses(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);
Lufia2ExecutionResult Lufia2BattleExpireStatuses(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:9AAA/$9ABC save/restore 51 direct bytes at $7F:F3DB. M1X0,
 * native mode; DP may vary. Caller DB is restored.
 */
Lufia2ExecutionResult Lufia2BattleSaveMessageState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleRestoreMessageState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:9671 clears 257 text bytes when $1266.low is nonzero. $85:95FE
 * displays text through explicit children and original upload/frame calls.
 * Any M/X, native binary mode, PB $85 and DP zero. Both restore A/X/Y/P/DB
 * on normal return; PLB then sets N/Z from the restored DB.
 */
Lufia2ExecutionResult Lufia2BattleClearMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleDisplayMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:9BDA queue $9800 -> $6C00, length $800. M0X0, DP zero,
 * native binary mode, PB $85 and DB mapping low WRAM.
 */
Lufia2ExecutionResult Lufia2BattleQueueStatusSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:9A7D trim trailing $20 spaces at DB:$1269 and count raw bytes into
 * $22 modulo 256. Retains the first byte even when it is a space. M1X0,
 * native mode, DP zero, DB mapping low WRAM and a NUL-terminated string;
 * $1268 must not be $20 when the string is empty.
 */
Lufia2ExecutionResult Lufia2BattleMeasureMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:E2AF label and proportional status gauge, $11 current/$13 maximum,
 * $16 label/$15 tile base at DB:Y. M1X0, PB $81, DP zero, DB $7E,
 * native binary mode. Retains original DB changes and zero-divisor behavior.
 * $81:E2C8 draws the current value clamped to 999 and restores Y.
 */
Lufia2ExecutionResult Lufia2BattleStatusGauge(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleStatusDigits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:E73B prepares the message tilemap and parses text at DB:$1269.
 * $81:E792 loads resource $023F into $7E:3000. M1X0, PB $81, DP zero,
 * native binary mode and DB mapping low WRAM and the SNES registers.
 * Text is NUL-terminated; control bytes $01..$0F have a payload byte.
 * The measuring child requires $1268 != $20 for empty text. Child calls
 * retain their original frames, including PHX around the glyph child.
 */
Lufia2ExecutionResult Lufia2BattleRenderMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);
Lufia2ExecutionResult Lufia2BattleLoadMessageGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:9ACE subtracts $0020 from the encoded glyph word at DP+$24,
 * preserving full A and P. Any M/X, native binary mode; DP may vary.
 */
Lufia2ExecutionResult Lufia2BattleNormalizeGlyph(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:EA35 renders the encoded glyph at half-tile position $23, then
 * advances it by two modulo 256. M1X0, PB $81, DP zero, native binary
 * mode. Restores DB; child calls preserve their original pushed frames.
 */
Lufia2ExecutionResult Lufia2BattleRenderGlyph(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:AADC installs the message display record and DMA channel 4 bank.
 * $85:AB28 decrements its nonzero word timer $1264; only expiry clears
 * the message. A zero timer keeps it displayed. $85:AB5B queues cleanup.
 * M1X0, DP zero, native binary mode and DB mapping low WRAM.
 */
Lufia2ExecutionResult Lufia2BattleStartMessageEffect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleTickMessageEffect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleQueueMessageCleanup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:8850 advances five party status-icon timers and sprite records;
 * $85:919C forwards it through an explicit child call. M1X0, PB $85,
 * DP zero, native binary mode; restores caller DB. Inactive/downed paths
 * retain the original A value stored into the sprite's glyph byte.
 */
Lufia2ExecutionResult Lufia2BattleAnimateStatusIcons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleUpdateStatusIcons(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:EC81 performs status/sprite upkeep, waits for a frame and handles
 * pause input. M1X0, PB $85, DP zero, native binary mode and DB mapping
 * low WRAM. The memory bus must deliver asynchronous frame/input changes
 * to finish the original polling loops. Children retain their pushed frames.
 */
Lufia2ExecutionResult Lufia2BattleFrameInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* Same entry, stopping at $85:EC94 before polling. Runtime consumers use
 * this continuation when their bus cannot deliver interrupts inside C loops.
 */
Lufia2ExecutionResult Lufia2BattleFrameInputUpkeep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:B705 appends A.low five-byte records at DB:X to OAM at DB:Y.
 * M1X0, PB $81, DP zero, native binary mode. DP $58 holds the sprite count.
 * Four-record groups preserve the original raw attribute merge. Returns
 * before RTS at $B77F or $B7D8, with the last count retained in DP $5A.
 */
Lufia2ExecutionResult Lufia2BattleAppendOamSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:B5C4 builds battle sprite groups and applies position overlays.
 * M1X0, PB $81, DP zero, native binary mode and DB mapping low WRAM.
 * Child JSR frames remain explicit. Returns before RTL $B704; if the bus
 * clears $154E during the prefix loop, the original RTS $B69F is retained.
 */
Lufia2ExecutionResult Lufia2BattleBuildSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $85:CCCE/$CCE3 clear the 132-byte action/saved work bodies; $CCF8 clears
 * 330 bytes of action records. All entry M/X widths and A/X/P are restored.
 * DP zero supplies the zero fill; other DP values retain the original TDC fill.
 */
Lufia2ExecutionResult Lufia2BattleClearActionWork(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleClearSavedActionWork(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleClearActionRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:CD8C/$CD9B copy the complete 136-byte action work record, including
 * its header, in descending order. M1X0, PB $85; DB and DP are preserved.
 */
Lufia2ExecutionResult Lufia2BattleSaveActionWork(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleRestoreActionWork(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:CDFA selects a bank-$7F record from A.low's first set bit and bit7.
 * M1X0, PB $85, native binary mode, DB mapping low WRAM. A zero mask keeps
 * shifting the original $09FB byte until the bus supplies a set bit.
 */
Lufia2ExecutionResult Lufia2BattleActionRecordPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:CDAA/$CDD0 load twelve record bytes through DP long pointers $B5/$B8.
 * All entry M/X widths and A/X/Y/P/DB are restored; native binary mode,
 * PB $85. Child frames remain explicit. $CDD0 clears action work first.
 */
Lufia2ExecutionResult Lufia2BattleLoadTurnRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);
Lufia2ExecutionResult Lufia2BattleLoadActionRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:B1A3/$B1C9/$B1F7 prepare configured, battler or item scripts.
 * M1X0, PB $81, DP zero, native binary mode and DB mapping low WRAM.
 * Original default scripts and base/bank fields are retained. The script VM
 * at $81:FAC9 remains an explicit child with its three-byte JSL frame.
 * The first two entries return before RTL; $B1F7 returns before RTS $B227.
 */
Lufia2ExecutionResult Lufia2BattleRunConfiguredScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);
Lufia2ExecutionResult Lufia2BattleRunBattlerScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);
Lufia2ExecutionResult Lufia2BattleRunItemScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* $81:8E92 rebuilds the actor sprite lists ($4B4A, $4C8A, $4DCA) from the
 * 64 actor records and publishes the per-list totals ($15DB, $15DF, $15E3).
 * M1X0, binary mode, any DP. DB is set to $7E and restored.
 * Rewritten child returns hand off after the original RTS.
 * Returns before RTS $818EE9. */
Lufia2ExecutionResult Lufia2BattleActorSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:8EEA adds actor Y to the sprite list of its kind, M0X0 only. DB must
 * be $7E for the original table addresses. Returns before RTS $818F8D, or
 * $818F90 when the sprite is off screen. */
Lufia2ExecutionResult Lufia2BattleActorSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:B396 fades palette entries (15 of them, from entry $11 * 16 + 1) of
 * the $7F:F1DB palette towards black or white into the $0320 buffer; sign of
 * $13 picks the direction and its low bits the level. M1X0 only (else
 * handed back). Native mode, DP=0, hardware DB, S=$1F02..$1FFC;
 * first palette <= 15. Returns before RTS $81B3F7. */
Lufia2ExecutionResult Lufia2BattlePaletteBrightness(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $81:B3F8 scales the colour in DP $15 by the factor already written to
 * $4202, per 5-bit component. X16, either M, native mode, DP=0, hardware
 * DB, S=$1F00..$1FFC. Returns before RTS $81B443. */
Lufia2ExecutionResult Lufia2BattleScaleColor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $85:AEEB fills the wave scroll table from phase 0; M1X0 only (else handed
 * back). */
Lufia2ExecutionResult Lufia2BattleWaveFill(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:AE68 fills the wave scroll table from the stored phase and advances
 * it; M1X0 only (else handed back). */
Lufia2ExecutionResult Lufia2BattleWaveForward(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:ADE1 fills the wave scroll table from the stored phase and steps it
 * back; M1X0 only (else handed back). */
Lufia2ExecutionResult Lufia2BattleWaveBackward(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:B26D computes circle half widths from DP $C6 into $4600; RTS.
 * Verified caller: DP0, DB7E, S=$1F00..$1FFC, any entry widths/status. */
Lufia2ExecutionResult Lufia2BattleCircleWidths(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $85:B208 rebuilds the circular window when radius $1B4A changes; RTS.
 * Verified caller: M8/X16, DP0, S=$1F08..$1FFC, any DB. The stack range
 * also supports the nested half-width call. */
Lufia2ExecutionResult Lufia2BattleCircleWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Effect video opcodes: M8/X16, DP0, PB81, stack $1F00..$1FFC. */
Lufia2ExecutionResult Lufia2BattleEffectVideoRegister(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectBg3Map(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectBackgroundRelease(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectBackgroundRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectBackgroundCopy(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectWindowBand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Decode effect tiles and queue the original VRAM transfer. */
Lufia2ExecutionResult Lufia2BattleEffectGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);
Lufia2ExecutionResult Lufia2BattleEffectGraphicsAlternate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

/* End a script and discard its opcode return frame. */
Lufia2ExecutionResult Lufia2BattleEffectEnd(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Preserve the original effect delay, jump and repeat operands. */
Lufia2ExecutionResult Lufia2BattleEffectDelay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectJump(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectRepeatStart(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectRepeatJump(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Four independent counted loops within each effect slot. */
Lufia2ExecutionResult Lufia2BattleEffectLoopStart(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectLoopStart2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectLoopStart3(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectLoopStart4(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectLoopNext2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectLoopNext3(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectLoopNext4(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Original effect setup, frame dispatcher and final cleanup. */
Lufia2ExecutionResult Lufia2BattleEffectFindActorSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectFindScriptSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnMovingActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnScriptAt(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnActorAt(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnScriptsForTargets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectBranchIfField(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSelectPortraitStream(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSkipArgument(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CharacterSpriteWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SpriteCoordinatesPacked(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleResolveTargetMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleTargetSpriteCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectTargetCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleRunActionEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleFinishActionEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattlePrepareActionEffect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleRunActorAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleRunActionPresentation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleSkipAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleAttackAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleBeginSpellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleSpellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleBeginItemAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleItemAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleDefendAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleContinueAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleBeginUncostedSpellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleUncostedSpellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleFollowupAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleWaitAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleWaitLongAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleRepeatActionEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2IpRecordPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2IpNamePointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleActionRecordPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleIpAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleCapsuleAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleAlternateAttackAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleCollectiveAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleAction07(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleDispatchAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattlePlayEffect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context);

Lufia2ExecutionResult Lufia2BattleQueueBackgroundTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueBackgroundTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueExtraBackgroundTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueSecondarySpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueWindowGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueWindowTilemapHalf(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueAuxiliaryTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueOverlayTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueWindowTilemaps(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueShortWindowGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueWindowTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueTilesAt3000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueTilesAt4000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueTilesAt4800(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueTilesAt1800(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueTilePatchAt4400(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueShortAuxiliaryTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueAlternateTilesAt4000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueWindowTilemapPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueMinimalAuxiliaryTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueSmallSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueAlternateSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueMidSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueUpperSpriteTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueTilesAt2000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueTilemapAt0E00(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueSmallTilesAt4000(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueTilesAt4400(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleQueueTilesAt4A00(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleShowActionMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleClearMessageRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleCopyRecordName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleLoadIpActionName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleLoadStatusMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleExpandActionMessage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleInsertTurn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleCopyMessageName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleRandomizeTurnPriority(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleMirrorSpriteBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleMirrorSpriteBlockFar(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleClearPartyRecordBytes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleRunRelativeScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectAddBg3Scroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetBg3Scroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectAddBg1Scroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetBg1Scroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleMergeGlyphTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEmitGlyphTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleBuildPartyNameTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleBuildAlternateNameTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectMoveTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectChangeDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSpawnSavedActorScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectTargetAdjustment(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSpawnCurrentTargetScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleInitializeRipple(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSpawnEnemyScripts(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectAimAtPoint(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectAccelerateAtAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectDecelerateAtAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectDecelerateAtOffsetAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSetPackedDrawState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetAlternatePackedDrawState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDrawState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectCopyDrawValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDrawVariant(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetParameterWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectRandomizeParameterWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectAccelerateByByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectDecelerateByByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectAccelerateOne(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectAccelerateTwo(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectAccelerateThree(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectAccelerateFour(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectRestorePaletteRange(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectProjectFromPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectVectorFromParameters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSetDelayOne(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDelayTwo(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDelayThree(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDelayFour(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDelayFive(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDelaySix(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDelayEight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDelayTen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDelayTwenty(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectOffsetBaseAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectRotateAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetVelocityWords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDrawAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDrawValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDrawMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDrawFlag(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetTargetMotionOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectMoveSpecialTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetTargetStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectLoadPaletteRange(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEnableCircleWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSetCircleRadius(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleConfigureLayerColorDma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnStationaryActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnHalfTurnActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnActorWithAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnOffsetActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnUnshiftedMovingActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnHalfTurnMovingActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnRightwardActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnUpwardActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnMovingActorWithAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnActorWithVerticalSpeed(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnMovingActorFromStream(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnZeroPositionScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnCenteredScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectClearDrawDirty(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectClearTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetDrawVariantThree(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectUseThirdDrawParameter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectClearBackgroundUploadRequest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectLoadFirstPalettePreset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectLoadSecondPalettePreset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectLoadThirdPalettePreset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectCopyTileRectangle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnScriptWhenEnabled(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSetPaletteBrightness(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectAdjustPaletteBrightness(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleTogglePartyTargetState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleToggleSpecialTargetState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectTogglePartyTargetState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectToggleSpecialTargetState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectLoadGraphicsResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectClearWindowTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectResetPartyTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSetPortraitPose(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectRestorePortraitPose(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSetPortraitPoseIfStatusClear(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectRefreshTurnDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectRebuildSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSendSoundCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectRandomizeCenteredParameter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSpawnVectorActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSendConditionalSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectSpawnPopupActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleEffectSpawnTargetPopups(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectPrepareFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleEffectCopyActorTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2BattleFadeOutWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleFadeInWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleUpdateSlotPortraitStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2BattleLoadCapsuleGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#ifdef __cplusplus
}
#endif

#endif
