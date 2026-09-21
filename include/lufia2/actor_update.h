#ifndef LUFIA2_ACTOR_UPDATE_H
#define LUFIA2_ACTOR_UPDATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t (*Lufia2ActorReadByte)(void *context, uint32_t wram_offset);
typedef void (*Lufia2ActorWriteByte)(
    void *context, uint32_t wram_offset, uint8_t value);
typedef void (*Lufia2ActorSlotCallback)(void *context, uint8_t slot);

typedef struct Lufia2ActorUpdateContext {
    Lufia2ActorReadByte read_byte;
    Lufia2ActorWriteByte write_byte;
    Lufia2ActorSlotCallback player_slot_special;
    Lufia2ActorSlotCallback actor_primary;
    Lufia2ActorSlotCallback actor_secondary;
    uint8_t decimal_mode;
    void *context;
} Lufia2ActorUpdateContext;

/*
 * Draft semantic reconstruction of $83:BB93.
 *
 * It preserves the 40-slot traversal, WRAM updates, read order and child-call
 * decisions. CPU register/flag and non-local-return details remain the
 * responsibility of a future consumer bridge before this routine can replace
 * the original function.
 */
void Lufia2UpdateActorSlots(const Lufia2ActorUpdateContext *context);

#ifdef __cplusplus
}
#endif

#endif
