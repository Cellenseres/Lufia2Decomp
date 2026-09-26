#ifndef LUFIA2_FIELD_FIELD_INTERNAL_H
#define LUFIA2_FIELD_FIELD_INTERNAL_H

/* Field subsystem internals shared across modules. */

#include "lufia2/execution.h"

/* $83:80CD: field idle test; zero = no event running. */
void Lufia2FieldIdleBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:CBAE: event slot timers; 0 = LLE at resume_pc. */
/* Read-only: cells $83:8E85 draws for layer X; ~0 for no layer. */
uint32_t Lufia2FieldRegionCells(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu);

/* $83:8E85: redraw region $7F:D046/D04C in layer X. */
void Lufia2FieldRedrawRegion(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address);

/* $83:8E66: redraw all four BG layers. */
void Lufia2FieldRedrawLayers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

uint8_t Lufia2FieldEventTimerBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned *passes);

/* $84:8000: screen effects of $1261 and the $1262 palette fade. */
void Lufia2FieldScreenEffects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#endif
