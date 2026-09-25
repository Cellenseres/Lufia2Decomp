#ifndef LUFIA2_SYSTEM_SYSTEM_INTERNAL_H
#define LUFIA2_SYSTEM_SYSTEM_INTERNAL_H

/* Shared game services. */

#include "lufia2/actor_frontend.h"

void Lufia2CallRandomScale(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address);

void Lufia2CallRandomByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address);

#endif
