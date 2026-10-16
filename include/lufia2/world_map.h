#ifndef LUFIA2_WORLD_MAP_H
#define LUFIA2_WORLD_MAP_H

/* World map NMI, streaming and region lookup. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $86:CEF6 world map NMI uploads, palette cycles, Mode 7. */
Lufia2ExecutionResult Lufia2WorldMapNmiUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:99BF world map edge streaming after a camera move. */
Lufia2ExecutionResult Lufia2WorldMapStreamEdges(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:9EDD world map region search; carry clear on a hit. */
Lufia2ExecutionResult Lufia2WorldMapRegionSearch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E8CE world map sprite chain; entry M1X0. */
Lufia2ExecutionResult Lufia2WorldSpriteChain(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:A894 ground plane tables; M1X0, PB/DB86, DP0, S1F00..1FFC. */
Lufia2ExecutionResult Lufia2WorldMapPlane(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E295 appends object X to the visible list at Y when it overlaps the
 * screen. M0X0, DP0; DB must map low work RAM. X <= $1FF1 and
 * Y <= $1FD2 keep the record and both list fields in RAM. Caller-frame
 * aliases retain the original writes and rewritten RTS destination. */
Lufia2ExecutionResult Lufia2WorldMapTestObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E287 runs the test for the objects counted in DP $22, stepping X by
 * $1D. Caller contract: M0X0, DP0, work-RAM DB, X=$1469, Y=$124F,
 * S=$1F00..$1FFC, and 1..21 objects. Other entries hand off before writes.
 * The count check reads RAM without changing CPU flags. */
Lufia2ExecutionResult Lufia2WorldMapTestObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E640 clears the flag words of the 32 object slots. M1X0 only. */
Lufia2ExecutionResult Lufia2WorldMapClearSlotFlags(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E650 hides all sprites, clears the OAM high table; any widths. */
Lufia2ExecutionResult Lufia2WorldMapClearSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E0B9 starts animation A on object X: loads its first frame record
 * and tile base. M1X0 only (else handed back). */
Lufia2ExecutionResult Lufia2WorldMapStartAnimation(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E11F steps the 22 object animations. M1X0, DP0, S $1F00..$1FFC. */
Lufia2ExecutionResult Lufia2WorldMapStepAnimations(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E555 draws the two sprites of the object at $02. M0X0. */
Lufia2ExecutionResult Lufia2WorldMapDrawSpritePair(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E5BB stores two OAM high bits, advances counter $1467. M0X0. */
Lufia2ExecutionResult Lufia2WorldMapStoreHighBits(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E479 draws one 16-pixel sprite of the object at $02. M0X0. */
Lufia2ExecutionResult Lufia2WorldMapDrawSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E4E7 draws one 8-pixel sprite of the object at $02. M0X0. */
Lufia2ExecutionResult Lufia2WorldMapDrawSmallSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E686 stably sorts up to 21 visible objects by descending unsigned
 * key. M16/X16, native mode, DP=0, first-bank WRAM mirror DB,
 * S=$1F02..$1FFC. Other entry states hand off unchanged; RTS. */
Lufia2ExecutionResult Lufia2WorldMapSortVisible(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Object-pass contracts: binary arithmetic, DP0, X16 and a WRAM data
 * bank. Records are the 22 fixed map/player records at $1469 + 29*n.
 * Unsupported states hand off at the unchanged entry before any write.
 * Assign, kind, draw, project and update require S >= $1F00, $1F04,
 * $1F08, $1F00 and $1F10 respectively, all <= $1FFC.
 * Assign searches 1..16 entries in pools $12A5..$12D5; existing pattern
 * users must be fixed records. Draw requires <=21 visible records, an
 * empty shared-pattern list and <=80 existing sprites. Project starts
 * at X=$1469, Y=$124F, with 1..21 objects in DP $22.
 * Update needs DB=$86/$06 for its ROM pool tables. Negative-Y records
 * are inactive and may contain unknown kinds. */
/* $86:E430 finds or adds the sprite pattern slot of the object at $02; carry
 * reports a new use. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapAssignSlot(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E3D2 draws the object at X by its kind; an unknown kind hands the
 * original entry back before writes. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapDrawObjectByKind(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E3AB draws the sorted visible objects and the player object. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapDrawObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:A5A9 (JSR) $00/$02 / $04, remainder in A; any widths. */
Lufia2ExecutionResult Lufia2WorldMapDivide32(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E2D2 tests object visibility for the tilted map view. M0X0 only. */
Lufia2ExecutionResult Lufia2WorldMapProjectObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:E1B9 runs the per-frame object pass of the world map. M1X0 only. */
Lufia2ExecutionResult Lufia2WorldMapUpdateObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Plane row caller contract: M0X0, DP0, DB86, Y=$0382 or $01C1,
 * and 1..112 rows in DP $26. The body has no stack operations or children;
 * the parent enters with S=$1EFC at its lowest supported stack. Counter
 * guards read RAM before writes, without changing CPU flags. */
/* $86:A9B0 plane scanline rows, quadrant 0; JSR, M16/X16 only. */
Lufia2ExecutionResult Lufia2WorldPlaneRows0(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:AA5B plane scanline rows, quadrant 1; JSR, M16/X16 only. */
Lufia2ExecutionResult Lufia2WorldPlaneRows1(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:AB0E plane scanline rows, quadrant 2; JSR, M16/X16 only. */
Lufia2ExecutionResult Lufia2WorldPlaneRows2(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:ABC1 plane scanline rows, quadrant 3; JSR, M16/X16 only. */
Lufia2ExecutionResult Lufia2WorldPlaneRows3(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:A583 writes the 24-bit product of DP $4E and DP $50; M8/X16, RTS.
 * Any DP/DB; preserves original multiplier accesses, decimal ADC and
 * scratch aliases. X retains the first hardware product. */
Lufia2ExecutionResult Lufia2WorldProduct16By8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:A417 signed step offsets for the distance $1249 in direction $1248;
 * M8/X16, RTS, DP0, low work-RAM DB, S=$1E00..$1FFC. */
Lufia2ExecutionResult Lufia2WorldStepOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:995B applies the step to the scroll position; M8/X16, RTS, DP0,
 * low work-RAM DB, S=$1E02..$1FFC so its child frame satisfies $A417. */
Lufia2ExecutionResult Lufia2WorldScrollAdvance(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapPackFirstSpriteHighBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapPackSecondSpriteHighBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapPackThirdSpriteHighBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapPackFourthSpriteHighBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapInitializeSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapInitializeObjectRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapSelectObjectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapIndexObjectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapCellCenter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapBindResourcePointers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapBlankDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapResetSceneState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapConfigureMode7(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapUpdatePlane(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapInstallGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapResetScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapUploadTilePlane(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapBuildCellOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapBuildBlockPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapStreamRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapStreamColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapUploadInitialTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapProjectDistance(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapBuildColorHdma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapResolveAngleScale(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapMultiply16(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapLoadResourceColors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapBuildSkylineHdma(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapProjectTiltDistance(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapProjectTiltReciprocal(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapCopyFlaggedBlocks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapQueuePaletteTransfer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapStepPaletteColors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadResourceBlocks(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadMotionRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapDisableDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapSumMotionTerms(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapRememberTilePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapCheckMovingArrival(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapCheckStoppedArrival(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapStartFadeOut(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapStartFadeIn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapResolveResourcePointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapCenterTargetTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapReadSecondObjectTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapConfigurePrimaryView(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapConfigureSecondaryView(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapEnableDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapUpdateObjectHeading(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapPositionAnimatedObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapAnimatePairedObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapResetDisplayState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapClearDisplayPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapReadTileClass(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRandomScaledWord(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context);

Lufia2ExecutionResult Lufia2WorldMapSetPlayerSong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapActivateVehicle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRestorePlayerObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapMovePlayerNorth(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapMovePlayerWest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapMovePlayerSouth(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapMovePlayerEast(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapFindRegionPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapConsumeRegionPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapApplyEntryPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunScene03(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunScene06(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapHandleSceneInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapPolarQuadrant0(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapPolarQuadrant1(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapPolarQuadrant2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapPolarQuadrant3(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapRunVehicleScene03(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunVehicleScene07(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapWaitFade(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapSelectSecondaryView(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunScene05(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapCallScene03(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunTileScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunPairedObjectInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);


Lufia2ExecutionResult Lufia2WorldMapApplyRegionRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapBuildScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapReadPlayerInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapMaybeReadPlayerInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRestoreEncounterScene(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapAnimateEncounterTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapCheckEncounter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapAnimateMenuReturn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapClearPlayerScenePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapApplyMenuReturnTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapMaybeOpenMenu(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunSession(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunSceneSequence(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapFinishDisplayReset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRefreshObjectPose(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack0(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack1(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack3(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack4(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunSceneTrack5(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapUpdateSceneActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunSceneScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapStepSceneScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapStartSceneActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapStartSceneActors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapApplyFadeIn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapRunGroupScene0(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunGroupScene1(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene0(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene1(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene3(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapRunResourceScene4(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapPositionSceneGroup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapPrepareSceneWindowTables(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapDarkenScenePalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2WorldMapInstallScenePalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapUploadSceneGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadSceneBuffer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapLoadAlternateSceneBuffer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2WorldMapSendSoundCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#ifdef __cplusplus
}
#endif

#endif
