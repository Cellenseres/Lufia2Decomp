#ifndef LUFIA2_TEXT_TEXT_INTERNAL_H
#define LUFIA2_TEXT_TEXT_INTERNAL_H

/* Text engine internals used by the field loop. */

#include "lufia2/actor_frontend.h"

/* $84:8328: clear the window buffer $7E:3000-37FF and $099C bit 0. */
void Lufia2TextWindowClear(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address);

/* $80:9CB8 text step: plain characters native, the rest on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2TextEngineStepBody(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result);

/* $80:9C80: text box waits for its timer or the A/X buttons. */
Lufia2ActorPrimaryUpdateResult Lufia2TextPromptTick(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result);

#endif
