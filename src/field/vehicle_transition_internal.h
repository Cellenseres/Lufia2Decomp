#ifndef LUFIA2_FIELD_VEHICLE_TRANSITION_INTERNAL_H
#define LUFIA2_FIELD_VEHICLE_TRANSITION_INTERNAL_H

#include "lufia2/field.h"

Lufia2ExecutionResult Lufia2FieldProbeVehicleTerrainBody(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#endif
