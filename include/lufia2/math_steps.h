#ifndef LUFIA2_MATH_STEPS_H
#define LUFIA2_MATH_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_DIVIDE_BIT_SPANS 1
#define LUFIA2_DIVIDE_SPAN_MAX_BITS 2u
#define LUFIA2_DIVIDE_BIT_MAX_OPCODES 7u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2DivideBitSpan(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry, unsigned max_bits);
#ifdef __cplusplus
}
#endif
#endif
