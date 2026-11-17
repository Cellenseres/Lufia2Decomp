#ifndef LUFIA2_BATTLE_FILL_STEPS_H
#define LUFIA2_BATTLE_FILL_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_FILL_STEPS 1
#define LUFIA2_BATTLE_FILL_MAX_CELLS 4u
#define LUFIA2_BATTLE_FILL_CELL_OPCODES 5u

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleFillCellsSpan(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, unsigned cells);

#ifdef __cplusplus
}
#endif
#endif
