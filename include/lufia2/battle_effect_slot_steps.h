#ifndef LUFIA2_BATTLE_EFFECT_SLOT_STEPS_H
#define LUFIA2_BATTLE_EFFECT_SLOT_STEPS_H
#include "lufia2/execution.h"
#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2BattleEffectScriptSlotStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectActorSlotStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectScriptAdvanceStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectActorAdvanceStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleEffectDispatchPrepareStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry);
enum { LUFIA2_BATTLE_EFFECT_CLEAR_SPAN_MAX_BYTES = 32u };
Lufia2ExecutionResult Lufia2BattleEffectClearByteSpan(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, unsigned max_bytes);
Lufia2ExecutionResult Lufia2BattleEffectClearBytesStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
#ifdef __cplusplus
}
#endif
#endif
