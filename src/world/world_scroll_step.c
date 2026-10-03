/* World map scroll step: a distance and an angle become signed offsets, and
 * the offsets move the scroll position. */

#include <stdbool.h>

#include "core/cpu_ops.h"
#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/world_map.h"

/* Direct page: the multiplication, and the step that is worked out. */
enum {
    MULTIPLICAND = 0x4eu,
    MULTIPLIER = 0x50u,
    PRODUCT = 0x51u,
    PRODUCT_MIDDLE = 0x52u,
    PRODUCT_TOP = 0x53u,
    SAVED_SCALE = 0x0eu,
    QUADRANT = 0x10u,
    STEP_X_LOW = 0x08u,
    STEP_X_HIGH = 0x0au,
    STEP_Y_LOW = 0x0bu,
    STEP_Y_HIGH = 0x0du,
    STEP_ANGLE = 0x05u
};

/* Absolute addresses of the data bank, and the scale table in ROM. */
enum {
    ANGLE = 0x1248u,
    DISTANCE = 0x1249u,
    SCALE_TABLE = 0x97b226u,
    HARDWARE_A = 0x4202u,
    HARDWARE_B = 0x4203u,
    HARDWARE_PRODUCT = 0x4216u
};

/* The angle byte: six bits of angle, then the quadrant. */
enum {
    ANGLE_BITS = 6u,
    ANGLE_MASK = 0x3fu,
    ANGLE_FULL = 0x40u
};

/* Scroll state: 24-bit offsets (a fraction byte, then a word), positions in
 * 12 bits, and the positions divided by 16. */
enum {
    OFFSET_X = 0x11ecu,
    OFFSET_Y = 0x11efu,
    POSITION_X = 0x11e8u,
    POSITION_Y = 0x11eau,
    TILE_X = 0x11f2u,
    TILE_Y = 0x11f4u,
    SAVED_POSITION_X = 0x04u,
    SAVED_POSITION_Y = 0x06u,
    POSITION_MASK = 0x0fffu,
    TILE_SHIFT = 4u
};

/* $86:A583: 24-bit product of the word at $4E and the byte at $50 stored
 * from $51; the multiplier is fed one byte of the word at a time. M8/X16. */
Lufia2ExecutionResult Lufia2WorldProduct16By8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t low_product;
    uint8_t high_byte;
    Word16Result middle;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86a583u);
    WramWrite(wram, HARDWARE_A, WramRead(wram, MULTIPLIER));
    WramWrite(wram, HARDWARE_B, WramRead(wram, MULTIPLICAND));
    high_byte = WramRead(wram, MULTIPLICAND + 1u);
    low_product = WramRead16(wram, HARDWARE_PRODUCT);
    WramWrite16(wram, PRODUCT, low_product);
    WramWrite(wram, HARDWARE_B, high_byte);
    WramWrite(wram, PRODUCT_TOP, 0);
    middle = Sum16(WramRead16(wram, PRODUCT_MIDDLE),
        WramRead16(wram, HARDWARE_PRODUCT), false);
    WramWrite16(wram, PRODUCT_MIDDLE, middle.value);
    cpu->x = low_product;
    LeaveSum(cpu, middle);
    return ExecutionReturned(0x86a5a8u);
}

/* JSR $A583 from the step routine, with the multiplier taken from the scale
 * table at the given index. Both of its entry widths are met here. */
