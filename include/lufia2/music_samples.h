#ifndef LUFIA2_MUSIC_SAMPLES_H
#define LUFIA2_MUSIC_SAMPLES_H

#include <stdbool.h>
#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Lufia2MusicSampleSearch {
    uint16_t slot;
    uint8_t value;
    bool found;
} Lufia2MusicSampleSearch;

uint16_t Lufia2MusicClearSampleSlots(const Lufia2Memory *memory, uint16_t slot);
Lufia2MusicSampleSearch Lufia2MusicFindSample(
    const Lufia2Memory *memory, uint16_t slot, uint8_t sample);

#ifdef __cplusplus
}
#endif

#endif
