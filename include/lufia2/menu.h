#ifndef LUFIA2_MENU_H
#define LUFIA2_MENU_H

/* Menu and file-select screens. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* $86:81A9 NMI installed by $86:8000. */
Lufia2ExecutionResult Lufia2SelectScreenNmi(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
