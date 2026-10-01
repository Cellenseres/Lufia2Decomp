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

#ifdef __cplusplus
}
#endif

#endif
