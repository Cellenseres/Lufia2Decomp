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

/* $82:8000: $1576:$1574 = $1570 * $1572, from a JSL. */
void Lufia2CallMultiply(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address);

/* $80:8450: sine of angle A; bit 7 = minus. */
void Lufia2CallSine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address);

/* $80:8486: cosine of angle A; bit 7 = minus. */
void Lufia2CallCosine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address);

#endif
