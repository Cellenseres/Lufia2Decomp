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

/* $86:81A9 NMI installed by $86:8000. */
Lufia2ExecutionResult Lufia2SelectScreenNmi(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
