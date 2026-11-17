#ifndef LUFIA2_NMI_JOYPAD_GATE_H
#define LUFIA2_NMI_JOYPAD_GATE_H

#include "lufia2/execution.h"

#define LUFIA2_NMI_JOYPAD_GATE 1
#define LUFIA2_NMI_JOYPAD_GATE_OPCODES 3u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2NmiPollJoypadReady(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif
#endif
