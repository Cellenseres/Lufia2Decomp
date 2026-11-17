#ifndef LUFIA2_BATTLE_QUEUE_STEPS_H
#define LUFIA2_BATTLE_QUEUE_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_BATTLE_QUEUE_PHASES 1
#define LUFIA2_QUEUE_BEGIN_HEADER_OPCODES 3u
#define LUFIA2_QUEUE_ROW_HEADER_OPCODES 2u
#define LUFIA2_QUEUE_ADVANCE_OPCODES 8u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2BattleBeginVramQueueSearch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleContinueVramQueueSearch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleReadVramQueueRowStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2BattleAdvanceVramQueueRowStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
#ifdef __cplusplus
}
#endif
#endif
