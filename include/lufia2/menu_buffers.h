#ifndef LUFIA2_MENU_BUFFERS_H
#define LUFIA2_MENU_BUFFERS_H

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Lufia2MenuFieldBuffer {
    LUFIA2_MENU_FIELD_SNAPSHOT,
    LUFIA2_MENU_MESSAGE_SNAPSHOT
} Lufia2MenuFieldBuffer;

/* Bounded original buffer loops; X is the byte index. */
Lufia2ExecutionResult Lufia2MenuRestoreFieldBufferSpan(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2MenuFieldBuffer buffer, unsigned max_bytes);

#ifdef __cplusplus
}
#endif

#endif
