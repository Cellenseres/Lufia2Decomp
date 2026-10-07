#ifndef LUFIA2_FIELD_H
#define LUFIA2_FIELD_H

/* Field loop, NMI, scrolling, sprites and triggers. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Scene-script operands at DB:Y; either M width, X16. */
Lufia2ExecutionResult Lufia2SceneScriptReadOperand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $83:F91F: copy tile bits from cell X to cell Y, keeping Y's attrs; M0X0. */
Lufia2ExecutionResult Lufia2FieldCopyCellTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $83:B062 restores Field display state and republishes its NMI callback. */
Lufia2ExecutionResult Lufia2FieldRestore(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $83:83E0 encounter handoff; the $83:83EB child uses its pushed JSL frame. */
Lufia2ExecutionResult Lufia2FieldEncounterHandoff(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $83:845B encounter transition through Battle and Field continuation. */
Lufia2ExecutionResult Lufia2EncounterBattleSequence(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context);

/* $83:80CD field idle test (X8); zero = idle. */
Lufia2ExecutionResult Lufia2FieldIdleTest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:83A0 field menu request check. */
Lufia2ExecutionResult Lufia2FieldMenuRequest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:867B consume pressed buttons in A. */
Lufia2ExecutionResult Lufia2FieldTakeButtons(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:8103 field $05B7 requests. */
Lufia2ExecutionResult Lufia2FieldStatusRequests(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:EF8E: scene graphics loading; PB80, binary, restores caller M/X. */
Lufia2ExecutionResult Lufia2FieldLoadSceneGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $80:F2F3: scene display setup; PB80, M1, accepts either X width. */
Lufia2ExecutionResult Lufia2FieldSetSceneDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:F338: scene palette copy; PB80, binary, accepts all M/X entry widths. */
Lufia2ExecutionResult Lufia2FieldCopyScenePalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:F35B: four planar tiles with original flip attributes; PB80, M0X0, binary. */
Lufia2ExecutionResult Lufia2FieldCopyMetatileGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:F3F1: mirror the two plane bytes in A; PB80, M0, either X width. */
Lufia2ExecutionResult Lufia2FieldMirrorPlaneWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:F40A: mirror low A, clear high A and retain the original carry; PB80, M1. */
Lufia2ExecutionResult Lufia2FieldMirrorPlaneByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $83:85DC completes the map reload; PB83, any native M/X widths. */
Lufia2ExecutionResult Lufia2FieldReloadMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $8E:B09C prepares pixel and cell scroll; PB8E, binary, any M/X widths. */
Lufia2ExecutionResult Lufia2FieldPrepareCameraScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $83:85DC field reload setup; loading stays LLE. */
Lufia2ExecutionResult Lufia2FieldReloadSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:8682 animation slot ticks at $7F:D057. */
Lufia2ExecutionResult Lufia2FieldAnimationTicks(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:9C72 idle frame: effects, event timer, text gate. */
Lufia2ExecutionResult Lufia2FieldEventTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:CBAE event slot timers; a due slot runs on LLE. */
Lufia2ExecutionResult Lufia2FieldEventTimerTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:81C6 field trigger checks; exact LLE boundaries. */
Lufia2ExecutionResult Lufia2FieldTriggerUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:B66E stair rectangles at $7E:F000. */
Lufia2ExecutionResult Lufia2FieldStairRects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:B711 event rectangles; hits run on LLE. */
Lufia2ExecutionResult Lufia2FieldEventRects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:B747 area rectangles; hits run on LLE. */
Lufia2ExecutionResult Lufia2FieldAreaRects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:AEB5 palette cycles and HDMA wave table. */
Lufia2ExecutionResult Lufia2FieldColourEffects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:9FA9 field NMI uploads (DMA, fade, windows). */
Lufia2ExecutionResult Lufia2FieldNmiUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $8E:BD77 BG scroll targets for three layers. */
Lufia2ExecutionResult Lufia2FieldScrollUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:F4FD/F518/F589/F5A2: queue the newly exposed map edge. */
Lufia2ExecutionResult Lufia2FieldStreamRightColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2FieldStreamLeftColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2FieldStreamTopRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2FieldStreamBottomRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:F47A / $83:8E66: rebuild one layer or all four layers; M=1. */
Lufia2ExecutionResult Lufia2FieldRedrawLayer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2FieldRedrawAllLayers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $83:A21A field OAM from the Y-sorted visible actors. */
Lufia2ExecutionResult Lufia2FieldActorSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Optional host visibility filter; none keeps stock behavior. */
typedef struct Lufia2FieldActorVisibility {
    uint16_t horizontal_padding;
    uint8_t (*accept)(void *context, uint16_t world_x, uint16_t world_y);
    void *context;
} Lufia2FieldActorVisibility;

Lufia2ExecutionResult Lufia2FieldActorSpritesWithVisibility(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    const Lufia2FieldActorVisibility *visibility);

/* $83:B53B: regular map resources, attribute fill and setup. M1, either X
 * width, native binary mode. Checkpoints B548 and loaded-only B580. */
Lufia2ExecutionResult Lufia2FieldInstallMap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context);

/* $80:EAE7: map resources and section headers; M1, either X width.
 * Child calls retain their original pushed frames and dispatch boundaries. */
Lufia2ExecutionResult Lufia2FieldLoadMapResources(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $83:B5D3: map header copy, object attributes and startup events. */
Lufia2ExecutionResult Lufia2FieldLoadMapHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $80:E722: queue the event list entry Y/2, using slot zero if full. */
Lufia2ExecutionResult Lufia2FieldStartEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $80:E844: clear event work records and select the map event script. */
Lufia2ExecutionResult Lufia2FieldInitializeMapEvents(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);


/* Field-object tile and sprite setup; return PCs are recorded in metadata. */
Lufia2ExecutionResult Lufia2FieldObjectLayer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ActorPositionToObjectProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSetObjectOrigin(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ActorResetObjectOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPendingTileOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSetMapTileNumber(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldReleaseClaimedActors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldCopyObjectPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPrepareObjectOrigin(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldStartObjectEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldUploadFixedGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSetupObjectActorSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);


/* Placed-object and actor transitions, with exact original child frames. */
Lufia2ExecutionResult Lufia2FieldLoadObjectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldInitializeObjectActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldRefreshObjectActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldPlaceActorObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldRebuildActorObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldClaimPlacedObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);


/* Object record lookup and attribute-grid addressing; native M1/X16. */
Lufia2ExecutionResult Lufia2FieldFindHeaderRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldFindPendingObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldObjectAttributeCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);


/* $83:8D42 sets pixel scroll and the overlapping coarse cache; preserves M/X. */
Lufia2ExecutionResult Lufia2FieldSetupLayerScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Layer coordinate scaling; X16, the scale calls change M1 to M0. */
Lufia2ExecutionResult Lufia2FieldLayerScaleMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPrepareCoordinateScale(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldScaleCoordinateRight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldScaleCoordinateLeft(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* X prepares either M width; Y enters M0. Both return M1. */
Lufia2ExecutionResult Lufia2FieldPrepareLayerScrollX(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPrepareLayerScrollY(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Object state bits; M1/X16, index and ROM mask retain the direct page. */
Lufia2ExecutionResult Lufia2FieldObjectBitIndex(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldObjectBitTest(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldObjectBitSet(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldObjectBitClear(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Clipped object-region rendering; M1/X16, explicit long-loop continuation. */
Lufia2ExecutionResult Lufia2FieldRenderLayerPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldRenderRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Region coordinates; M0/either X width, negative flag selects direct page. */
Lufia2ExecutionResult Lufia2FieldPixelCellFloor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPixelCellCeiling(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $83:8C8A refreshes the pending rectangle; restores caller M/X. */
Lufia2ExecutionResult Lufia2FieldRefreshObjectAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Object attributes, saved tiles, redraw requests and tile-bit copying. */
Lufia2ExecutionResult Lufia2FieldUpdatePlacedObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldClearObjectTileBit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldRenderObjectLayers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldClearObjectTileIds(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPlacePendingObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldMarkObjectAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldRestoreObjectTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldQueueObjectRedraw(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldCopyObjectTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $83:83EB runs the field battle transition; PB83/M1X16, binary. */
Lufia2ExecutionResult Lufia2FieldBattleTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $83:B76E applies a map rectangle destination; PB83/M1X16, binary. */
Lufia2ExecutionResult Lufia2FieldApplyAreaTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $80:ED9C map cell attributes of map A; M=1. */
Lufia2ExecutionResult Lufia2FieldBuildAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:94D4 steps the five scene value tracks of $1210; carry set when a
 * script ends. DP0, DB maps scene RAM; any M, X16, leaves M8. JSR/RTS. */
Lufia2ExecutionResult Lufia2SceneTrackStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:A791 screen origin and scroll words from the view position at
 * $11E8/$11EA. DP0, DB maps scene RAM; M8/X16. JSR/RTS. */
Lufia2ExecutionResult Lufia2SceneViewOrigin(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:ED0E unpacks four 2-bit fields per source byte; JSL, any width,
 * leaves M8/X16. DP0, S=$1F00..$1FFC and a nonzero rounded dimension
 * product; unsupported entries retain the original path. */
Lufia2ExecutionResult Lufia2FieldUnpackAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:C195 marks object slots 5 through 39; M8/X16, RTL, any DP/DB.
 * Native selection keeps S=$1F00..$1FFC clear of the slot bytes. */
Lufia2ExecutionResult Lufia2FieldMarkObjectSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:F821 traces the edge of the walkable region from the party cell and
 * folds the marks into the attribute bytes; any width, JSL. Native entries
 * require PB80, DP0, binary arithmetic, low-WRAM DB and S1F09..1FFC.
 * A read-only edge replay rejects unsafe tables or walks before writes. */
Lufia2ExecutionResult Lufia2FieldTraceCellEdges(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $83:F9D0 computes the cell pointer from column $8F and row $91;
 * M8/X16, RTL, any DP/DB, S=$1F04..$1FFC. Retains both nested frames. */
Lufia2ExecutionResult Lufia2FieldCellPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Rebuild scene actors, restoring all caller M/X widths. */
Lufia2ExecutionResult Lufia2FieldRebuildSceneActors(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* Session setup hands control to the original field system. */
Lufia2ExecutionResult Lufia2FieldBeginSessionSetup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);
Lufia2ExecutionResult Lufia2FieldResumeSessionSetup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2ObjectFinePositionToProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ObjectScaleFinePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MapProbeTileHeight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MapProbeCellOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MapCellOffsetLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MapPackedAttributeCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldObjectAttributeCellLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MapPackedAttributeCellLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ObjectInterpolateCoordinate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ObjectApproachCoordinate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldProbeObjectAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldProbeObjectState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldProbeObjectProperties(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPendingObjectState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldFindPendingObjectLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldReadProbeAttribute(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldClearPendingOccupancy(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldFindSpecialActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldProbeInputAllowed(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldUpdateProbeAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldProbeDirectionBlocked(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSaveProbePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldRestoreProbePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ObjectProbeNextTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSaveObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldRestoreObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldNormalizeObjectOrigin(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldMarkRegionObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldFindPointRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldFindRectangleRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldClearObjectAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSetObjectTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldStartEventAtProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldStartPositionEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPrepareObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldCopyEventObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldRemoveRegionObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldRestoreRegionObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldUpdateEventObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldReadObjectRegionPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldReadObjectRegionDestination(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldReadObjectRegionArea(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldProbeLeaderPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldAcknowledgeControlChange(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSetFollowingObjectDrawFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ObjectWakeMatchingPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ObjectRemoveSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SceneScriptReadWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SceneScriptSeekRelative(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SceneScriptFindRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPrepareEventControl(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldBeginEventControl(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldFindActorAtProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldApplyObjectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SceneScriptSelectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSetPendingProbePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ObjectStartInteractionEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2ObjectAllocateSpriteResources(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldCanPushObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldClaimActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldPushPendingObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSaveActorSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldRestoreActorSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSetObjectDrawFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldLoadObjectActionHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldLoadObjectControlHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldLoadObjectRegionHeader(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSelectObjectCondition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldResolveObjectCondition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSaveAnimationRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldRestoreAnimationRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldRedrawAnimatedRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldQueueObjectControlSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldCloseAnimatedRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldOpenAnimatedRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldApplyInitialObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldApplyAlternateObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldAnimateObjectAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldAnimateObjectControl(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldAnimationTickSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2FieldFlagAnimationRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldResetSpriteBuffer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldResetObjectAnimation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2FieldSelectSceneRecordBase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SceneResetTextState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2SceneRunStartRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SceneRunInitialRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SceneRunResumeRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SceneRunTransitionRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SceneRunMapText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SceneReadActorAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SceneReadActorCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SceneReadActorPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2SceneApplyActorCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
