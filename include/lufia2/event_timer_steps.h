#ifndef LUFIA2_EVENT_TIMER_STEPS_H
#define LUFIA2_EVENT_TIMER_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_EVENT_TIMER_STEPS_API 1
#define LUFIA2_EVENT_TIMER_MAX_OPCODES 25u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2FieldEventTimerStep(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu, uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
