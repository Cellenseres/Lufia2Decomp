#ifndef LUFIA2_ACTOR_ACTOR_SLOT_VIEW_H
#define LUFIA2_ACTOR_ACTOR_SLOT_VIEW_H

/* One actor slot over the parallel per-slot WRAM arrays. */

#include "core/cpu_internal.h"
#include "system/wram.h"

typedef struct Lufia2ActorSlotView {
    const Lufia2Memory *memory;
    const Lufia2CpuState *cpu;
    uint16_t index;
} Lufia2ActorSlotView;

/* Number of actor slots walked by $83:BB93. */
#define ACTOR_SLOT_COUNT WRAM_ACTOR_STATE_COUNT

static inline Lufia2ActorSlotView Lufia2ActorSlotAt(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t index) {
    Lufia2ActorSlotView view;

    view.memory = memory;
    view.cpu = cpu;
    view.index = index;
    return view;
}

/* Mirrored arrays follow DB; bank-$7F arrays are long. */
static inline uint32_t Lufia2ActorSlotMirrored(
    const Lufia2ActorSlotView *slot, uint16_t array) {
    return AbsoluteIndexedAddress(slot->cpu, array, slot->index);
}

static inline uint32_t Lufia2ActorSlotLong(
    const Lufia2ActorSlotView *slot, uint32_t array) {
    return LongIndexedAddress(array, slot->index);
}

static inline uint8_t Lufia2ActorSlotReadMirrored(
    const Lufia2ActorSlotView *slot, uint16_t array) {
    return Read8(slot->memory, Lufia2ActorSlotMirrored(slot, array));
}

static inline void Lufia2ActorSlotWriteMirrored(
    const Lufia2ActorSlotView *slot, uint16_t array, uint8_t value) {
    Write8(slot->memory, Lufia2ActorSlotMirrored(slot, array), value);
}

static inline uint8_t Lufia2ActorSlotReadLong(
    const Lufia2ActorSlotView *slot, uint32_t array) {
    return Read8(slot->memory, Lufia2ActorSlotLong(slot, array));
}

static inline void Lufia2ActorSlotWriteLong(
    const Lufia2ActorSlotView *slot, uint32_t array, uint8_t value) {
    Write8(slot->memory, Lufia2ActorSlotLong(slot, array), value);
}

static inline uint8_t Lufia2ActorSlotState(const Lufia2ActorSlotView *slot) {
    return Lufia2ActorSlotReadMirrored(slot, WRAM_ACTOR_STATE);
}

static inline void Lufia2ActorSlotSetState(
    const Lufia2ActorSlotView *slot, uint8_t value) {
    Lufia2ActorSlotWriteMirrored(slot, WRAM_ACTOR_STATE, value);
}

static inline uint8_t Lufia2ActorSlotId(const Lufia2ActorSlotView *slot) {
    return Lufia2ActorSlotReadMirrored(slot, WRAM_ACTOR_ID);
}

static inline uint8_t Lufia2ActorSlotTileX(const Lufia2ActorSlotView *slot) {
    return Lufia2ActorSlotReadMirrored(slot, WRAM_ACTOR_TILE_X);
}

static inline uint8_t Lufia2ActorSlotTileY(const Lufia2ActorSlotView *slot) {
    return Lufia2ActorSlotReadMirrored(slot, WRAM_ACTOR_TILE_Y);
}

static inline uint8_t Lufia2ActorSlotPrimaryTimer(
    const Lufia2ActorSlotView *slot) {
    return Lufia2ActorSlotReadLong(slot, WRAM_ACTOR_PRIMARY_TIMER);
}

static inline void Lufia2ActorSlotSetPrimaryTimer(
    const Lufia2ActorSlotView *slot, uint8_t value) {
    Lufia2ActorSlotWriteLong(slot, WRAM_ACTOR_PRIMARY_TIMER, value);
}

static inline uint8_t Lufia2ActorSlotSecondaryTimer(
    const Lufia2ActorSlotView *slot) {
    return Lufia2ActorSlotReadLong(slot, WRAM_ACTOR_SECONDARY_TIMER);
}

static inline void Lufia2ActorSlotSetSecondaryTimer(
    const Lufia2ActorSlotView *slot, uint8_t value) {
    Lufia2ActorSlotWriteLong(slot, WRAM_ACTOR_SECONDARY_TIMER, value);
}

static inline uint8_t Lufia2ActorSlotWalkCounter(
    const Lufia2ActorSlotView *slot) {
    return Lufia2ActorSlotReadLong(slot, WRAM_ACTOR_WALK_COUNTER);
}

static inline void Lufia2ActorSlotSetWalkCounter(
    const Lufia2ActorSlotView *slot, uint8_t value) {
    Lufia2ActorSlotWriteLong(slot, WRAM_ACTOR_WALK_COUNTER, value);
}

#endif
