#ifndef LUFIA2_FIELD_ACTOR_VISIBILITY_INTERNAL_H
#define LUFIA2_FIELD_ACTOR_VISIBILITY_INTERNAL_H

#include <stdbool.h>
#include "lufia2/execution.h"

bool Lufia2FieldRevealActorState(const Lufia2Memory *memory, Lufia2CpuState *cpu);
void Lufia2FieldHideActorState(const Lufia2Memory *memory, Lufia2CpuState *cpu);

#endif
