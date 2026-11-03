#ifndef LUFIA2_NAME_ENTRY_INTERNAL_H
#define LUFIA2_NAME_ENTRY_INTERNAL_H

#include "lufia2/execution.h"

/* Shared original body; callers retain their entry contracts. */
Lufia2ExecutionResult Lufia2MenuNameEntryActions(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

#endif
