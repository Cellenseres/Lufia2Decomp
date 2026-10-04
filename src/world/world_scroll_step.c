/* Direction and distance produce the signed world-map scroll step. */

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
    ANGLE_FULL = 0x40u,
    STEP_STACK_MIN = 0x1e00u,
    SCROLL_STACK_MIN = 0x1e02u,
    WORLD_STACK_MAX = 0x1ffcu
};

/* Scroll offsets are 24-bit; wrapped positions use twelve bits. */
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

/* $86:A583: hardware 16-by-8 multiplication into a 24-bit product. */
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
    middle.value = WramRead16(wram, PRODUCT_MIDDLE);
    middle = Sum16Mode(middle.value,
        WramRead16(wram, HARDWARE_PRODUCT), false, cpu->decimal);
    WramWrite16(wram, PRODUCT_MIDDLE, middle.value);
    cpu->x = low_product;
    LeaveSum(cpu, middle);
    return ExecutionReturned(0x86a5a8u);
}

/* World positions and angle inputs are low work-RAM fields. */
static bool WorldMotionRamBank(uint8_t bank) {
    return bank < 0x40u || bank == 0x7eu || bank == 0x7fu ||
        (bank >= 0x80u && bank < 0xc0u);
}

/* Scale the distance by the selected angle-table entry. */
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

/* A nonzero negative word leaves $FF in the accumulator low byte. */
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

/* $86:A417: signed step offsets from direction and distance; M1X0. */
Lufia2ExecutionResult Lufia2WorldStepOffsets(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t distance;
    uint8_t angle;
    uint8_t quadrant;
    unsigned index;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page != 0u || cpu->stack < STEP_STACK_MIN || cpu->stack > WORLD_STACK_MAX ||
        !WorldMotionRamBank(cpu->data_bank))
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
            (uint8_t)(Difference8Mode(ANGLE_FULL,
                WramRead(wram, STEP_ANGLE), cpu->decimal).value << 1),
            0xa454u);
    } else {
        WramWrite16(wram, PRODUCT_MIDDLE, distance);
        WramWrite16(wram, SAVED_SCALE, 0);
    }
    quadrant = WramRead(wram, QUADRANT);
    index = (unsigned)((quadrant << 1) & 0x06u);
    SimulateJsrFrame(memory, cpu, 0xa460u);
    QuadrantOffsets(cpu, wram, index >> 1);
    SimulateRtsFrame(memory, cpu);
    cpu->x = (uint16_t)index;
    cpu->carry = (quadrant & 0x80u) != 0;
    return ExecutionReturned(0x86a461u);
}

/* The scroll offset integer word is cleared before adding the step. */
static void AddStep(Lufia2Wram wram, bool decimal, uint16_t offset, uint8_t step_low,
    uint8_t step_high) {
    Word16Result word;
    Byte8Result top;

    WramWrite16(wram, offset + 1u, 0);
    word.value = WramRead16(wram, step_low);
    word = Sum16Mode(word.value, WramRead16(wram, offset), false, decimal);
    WramWrite16(wram, offset, word.value);
    top.value = WramRead(wram, step_high);
    top = Sum8Mode(top.value, WramRead(wram, offset + 2u),
        word.carry, decimal);
    WramWrite(wram, offset + 2u, top.value);
}

/* Retain the position sum flags before wrapping to twelve bits. */
static Word16Result MovePosition(Lufia2Wram wram, bool decimal, uint16_t position,
    uint8_t saved, uint16_t offset, uint16_t tile) {
    Word16Result sum;
    uint16_t wrapped;

    sum.value = WramRead16(wram, position);
    WramWrite16(wram, saved, sum.value);
    sum = Sum16Mode(sum.value, WramRead16(wram, offset + 1u), false, decimal);
    wrapped = sum.value & POSITION_MASK;
    WramWrite16(wram, position, wrapped);
    WramWrite16(wram, tile, wrapped >> TILE_SHIFT);
    return sum;
}

/* $86:995B: advance scroll offsets and wrapped tile positions. */
Lufia2ExecutionResult Lufia2WorldScrollAdvance(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Word16Result sum;
    uint16_t tile;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page != 0u || cpu->stack < SCROLL_STACK_MIN || cpu->stack > WORLD_STACK_MAX ||
        !WorldMotionRamBank(cpu->data_bank))
        return ExecutionHandoff(cpu, 0x86995bu);
    SimulateJsrFrame(memory, cpu, 0x995du);
    (void)Lufia2WorldStepOffsets(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    AddStep(wram, cpu->decimal, OFFSET_X, STEP_X_LOW, STEP_X_HIGH);
    AddStep(wram, cpu->decimal, OFFSET_Y, STEP_Y_LOW, STEP_Y_HIGH);
    (void)MovePosition(wram, cpu->decimal, POSITION_X, SAVED_POSITION_X, OFFSET_X, TILE_X);
    sum = MovePosition(wram, cpu->decimal, POSITION_Y, SAVED_POSITION_Y, OFFSET_Y, TILE_Y);
    /* The last shift down leaves the bit shifted out in the carry. */
    tile = (uint16_t)((sum.value & POSITION_MASK) >> TILE_SHIFT);
    cpu->carry = ((sum.value >> (TILE_SHIFT - 1u)) & 1u) != 0;
    cpu->overflow = sum.overflow;
    cpu->accumulator = tile;
    SetNz16(cpu, tile);
    return ExecutionReturned(0x8699beu);
}
