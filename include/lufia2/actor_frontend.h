#ifndef LUFIA2_ACTOR_FRONTEND_H
#define LUFIA2_ACTOR_FRONTEND_H

/* Deprecated umbrella; include lufia2/decomp.h instead. */

#include "lufia2/decomp.h"

typedef Lufia2BusReadByte Lufia2ActorBusReadByte;
typedef Lufia2BusWriteByte Lufia2ActorBusWriteByte;
typedef Lufia2Memory Lufia2ActorFrontendMemory;
typedef Lufia2CpuState Lufia2ActorFrontendCpu;
typedef Lufia2ExecutionFlow Lufia2ActorPrimaryUpdateFlow;
typedef Lufia2ExecutionResult Lufia2ActorPrimaryUpdateResult;

#define LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED LUFIA2_EXECUTION_RETURNED
#define LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY LUFIA2_EXECUTION_BOUNDARY
#define LUFIA2_ACTOR_PRIMARY_UPDATE_CHILD_UNWOUND LUFIA2_EXECUTION_CHILD_UNWOUND

#endif
