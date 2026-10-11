#ifndef LUFIA2_FIELD_INDEXED_PROBE_INTERNAL_H
#define LUFIA2_FIELD_INDEXED_PROBE_INTERNAL_H
#include "lufia2/field.h"
Lufia2ExecutionResult Lufia2FieldProbeCollisionBody(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry);
#endif
