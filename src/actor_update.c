#include "lufia2/actor_update.h"

enum {
    LUFIA2_WRAM_CURRENT_SLOT = 0x00a7,
    LUFIA2_WRAM_SLOT_A8 = 0x00a8,
    LUFIA2_WRAM_SLOT_A9 = 0x00a9,
    LUFIA2_WRAM_SLOT_AA = 0x00aa,
    LUFIA2_WRAM_SLOT_AB = 0x00ab,
    LUFIA2_WRAM_SLOT_AC = 0x00ac,
    LUFIA2_WRAM_SLOT_AD = 0x00ad,

    LUFIA2_WRAM_ACTOR_FLAGS = 0x0622,
    LUFIA2_WRAM_STATE_09A1 = 0x09a1,

    LUFIA2_WRAM_STATE_D0A1 = 0x1d0a1,
    LUFIA2_WRAM_STATE_D0FE = 0x1d0fe,
    LUFIA2_WRAM_STATE_E216 = 0x1e216,

    LUFIA2_ACTOR_SLOT_COUNT = 0x28,
};

static uint8_t Read8(
    const Lufia2ActorUpdateContext *context, uint32_t offset) {
    return context->read_byte(context->context, offset);
}

static void Write8(
    const Lufia2ActorUpdateContext *context,
    uint32_t offset,
    uint8_t value) {
    context->write_byte(context->context, offset, value);
}

static void RunSlotCallback(
    Lufia2ActorSlotCallback callback, void *context, uint8_t slot) {
    if (callback != 0)
        callback(context, slot);
}

static uint8_t AddSlotToDouble(
    uint8_t doubled, uint8_t slot, uint8_t decimal_mode) {
    unsigned result;

    if (!decimal_mode)
        return (uint8_t)(doubled + slot);

    result = (doubled & 0x0fu) + (slot & 0x0fu);
    if (result > 0x09u)
        result = ((result + 0x06u) & 0x0fu) + 0x10u;
    result = (doubled & 0xf0u) + (slot & 0xf0u) + result;
    if (result > 0x9fu)
        result += 0x60u;
    return (uint8_t)result;
}

static void PrepareSlotOffsets(
    const Lufia2ActorUpdateContext *context, uint8_t slot) {
    const uint8_t doubled = (uint8_t)(slot << 1);
    const uint8_t tripled =
        AddSlotToDouble(doubled, slot, context->decimal_mode);

    /*
     * $83:AB4F as reached from $83:BB93. The caller constrains slot to 0..39,
     * so the byte-width ASL cannot carry. ADC still observes the live decimal
     * flag, which is carried explicitly by the portable context.
     */
    Write8(context, LUFIA2_WRAM_CURRENT_SLOT, slot);
    Write8(context, LUFIA2_WRAM_SLOT_A8, 0);
    Write8(context, LUFIA2_WRAM_SLOT_A9, doubled);
    Write8(context, LUFIA2_WRAM_SLOT_AA, 0);
    Write8(context, LUFIA2_WRAM_SLOT_AB, tripled);
    Write8(context, LUFIA2_WRAM_SLOT_AC, 0);
    Write8(context, LUFIA2_WRAM_SLOT_AD, 0);
}

void Lufia2UpdateActorSlots(const Lufia2ActorUpdateContext *context) {
    if (Read8(context, LUFIA2_WRAM_STATE_D0FE) != 0)
        Write8(context, LUFIA2_WRAM_STATE_E216, 1);

    for (uint8_t slot = 0; slot < LUFIA2_ACTOR_SLOT_COUNT; ++slot) {
        uint8_t flags;

        PrepareSlotOffsets(context, slot);
        flags = Read8(context, LUFIA2_WRAM_ACTOR_FLAGS + slot);

        if ((flags & 0x04u) != 0)
            continue;

        if ((flags & 0x18u) != 0) {
            const uint8_t state = Read8(context, LUFIA2_WRAM_STATE_D0A1);
            if ((state & 0x2cu) == 0 || slot == 0)
                RunSlotCallback(
                    context->actor_primary, context->context, slot);
        } else if ((flags & 0x01u) == 0) {
            if (slot == 0) {
                RunSlotCallback(
                    context->player_slot_special, context->context, slot);
            } else {
                const uint8_t state = Read8(context, LUFIA2_WRAM_STATE_D0A1);
                if ((state & 0x2cu) == 0)
                    RunSlotCallback(
                        context->actor_primary, context->context, slot);
            }
        }

        RunSlotCallback(context->actor_secondary, context->context, slot);
    }

    if (Read8(context, LUFIA2_WRAM_STATE_D0FE) != 0)
        Write8(context, LUFIA2_WRAM_STATE_E216, 3);

    {
        const uint8_t state = Read8(context, LUFIA2_WRAM_STATE_09A1);
        if (state != 0xffu)
            Write8(context, LUFIA2_WRAM_STATE_09A1, (uint8_t)(state & 0x7fu));
    }
}
