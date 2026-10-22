#ifndef LUFIA2_MENU_CURSOR_STEP_H
#define LUFIA2_MENU_CURSOR_STEP_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Lufia2MenuSlideAxis {
    LUFIA2_MENU_SLIDE_HORIZONTAL,
    LUFIA2_MENU_SLIDE_VERTICAL
} Lufia2MenuSlideAxis;

typedef enum Lufia2MenuSlidePhase {
    LUFIA2_MENU_SLIDE_ERROR,
    LUFIA2_MENU_SLIDE_POSITION
} Lufia2MenuSlidePhase;

/* Retain the correction and frame-count child boundaries. */
Lufia2ExecutionResult Lufia2MenuAdvanceCursorSlide(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2MenuSlideAxis axis, Lufia2MenuSlidePhase phase);

#ifdef __cplusplus
}
#endif

#endif
