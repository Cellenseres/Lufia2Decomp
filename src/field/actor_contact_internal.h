#ifndef LUFIA2_ACTOR_CONTACT_INTERNAL_H
#define LUFIA2_ACTOR_CONTACT_INTERNAL_H

#include <stdbool.h>
#include "lufia2/decomp.h"

bool Lufia2FieldActorOverlapsPlayer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#endif
