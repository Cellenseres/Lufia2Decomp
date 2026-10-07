#ifndef LUFIA2_FIELD_COORDINATES_INTERNAL_H
#define LUFIA2_FIELD_COORDINATES_INTERNAL_H

#include "lufia2/field.h"

void Lufia2ObjectRoundedProbe(const Lufia2Memory *memory, Lufia2CpuState *cpu);
void Lufia2MapProbeHeightBody(const Lufia2Memory *memory, Lufia2CpuState *cpu);
void Lufia2ObjectInterpolateCoordinateBody(const Lufia2Memory *memory, Lufia2CpuState *cpu);
void Lufia2ObjectApproachCoordinateBody(const Lufia2Memory *memory, Lufia2CpuState *cpu);

#endif
