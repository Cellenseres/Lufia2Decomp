#ifndef LUFIA2_SESSION_ACTORS_H
#define LUFIA2_SESSION_ACTORS_H

#include <stdbool.h>
#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Lufia2SessionPresentation {
    uint8_t mode;
    uint8_t value;
    bool changed;
} Lufia2SessionPresentation;

Lufia2SessionPresentation Lufia2FieldSelectSessionPresentation(uint8_t destination);
uint8_t Lufia2FieldRestoreSessionActor(
    const Lufia2Memory *memory, uint8_t bank,
    uint16_t source, uint16_t destination);
uint8_t Lufia2FieldRestoreReservedActor(
    const Lufia2Memory *memory, uint8_t bank, uint16_t destination);

#ifdef __cplusplus
}
#endif

#endif
