#ifndef LUFIA2_FIELD_FRAME_OPS_H
#define LUFIA2_FIELD_FRAME_OPS_H

#include <stdbool.h>
#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Lufia2FieldFrameAction {
    LUFIA2_FIELD_FRAME_WAIT,
    LUFIA2_FIELD_FRAME_TRANSITION,
    LUFIA2_FIELD_FRAME_CHANGE_MAP,
    LUFIA2_FIELD_FRAME_CHANGE_SCENE,
    LUFIA2_FIELD_FRAME_UPDATE
} Lufia2FieldFrameAction;

typedef struct Lufia2FieldFrameDecision {
    Lufia2FieldFrameAction action;
    uint8_t value;
    uint8_t second_pad;
    bool tested_second_pad;
} Lufia2FieldFrameDecision;

Lufia2FieldFrameDecision Lufia2FieldSelectFrameAction(
    const Lufia2Memory *memory, uint8_t bank, uint16_t direct_page);
uint16_t Lufia2FieldSwapTransitionMaps(
    const Lufia2Memory *memory, uint8_t bank, uint16_t stack);
void Lufia2FieldClearTransitionState(const Lufia2Memory *memory, uint8_t bank);

#ifdef __cplusplus
}
#endif

#endif
