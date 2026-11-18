#ifndef LUFIA2_BATTLE_ACTION_WORK_STEPS_H
#define LUFIA2_BATTLE_ACTION_WORK_STEPS_H

#include "lufia2/execution.h"

#define LUFIA2_BATTLE_ACTION_COPY_SPANS 1
#define LUFIA2_BATTLE_ACTION_CLEAR_SPANS 1
#define LUFIA2_BATTLE_ACTION_CLEAR_MAX_WORDS 8u
#define LUFIA2_BATTLE_ACTION_RECORD_COPY_SPANS 1
#define LUFIA2_BATTLE_ACTION_RECORD_COPY_MAX_BYTES 5u

enum { LUFIA2_BATTLE_ACTION_COPY_SPAN_MAX_BYTES = 8u };

#ifdef __cplusplus
extern "C" {
#endif

Lufia2ExecutionResult Lufia2BattleCopyActionWorkSpan(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint32_t entry, unsigned max_bytes);

Lufia2ExecutionResult Lufia2BattleCopyActionRecordSpan(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       uint32_t entry,
                                                       unsigned max_bytes);

Lufia2ExecutionResult Lufia2BattleClearActionWordSpan(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      uint32_t entry,
                                                      unsigned max_words);

#ifdef __cplusplus
}
#endif
#endif
