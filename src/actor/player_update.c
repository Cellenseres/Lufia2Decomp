#include "lufia2/player_update.h"

static void Load8(Lufia2PlayerSlotSpecialResult *result, uint8_t value) {
    result->accumulator_low = value;
    result->negative = (value & 0x80u) != 0;
    result->zero = value == 0;
}

static int BitImmediate(
    Lufia2PlayerSlotSpecialResult *result, uint8_t mask) {
    result->zero = (result->accumulator_low & mask) == 0;
    return !result->zero;
}

Lufia2PlayerSlotSpecialResult Lufia2PlayerSlotSpecialUpdate(
    const Lufia2PlayerSlotSpecialMemory *memory) {
    Lufia2PlayerSlotSpecialResult result = {
        LUFIA2_PLAYER_SLOT_NO_CHILD, 0, 0, 1};

    Load8(&result, memory->read_byte(memory->context, 0x09a8u));
    if (BitImmediate(&result, 0x08u))
        return result;

    Load8(&result, memory->read_byte(memory->context, 0x05b5u));
    if (BitImmediate(&result, 0x02u))
        return result;

    Load8(&result, memory->read_byte(memory->context, 0x05b7u));
    if (BitImmediate(&result, 0x07u))
        return result;

    Load8(&result, memory->read_byte(memory->context, 0x0622u));
    if (BitImmediate(&result, 0x80u))
        return result;

    Load8(&result, memory->read_byte(memory->context, 0x099bu));
    if (result.negative)
        return result;

    Load8(&result, memory->read_byte(memory->context, 0x09a7u));
    if (BitImmediate(&result, 0x01u))
        result.action = LUFIA2_PLAYER_SLOT_SPECIAL_CHILD;
    else
        result.action = LUFIA2_PLAYER_SLOT_STANDARD_CHILD;
    return result;
}
