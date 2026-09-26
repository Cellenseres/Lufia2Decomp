#ifndef LUFIA2_ITEM_H
#define LUFIA2_ITEM_H

/* Item and spell records. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $81:F1C5 item $0A06: name $0B77, record $0B84; any width. */
Lufia2ExecutionResult Lufia2LoadItemRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F414 spell $0A0B: name and record $0B77; M1X0. */
Lufia2ExecutionResult Lufia2LoadSpellRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F194 A = first record byte of item $0A06; any width. */
Lufia2ExecutionResult Lufia2ItemRecordByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
