#ifndef LUFIA2_BATTLE_BACKGROUND_STEPS_H
#define LUFIA2_BATTLE_BACKGROUND_STEPS_H
#include "lufia2/execution.h"

#define LUFIA2_BACKGROUND_WORD_SPANS 1
#define LUFIA2_BACKGROUND_SPAN_MAX_WORDS 4u
#define LUFIA2_BACKGROUND_WORD_OPCODES 7u

#ifdef __cplusplus
extern "C" {
#endif
Lufia2ExecutionResult Lufia2BattleDecodeBackgroundWordStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);
Lufia2ExecutionResult Lufia2BattleDecodeBackgroundWordsSpan(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, unsigned words);
#ifdef __cplusplus
}
#endif
#endif
