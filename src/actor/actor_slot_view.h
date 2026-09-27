#ifndef LUFIA2_ACTOR_ACTOR_SLOT_VIEW_H
#define LUFIA2_ACTOR_ACTOR_SLOT_VIEW_H

/* Semantic views of one actor slot over the game's parallel per-slot arrays.
 *
 * WRAM keeps actor fields as separate arrays indexed by the slot, not as a
 * packed record, so a view is only a handle: the memory interface, the CPU
 * whose data bank the mirrored arrays follow, and the index register value
 * the original code uses. Every accessor is one bus access in the original
 * order; nothing is cached or copied into host structures. Field names come
 * from metadata/memory_map.toml through system/wram.h. */

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

/* Element addresses. Arrays in the low-WRAM mirror are read as absolute,X
 * (or ,Y) and therefore follow the live data bank; bank-$7F arrays are
 * long,X. Indexing carries like the CPU. */
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

/* Named fields ($7E:0622 actor_state, $7E:05FA actor_id, $7E:06BA/$06E2
 * actor_tile_x/y, $7F:E3C6/E35E the primary and secondary timers, $7F:E48E
 * the walk counter). Fields with neutral unk_ names use the generic
 * accessors above with their WRAM_UNK_ constant. */
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
