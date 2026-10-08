#ifndef LUFIA2_MENU_H
#define LUFIA2_MENU_H

/* Menu and file-select screens. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $82:A318 status/equipment text selector; M1X0. */
Lufia2ExecutionResult Lufia2MenuDrawStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context);

/* $82:939C menu NMI; redraws stay LLE. */
Lufia2ExecutionResult Lufia2MenuNmi(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:8B4B menu buttons; carry set when none. */
Lufia2ExecutionResult Lufia2MenuButtons(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:9313 menu window refresh request. */
Lufia2ExecutionResult Lufia2MenuWindowRequest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C627 menu cursor blink timer. */
Lufia2ExecutionResult Lufia2MenuCursorBlink(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:8878 menu string $5F:Y at $7E:X; any width. */
Lufia2ExecutionResult Lufia2MenuDrawString(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* Optional number checkpoints before PHY at $80:8922, including nested strings. */
Lufia2ExecutionResult Lufia2MenuDrawStringWithCheckpoint(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionCheckpoint checkpoint, void *context);

/* $82:810E window frame at A, X = width, height; M0X0. */
Lufia2ExecutionResult Lufia2MenuDrawWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:80A5 fills a 4 x 4 tile block at $54, counting up from $5A.
 * M16/X16, native mode, DP=0, any DB, S=$1F02..$1FFC; RTS. */
Lufia2ExecutionResult Lufia2MenuTileBlockFill(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:8069 8 x 8 grid of tile blocks, then the redraw wait; any M, X16.
 * Requires a frame-wait child, DP0, binary arithmetic, S $1F04..$1FFC. */
Lufia2ExecutionResult Lufia2MenuTileGridFill(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $82:80CA palette number Y into the rectangle at A, X = width << 8 | rows,
 * then the redraw wait; M0X0, both dimensions 1..32.
 * Requires a child, DP0, binary arithmetic, S $1F04..$1FFC. */
Lufia2ExecutionResult Lufia2MenuRecolorRect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $82:838F clears both layers, then waits through the child; any M, X16.
 * Requires a child, DP0, binary arithmetic, S $1F04..$1FFC. */
Lufia2ExecutionResult Lufia2MenuClearLayers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $82:8000 (JSL) $1574/$1576 = $1570 * $1572; any widths. */
Lufia2ExecutionResult Lufia2MenuMultiply(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:88A0 list index of item X into $14B3; S $1F00..$1FFC. */
Lufia2ExecutionResult Lufia2MenuItemIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:88CB pixel position of item X; M1, S $1F00..$1FFC. */
Lufia2ExecutionResult Lufia2MenuItemPosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:8AD8 / $82:8AE9 slide corrections for slot Y. Native: M8, DP0,
 * hardware-mapped DB, Y <= $0800, S $1F00..$1FFC. */
Lufia2ExecutionResult Lufia2MenuSlideCorrectX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2MenuSlideCorrectY(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:8AFA slide step count. Requires a child, PB82, M8/X16, DP0, binary
 * arithmetic and S $1F10..$1FFC. Every sixteenth step calls the original
 * $86:8B55 sprite frame through the JSL at $82:8B02. */
Lufia2ExecutionResult Lufia2MenuSlideCount(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $82:89FA cursor slide from slot Y to slot X. Requires a child, PB82,
 * M8/X16, DP0, binary arithmetic, S $1F12..$1FFC, a register/low-WRAM DB
 * mirror and slots below 48. Spans above 127, sprite timers that expire
 * during the slide, descriptors outside ROM or $7E:2000+ and more than 128
 * OAM pieces hand off before writes. $82:8AFA runs through its JSR sites. */
Lufia2ExecutionResult Lufia2MenuCursorSlide(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $82:8720 moves the selected item from pressed buttons. PB82, M8/X16,
 * DP0, binary arithmetic, low-WRAM DB, S1F02..1FFC and item X below256.
 * The sound request is an original child; no button leaves carry set. */
Lufia2ExecutionResult Lufia2MenuCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $82:8B08 polls until a button counts, preserving original frame/text children.
 * PB82, M8/X16, DP0, binary arithmetic, low-WRAM DB, S1F04..1FFC;
 * the cursor slot minus5 must be below256. Original children may transfer
 * control with their live CPU and guest call frames preserved. */
Lufia2ExecutionResult Lufia2MenuInputLoop(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $86:8DD7 select screen video setup and empty layers; M1X0. Hands off at
 * $86:8B48 with its return pushed. */
/* Complete screen setup requires a child, PB86, M8/X16, DP0, binary
 * arithmetic and S $1F04..$1FFC. The original frame wait is preserved. */
Lufia2ExecutionResult Lufia2MenuScreenSetup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $86:8E6B clears 1000 sprite flag bytes; M8/X16, RTL, any DP/DB.
 * Native selection uses S=$1F00..$1FFC, outside the flag table. */
Lufia2ExecutionResult Lufia2SpriteClearSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:8CF5 sprite slot X plays animation A; M1X0. */
Lufia2ExecutionResult Lufia2SpriteSetAnimation(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:9F6F equipment commands window; M1X0. */
Lufia2ExecutionResult Lufia2MenuEquipCommands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:D721 shop windows; M1X0. */
Lufia2ExecutionResult Lufia2MenuShopWindows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:E49E shop kind title; M1X0. */
Lufia2ExecutionResult Lufia2MenuShopTitle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:EFC5 "Game saved." window; M1X0. */
Lufia2ExecutionResult Lufia2MenuSavedWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:F0A2 name entry windows; M1X0. */
Lufia2ExecutionResult Lufia2MenuNameEntryWindows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:D749 shop party screen with stats; M1X0. */
Lufia2ExecutionResult Lufia2MenuShopParty(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:A2E3 capsule monster screen; M1X0. */
Lufia2ExecutionResult Lufia2MenuCapsuleScreen(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:950E member level, HP, MP at X; M1X0. */
Lufia2ExecutionResult Lufia2MenuMemberStatus(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:D07B capsule status screen; M1X0. */
Lufia2ExecutionResult Lufia2MenuCapsuleStatus(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:E297 shop $30 kind and item lists; M1X0. */
Lufia2ExecutionResult Lufia2MenuShopSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:CD1F capsule may learn a skill; carry clear if so; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleTryLearn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:E5E1 shop equipment comparison; M1X0. */
Lufia2ExecutionResult Lufia2MenuShopCompare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:DCF4 one shop row (M0); M1X0. */
Lufia2ExecutionResult Lufia2MenuShopRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:DCC1 five shop rows; M1X0. */
Lufia2ExecutionResult Lufia2MenuShopRows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:B2C5 upgrade flagged equipment; M1X0. */
Lufia2ExecutionResult Lufia2MenuEquipUpgrade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:8CDA sprite slot X animation list from $8E:D9A9,Y; M1X0. */
Lufia2ExecutionResult Lufia2SpriteSetTable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:9CB2 warp destinations list; M1X0. */
Lufia2ExecutionResult Lufia2MenuWarpList(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:A918 list cursor and page by mode $09D1; M1X0. */
Lufia2ExecutionResult Lufia2MenuListCursor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:ACDB one list row at $3748; M1X0. */
Lufia2ExecutionResult Lufia2MenuListRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:8B55 animate, clear and build OAM, then the frame wait; any width. */
Lufia2ExecutionResult Lufia2SpriteFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:8B73 step the animation of every active slot; M1. */
Lufia2ExecutionResult Lufia2SpriteAnimateAll(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:8BCF OAM buffer off screen; M1X0. */
Lufia2ExecutionResult Lufia2SpriteClearOam(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:8BF5 OAM from the active slots; M1X0. */
Lufia2ExecutionResult Lufia2SpriteBuildOam(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:81A9 NMI installed by $86:8000. */
Lufia2ExecutionResult Lufia2SelectScreenNmi(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:9918 adjusts price X, preserving P and entry-width Y; binary mode. */
Lufia2ExecutionResult Lufia2AdjustPurchasePrice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $82:D905 spell-shop price and windows; M1X0, binary mode.
 * Optional checkpoint after both word stores, before LDA #$20 at D922. */
Lufia2ExecutionResult Lufia2MenuSpellShopSetup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context);

/* $82:8044 video transfer setup, then the frame wait; M8/X16, JSL */
/* Complete queue/upload parent: required child, PB82, M8/X16, DP0,
 * binary arithmetic and S $1F04..$1FFC; original $82:93C2 wait and RTL. */
Lufia2ExecutionResult Lufia2MenuQueueVideoWrite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $86:9009 copies 256 bytes: M8/X16, DP=0, DB in a WRAM mirror,
 * S=$1F04..$1FFC, prepared MVN/RTS stub, target=$2000..$FF00. RTS. */
Lufia2ExecutionResult Lufia2MenuCopyImageRow256(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:906A copies 128 bytes: X16, DP=0, DB in a WRAM mirror,
 * S=$1F04..$1FFC, prepared MVN/RTS stub, target=$2000..$FF80.
 * Any accumulator width and decimal mode; RTS. */
Lufia2ExecutionResult Lufia2MenuCopyImageRow128(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:8FF6 copies two rows: X16, DP=0, DB in a WRAM mirror,
 * S=$1F08..$1FFC, prepared MVN/RTS stub, target=$2000..$FD00.
 * Any accumulator width and decimal mode; RTS. */
Lufia2ExecutionResult Lufia2MenuCopyImageBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:9022 image grid and upload; X16, JSL */
/* Complete image parents require a child, PB86, DP0, X16, binary arithmetic,
 * S $1F10..$1FFC and the original MVN/RTS RAM stub. DB is a low-WRAM alias
 * or $7E. The set additionally requires M8, 1..7 list entries and each byte
 * index <=85. Copies stay outside scratch/frames. Unsupported entries hand
 * off before writes. The original upload wait owns frame progress. */
Lufia2ExecutionResult Lufia2MenuLoadImageGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $86:8F6F image set and upload; M8/X16, JSL */
Lufia2ExecutionResult Lufia2MenuLoadImageSet(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $86:90C0 palette block copy; X16, JSL */
Lufia2ExecutionResult Lufia2MenuLoadPalette0(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:90D3 palette block copy; X16, JSL */
Lufia2ExecutionResult Lufia2MenuLoadPalette1(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:90E6 palette block copy; X16, JSL */
Lufia2ExecutionResult Lufia2MenuLoadPalette2(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:90F9 palette block copy; X16, JSL */
Lufia2ExecutionResult Lufia2MenuLoadPalette3(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:910C palette block copy; X16, JSL */
Lufia2ExecutionResult Lufia2MenuLoadPalette4(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $86:911F slot palette blocks; JSL, X16, DP0,
 * count 1..7, S $1F00..$1FFC in the consumer. */
Lufia2ExecutionResult Lufia2MenuLoadSlotPalettes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MenuFormatNumberDigits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MenuAppendNumberDigit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MenuResetDisplayRequests(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MenuInitializeDisplayState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MenuLoadPrimaryGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuLoadAuxiliaryGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuLoadLargeGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuLoadMediumGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuInitializeAuxiliarySprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* Any entry widths; original menu preparation and loop. */
Lufia2ExecutionResult Lufia2MenuRunMainScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuRunMainInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuApplySpellList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuRunAlternateSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuRefreshMainDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuPlacePartyPortraits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuInitializePartyPortrait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuInitializePartyPortraits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuBuildMainWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuPresentMainDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuBuildAlternateCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuClearSpriteMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuLoadCursorGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuSetCursorStyle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuValidatePartyCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuBuildCursorPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuMoveCursorPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuFadeDisplayIn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuFadeDisplayOut(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuPrepareDisplayText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuReplaceCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuInitializeListScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuBuildListWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuRunListInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuRunListSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuSelectedListOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuScrollListPage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuInitializeScrollStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuSetScrollPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuInitializeScrollRange(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuInitializeScrollSprite(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuLoadListParameters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuInitializeListCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuDrawListPage(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuMoveListMarkerDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuMoveListMarkerUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuAnimateListUp(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuAnimateListDown(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuWriteGlyph(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MenuAdvanceTextRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MenuResetTextWidth(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MenuSetTextPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2MenuWriteCharacter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuWriteRawControl(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2MenuClearTileRectangle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Original cached menu selection and saved cursor state. */
Lufia2ExecutionResult Lufia2MenuRestoreSelectionState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
