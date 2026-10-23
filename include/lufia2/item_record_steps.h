#ifndef LUFIA2_ITEM_RECORD_STEPS_H
#define LUFIA2_ITEM_RECORD_STEPS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Save the caller status before resolving the item record. */
Lufia2ExecutionResult Lufia2ItemBeginRecordRead(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* Read the resolved record and restore the saved status. */
Lufia2ExecutionResult Lufia2ItemFinishRecordRead(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
