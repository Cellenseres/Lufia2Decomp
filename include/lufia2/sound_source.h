#ifndef LUFIA2_SOUND_SOURCE_H
#define LUFIA2_SOUND_SOURCE_H

#include <stdbool.h>
#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Lufia2SoundSourcePair {
    uint16_t next;
    uint8_t last_byte;
    bool complete;
} Lufia2SoundSourcePair;

/* Bank-crossing pairs remain with the original caller. */
Lufia2SoundSourcePair Lufia2SoundReadSourcePair(
    const Lufia2Memory *memory, uint8_t bank, uint16_t direct_page, uint16_t source);

#ifdef __cplusplus
}
#endif

#endif
