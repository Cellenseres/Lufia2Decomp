#ifndef LUFIA2_SYSTEM_SYSTEM_INTERNAL_H
#define LUFIA2_SYSTEM_SYSTEM_INTERNAL_H

/* Shared game services. */

#include "lufia2/execution.h"

void Lufia2CallRandomScale(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

void Lufia2CallRandomByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

#endif
