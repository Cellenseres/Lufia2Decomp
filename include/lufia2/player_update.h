#ifndef LUFIA2_PLAYER_UPDATE_H
#define LUFIA2_PLAYER_UPDATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t (*Lufia2ReadByte)(void *context, uint16_t address);

typedef struct Lufia2PlayerSlotSpecialMemory {
    Lufia2ReadByte read_byte;
    void *context;
} Lufia2PlayerSlotSpecialMemory;

typedef enum Lufia2PlayerSlotSpecialAction {
    LUFIA2_PLAYER_SLOT_NO_CHILD = 0,
    LUFIA2_PLAYER_SLOT_SPECIAL_CHILD = 1,
    LUFIA2_PLAYER_SLOT_STANDARD_CHILD = 2,
} Lufia2PlayerSlotSpecialAction;

typedef struct Lufia2PlayerSlotSpecialResult {
    uint8_t action;
    uint8_t accumulator_low;
    uint8_t negative;
    uint8_t zero;
} Lufia2PlayerSlotSpecialResult;

/* Verified semantic model of $83:BBF3. The abstract reader preserves
 * original early-exit read order without exposing a platform or emulator
 * state type. */
Lufia2PlayerSlotSpecialResult Lufia2PlayerSlotSpecialUpdate(
    const Lufia2PlayerSlotSpecialMemory *memory);

#ifdef __cplusplus
}
#endif

#endif
