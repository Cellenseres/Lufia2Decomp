#ifndef LUFIA2_DECIMAL_DIGITS_STEPS_H
#define LUFIA2_DECIMAL_DIGITS_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_DECIMAL_DIGITS_STEPS_API 1
#define LUFIA2_DECIMAL_DIGITS_STEP_MAX_OPCODES 9u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2DecimalDigitsStep(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu, uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
