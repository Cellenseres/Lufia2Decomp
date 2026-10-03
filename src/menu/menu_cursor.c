/* Menu cursor placement: the 16-bit multiply and the two routines that turn an
 * item's row and column into its cursor position. */

#include "core/cpu_internal.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"

enum {
    FACTOR_A = 0x1570u,
    FACTOR_B = 0x1572u,
    PRODUCT_LOW = 0x1574u,
    PRODUCT_HIGH = 0x1576u,
    MULTIPLY_BITS = 0x0010u,
    ITEM_COLUMN = 0x14ebu,
    ITEM_BASE = 0x14d9u,
    ITEM_STRIDE = 0x14f1u,
    ITEM_INDEX = 0x14b3u,
    PIXEL_X_LOW = 0x138du,
    PIXEL_X_HIGH = 0x13bdu,
    PIXEL_Y_LOW = 0x13edu,
    PIXEL_Y_HIGH = 0x141du,
    CELL_WIDTH = 0x14fdu,
    CELL_HEIGHT = 0x1503u,
    ORIGIN_X_LOW = 0x14cdu,
    ORIGIN_X_HIGH = 0x14d3u,
    ORIGIN_Y_LOW = 0x14dfu,
    ORIGIN_Y_HIGH = 0x14e5u
};

/* Rotates a word through carry the way a read-modify-write does: the high
 * byte is stored first. */
static void RotateRightWord(
    Lufia2Wram wram, Lufia2CpuState *cpu, uint32_t location) {
    const uint16_t old = WramRead16(wram, location);
    const uint32_t low = WramAddress(wram, location, 0);
    const uint16_t value = (uint16_t)((old >> 1) | (cpu->carry ? 0x8000u : 0u));

    cpu->carry = old & 1u;
    Write8(wram.memory, WramNextAddress(location, low), (uint8_t)(value >> 8));
    Write8(wram.memory, low, (uint8_t)value);
}

/* $82:8000 (JSL): $1574/$1576 = $1570 * $1572 by shift and add. The
 * multiplier word is rotated away (the incoming carry enters its top), and X
 * and P are kept. */
Lufia2ExecutionResult Lufia2MenuMultiply(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint8_t bits;

    PushIndex(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    WramWrite16(wram, PRODUCT_LOW, 0);
    WramWrite16(wram, PRODUCT_HIGH, 0);
    for (bits = MULTIPLY_BITS; bits != 0; --bits) {
        RotateRightWord(wram, cpu, FACTOR_B);
        if (cpu->carry) {
            LoadA16(cpu, WramRead16(wram, PRODUCT_HIGH));
            cpu->carry = 0;
            Add16Value(cpu, WramRead16(wram, FACTOR_A));
            WramWrite16(wram, PRODUCT_HIGH, cpu->accumulator);
        }
        RotateRightWord(wram, cpu, PRODUCT_HIGH);
        RotateRightWord(wram, cpu, PRODUCT_LOW);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x828027u);
}

static void Multiply(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x82u, return_address);
    (void)Lufia2MenuMultiply(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $82:88A0: item X's list index $14B3 = base + column * stride. */
Lufia2ExecutionResult Lufia2MenuItemIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16At(wram, ITEM_STRIDE, cpu->x));
    And16(cpu, 0x00ffu);
    WramWrite16(wram, FACTOR_A, cpu->accumulator);
    LoadA16(cpu, WramRead16At(wram, ITEM_COLUMN, cpu->x));
    And16(cpu, 0x00ffu);
    WramWrite16(wram, FACTOR_B, cpu->accumulator);
    Multiply(memory, cpu, 0x88b7u);
    LoadA16(cpu, WramRead16At(wram, ITEM_BASE, cpu->x));
    And16(cpu, 0x00ffu);
    cpu->carry = 0;
    Add16Value(cpu, WramRead16(wram, PRODUCT_LOW));
    WramWrite16(wram, PRODUCT_LOW, cpu->accumulator);
    WramWrite16(wram, ITEM_INDEX, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x8288cau);
}

/* One axis: product of the cell size and a count, plus a 16-bit origin. */
static void PlaceAxis(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint32_t size, uint32_t count, uint32_t origin_low, uint32_t origin_high,
    uint32_t out_low, uint32_t out_high, uint16_t return_address) {
    LoadA8(cpu, WramReadAt(wram, size, cpu->x));
    WramWrite(wram, FACTOR_A, A8(cpu));
    WramWrite(wram, FACTOR_A + 1u, 0);
    LoadA8(cpu, WramReadAt(wram, count, cpu->x));
    WramWrite(wram, FACTOR_B, A8(cpu));
    WramWrite(wram, FACTOR_B + 1u, 0);
    Multiply(memory, cpu, return_address);
    LoadA8(cpu, WramRead(wram, PRODUCT_LOW));
    cpu->carry = 0;
    Adc8(cpu, WramReadAt(wram, origin_low, cpu->x));
    WramWriteAt(wram, out_low, cpu->x, A8(cpu));
    LoadA8(cpu, WramRead(wram, PRODUCT_LOW + 1u));
    Adc8(cpu, WramReadAt(wram, origin_high, cpu->x));
    WramWriteAt(wram, out_high, cpu->x, A8(cpu));
}

/* $82:88CB: item X's pixel position: cell width * column + the horizontal
 * origin, and cell height * row + the vertical origin; M1. */
Lufia2ExecutionResult Lufia2MenuItemPosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x8288cbu);
    PlaceAxis(memory, cpu, wram, CELL_WIDTH, ITEM_BASE, ORIGIN_X_LOW,
        ORIGIN_X_HIGH, PIXEL_X_LOW, PIXEL_X_HIGH, 0x88e0u);
    PlaceAxis(memory, cpu, wram, CELL_HEIGHT, ITEM_COLUMN, ORIGIN_Y_LOW,
        ORIGIN_Y_HIGH, PIXEL_Y_LOW, PIXEL_Y_HIGH, 0x8909u);
    return ExecutionReturned(0x82891du);
}
