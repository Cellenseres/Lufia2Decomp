#ifndef LUFIA2_BATTLE_GAUGE_STEPS_H
#define LUFIA2_BATTLE_GAUGE_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_BATTLE_GAUGE_STEPS 1
#define LUFIA2_GAUGE_BEGIN_OPCODES 4u
#define LUFIA2_GAUGE_READ_OPCODES 2u
#define LUFIA2_GAUGE_SCALE_OPCODES 5u
#define LUFIA2_GAUGE_PART_MAX_OPCODES 12u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2BattleBeginGaugeStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
Lufia2ExecutionResult Lufia2BattlePrepareGaugeScaleStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleGaugePartStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
#ifdef __cplusplus
}
#endif
#endif