static void ScaleBy(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2Wram wram, uint16_t index, uint16_t last_byte) {
    WramWrite(wram, MULTIPLIER,
        Read8(memory, LongIndexedAddress(SCALE_TABLE, index)));
    SimulateJsrFrame(memory, cpu, last_byte);
    (void)Lufia2WorldProduct16By8(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* The sign extension byte of a negated word: $FF unless it came out zero. */
static uint8_t SignByte(uint16_t negated) {
    return negated == 0 ? 0u : 0xffu;
}

/* A negated word is in the accumulator and its sign byte has been stored:
 * the low byte holds $FF unless the word is zero, and the flags describe
 * that byte. */
static void LeaveNegation(Lufia2CpuState *cpu, uint16_t negated) {
    cpu->accumulator = negated;
    if (negated != 0)
        LoadA8(cpu, 0xffu);
    else
        SetNz16(cpu, 0);
}

/* The four quadrant handlers selected through the table at $A46B. */
static void QuadrantOffsets(Lufia2CpuState *cpu, Lufia2Wram wram,
    unsigned quadrant) {
    uint16_t negated;

    switch (quadrant) {
    case 0:
        negated = (uint16_t)(0u - WramRead16(wram, SAVED_SCALE));
        WramWrite16(wram, STEP_X_LOW, negated);
        WramWrite(wram, STEP_X_HIGH, SignByte(negated));
        negated = (uint16_t)(0u - WramRead16(wram, PRODUCT_MIDDLE));
        WramWrite16(wram, STEP_Y_LOW, negated);
        WramWrite(wram, STEP_Y_HIGH, SignByte(negated));
        LeaveNegation(cpu, negated);
        break;
    case 1:
        WramWrite16(wram, STEP_Y_LOW, WramRead16(wram, SAVED_SCALE));
        negated = (uint16_t)(0u - WramRead16(wram, PRODUCT_MIDDLE));
        WramWrite16(wram, STEP_X_LOW, negated);
        WramWrite(wram, STEP_X_HIGH, SignByte(negated));
        WramWrite(wram, STEP_Y_HIGH, 0);
        LeaveNegation(cpu, negated);
        break;
    case 2:
        WramWrite16(wram, STEP_X_LOW, WramRead16(wram, SAVED_SCALE));
        WramWrite16(wram, STEP_Y_LOW, WramRead16(wram, PRODUCT_MIDDLE));
        WramWrite(wram, STEP_X_HIGH, 0);
        WramWrite(wram, STEP_Y_HIGH, 0);
        LeaveWord(cpu, WramRead16(wram, PRODUCT_MIDDLE));
        break;
    default:
        WramWrite16(wram, STEP_X_LOW, WramRead16(wram, PRODUCT_MIDDLE));
        negated = (uint16_t)(0u - WramRead16(wram, SAVED_SCALE));
        WramWrite16(wram, STEP_Y_LOW, negated);
        WramWrite(wram, STEP_Y_HIGH, SignByte(negated));
        WramWrite(wram, STEP_X_HIGH, 0);
        LeaveNegation(cpu, negated);
        break;
    }
}

/* $86:A417: signed offsets $08/$0A (horizontal) and $0B/$0D (vertical) for
 * the distance at $1249 in the direction $1248 (angle in the low six
 * bits, quadrant in the top two). M8/X16. The overflow flag is that of the
 * last multiplication, or the caller's when the angle is zero; the carry
 * leaves clear. */
Lufia2ExecutionResult Lufia2WorldStepOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t distance;
    uint8_t angle;
    unsigned index;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86a417u);
    distance = WramRead16(wram, DISTANCE);
    WramWrite16(wram, MULTIPLICAND, distance);
    WramWrite(wram, QUADRANT, WramRead(wram, ANGLE) >> ANGLE_BITS);
    angle = WramRead(wram, ANGLE) & ANGLE_MASK;
    if (angle != 0) {
        WramWrite(wram, STEP_ANGLE, angle);
        ScaleBy(memory, cpu, wram, (uint16_t)(angle << 1), 0xa43du);
        WramWrite16(wram, SAVED_SCALE, WramRead16(wram, PRODUCT_MIDDLE));
        ScaleBy(memory, cpu, wram,
            (uint16_t)((ANGLE_FULL - WramRead(wram, STEP_ANGLE)) << 1),
            0xa454u);
    } else {
        WramWrite16(wram, PRODUCT_MIDDLE, distance);
        WramWrite16(wram, SAVED_SCALE, 0);
    }
    index = (unsigned)((WramRead(wram, QUADRANT) << 1) & 0x06u);
    SimulateJsrFrame(memory, cpu, 0xa460u);
    QuadrantOffsets(cpu, wram, index >> 1);
    SimulateRtsFrame(memory, cpu);
    cpu->x = (uint16_t)index;
    cpu->carry = false;
    return ExecutionReturned(0x86a461u);
}

/* Adds a 24-bit step to a 24-bit scroll offset; the word above the
 * fraction byte is cleared first. */
static void AddStep(Lufia2Wram wram, uint16_t offset, uint8_t step_low,
    uint8_t step_high) {
    Word16Result word;
    Byte8Result top;

    WramWrite16(wram, offset + 1u, 0);
    word = Sum16(WramRead16(wram, step_low), WramRead16(wram, offset), false);
    WramWrite16(wram, offset, word.value);
    top = Sum8(WramRead(wram, step_high), WramRead(wram, offset + 2u),
        word.carry);
    WramWrite(wram, offset + 2u, top.value);
}

/* Wraps a position by its offset to 12 bits and stores it divided by 16.
 * The sum is returned for the flags it leaves. */
static Word16Result MovePosition(Lufia2Wram wram, uint16_t position,
    uint8_t saved, uint16_t offset, uint16_t tile) {
    Word16Result sum;
    uint16_t wrapped;

    WramWrite16(wram, saved, WramRead16(wram, position));
    sum = Sum16(WramRead16(wram, position), WramRead16(wram, offset + 1u),
        false);
    wrapped = sum.value & POSITION_MASK;
    WramWrite16(wram, position, wrapped);
    WramWrite16(wram, tile, wrapped >> TILE_SHIFT);
    return sum;
}

/* $86:995B: applies the step of $86:A417 to the 24-bit scroll offsets at
 * $11EC and $11EF, then wraps the positions $11E8 and $11EA to 12 bits and
 * stores them divided by 16 at $11F2 and $11F4. M8/X16. */
Lufia2ExecutionResult Lufia2WorldScrollAdvance(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Word16Result sum;
    uint16_t tile;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86995bu);
    SimulateJsrFrame(memory, cpu, 0x995du);
    (void)Lufia2WorldStepOffsets(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    AddStep(wram, OFFSET_X, STEP_X_LOW, STEP_X_HIGH);
    AddStep(wram, OFFSET_Y, STEP_Y_LOW, STEP_Y_HIGH);
    (void)MovePosition(wram, POSITION_X, SAVED_POSITION_X, OFFSET_X, TILE_X);
    sum = MovePosition(wram, POSITION_Y, SAVED_POSITION_Y, OFFSET_Y, TILE_Y);
    /* The last shift down leaves the bit shifted out in the carry. */
    tile = (uint16_t)((sum.value & POSITION_MASK) >> TILE_SHIFT);
    cpu->carry = ((sum.value >> (TILE_SHIFT - 1u)) & 1u) != 0;
    cpu->overflow = sum.overflow;
    cpu->accumulator = tile;
    SetNz16(cpu, tile);
    return ExecutionReturned(0x8699beu);
}
