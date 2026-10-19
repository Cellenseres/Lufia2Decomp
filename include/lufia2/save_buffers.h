#ifndef LUFIA2_SAVE_BUFFERS_H
#define LUFIA2_SAVE_BUFFERS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Lufia2SaveBuffer {
    LUFIA2_SAVE_FIELD_STATE,
    LUFIA2_SAVE_PACKED_ITEM_COUNTS
} Lufia2SaveBuffer;

typedef enum Lufia2SaveBufferDirection {
    LUFIA2_SAVE_TO_RECORD,
    LUFIA2_SAVE_FROM_RECORD
} Lufia2SaveBufferDirection;

typedef enum Lufia2SaveRecordTail {
    LUFIA2_SAVE_CAPSULE_STATS,
    LUFIA2_SAVE_BATTLE_SUBMENU_CHOICES
} Lufia2SaveRecordTail;

/* X descends through state; Y advances through the record. */
Lufia2ExecutionResult Lufia2SaveCopyRecordTailSpan(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2SaveRecordTail tail, Lufia2SaveBufferDirection direction,
    unsigned max_bytes);

/* X advances through the record; Y descends through state. */
Lufia2ExecutionResult Lufia2SaveCopyBufferSpan(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2SaveBuffer buffer, Lufia2SaveBufferDirection direction,
    unsigned max_bytes);

#ifdef __cplusplus
}
#endif

#endif
