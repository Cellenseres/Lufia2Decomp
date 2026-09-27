#ifndef LUFIA2_FIELD_FIELD_INTERNAL_H
#define LUFIA2_FIELD_FIELD_INTERNAL_H

/* Field subsystem internals shared across modules. */

#include "lufia2/execution.h"

/* $83:80CD: field idle test; zero = no event running. */
void Lufia2FieldIdleBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:CBAE: event slot timers; 0 = LLE at resume_pc. */
/* $80:BFAA: find key A in the $7E:F000 list at [X], stride B;
   carry clear = found. 0 = handoff at $80:BFBC. */
uint8_t Lufia2FieldListSearch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t return_bank,
    uint16_t return_address);

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

/* $80:EBAA section table at $7F:D000 from the header at DB:[$5D];
   leaves DB = $7F. M1/X0 (A8, X/Y16). */
Lufia2ExecutionResult Lufia2FieldReadSections(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:EC18 pack bits 4-5 of every fourth cell word of section $05AA
   into $7F:C000 (DB = $7F). M1/X0 (A8, X/Y16). */
Lufia2ExecutionResult Lufia2FieldPackSectionAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:EC78 size of section $05AA into $05B9/$05BB (DB = $7F). */
Lufia2ExecutionResult Lufia2FieldSectionSize(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $80:EC98 (M0/X0 (A16, X/Y16)) decompress resource A, then $29 & $0F + $166 when
   $29 is set, to $7F:[$2D]; returns M1. */
Lufia2ExecutionResult Lufia2FieldDecompressMapData(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#endif
