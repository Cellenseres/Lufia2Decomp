#ifndef LUFIA2_SOUND_PROGRESS_H
#define LUFIA2_SOUND_PROGRESS_H

#include <stdbool.h>
#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Lufia2SoundHeaderProgress {
    uint16_t source;
    uint16_t remaining;
} Lufia2SoundHeaderProgress;

typedef struct Lufia2SoundPayloadProgress {
    uint16_t remaining;
    bool more;
} Lufia2SoundPayloadProgress;

Lufia2SoundHeaderProgress Lufia2SoundAdvanceHeader(
    const Lufia2Memory *memory, uint16_t direct_page,
    uint16_t source, uint16_t remaining);
Lufia2SoundPayloadProgress Lufia2SoundAdvancePayload(
    const Lufia2Memory *memory, uint16_t direct_page, uint16_t remaining);

#ifdef __cplusplus
}
#endif

#endif
