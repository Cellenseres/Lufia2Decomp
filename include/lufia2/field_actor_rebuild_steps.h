#ifndef LUFIA2_FIELD_ACTOR_REBUILD_STEPS_H
#define LUFIA2_FIELD_ACTOR_REBUILD_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_FIELD_ACTOR_REBUILD_STEPS_API 1
#define LUFIA2_FIELD_ACTOR_REBUILD_MAX_OPCODES 16u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2FieldRebuildActorStep(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu, uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
