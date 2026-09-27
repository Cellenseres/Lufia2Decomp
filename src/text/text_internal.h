#ifndef LUFIA2_TEXT_TEXT_INTERNAL_H
#define LUFIA2_TEXT_TEXT_INTERNAL_H

/* Shared text engine internals. */

#include "lufia2/execution.h"

/* Shared script fetch/pointer helpers include their original JSR frames. */
void Lufia2TextNextByte(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address);
void Lufia2TextSetScriptPointer(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address);

/* $84:8328: clear the window buffer $7E:3000-37FF and $099C bit 0. */
void Lufia2TextWindowClear(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address);

/* $80:9CB8 text step: known paths native, exact continuations for unknown children. */
Lufia2ExecutionResult Lufia2TextEngineStepBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2ExecutionResult result);

/* $80:9C80: text box waits for its timer or the A/X buttons. */
Lufia2ExecutionResult Lufia2TextPromptTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2ExecutionResult result);

#endif
