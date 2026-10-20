#ifndef LUFIA2_MENU_WAIT_H
#define LUFIA2_MENU_WAIT_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Lufia2MenuWaitPoll {
    LUFIA2_MENU_WAIT_FRAME_BUTTONS,
    LUFIA2_MENU_WAIT_MOTION_BUTTONS,
    LUFIA2_MENU_WAIT_BUTTON_MASKS,
    LUFIA2_MENU_WAIT_FRAME_COUNT,
    LUFIA2_MENU_WAIT_SPRITE_COUNT,
    LUFIA2_MENU_WAIT_SPRITE_MOTION
} Lufia2MenuWaitPoll;

/* One finite test between the original child calls. */
Lufia2ExecutionResult Lufia2MenuPollInputWait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2MenuWaitPoll poll);

#ifdef __cplusplus
}
#endif

#endif
