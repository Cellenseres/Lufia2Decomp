/* Battle palette brightness: scales a 15-bit colour component-wise with the
 * hardware multiplier, and applies that to a run of palette entries. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

/* Direct page. */
enum {
    DP_COLOR = 0x15u,      /* colour in, scaled colour out */
    DP_COLOR_HIGH = 0x16u,
    DP_FIRST_ENTRY = 0x11u,
    DP_LEVEL = 0x13u,      /* bit 7: darken towards black, else towards white */
    DP_REMAINING = 0x17u,
    DP_PALETTE_DIRTY = 0x73u
};

enum {
    SOURCE_PALETTE = 0x7ff1dbu,
    PALETTE_BUFFER = 0x0320u,
    ENTRY_COUNT = 15u,
    COMPONENT_MASK = 0x1fu,
    MIDDLE_BITS = 0x07c0u,
    MAX_COLOR = 0x7fffu,
    LEVEL_BASE = 0x40u,
    PALETTE_UPLOAD = 0x80u
};

/* Writes a word the way a read-modify-write does, high byte first. */
static void RewriteWord(Lufia2Wram wram, uint32_t location, uint16_t value) {
    const uint32_t low = WramAddress(wram, location, 0);

    Write8(wram.memory, WramNextAddress(location, low), (uint8_t)(value >> 8));
    Write8(wram.memory, low, (uint8_t)value);
}

/* TSB direct, 16-bit: the zero flag tells whether the bits were all clear. */
static bool SetBits16(Lufia2Wram wram, uint32_t location, uint16_t bits) {
    const uint16_t value = WramRead16(wram, location);

    RewriteWord(wram, location, (uint16_t)(value | bits));
    return (value & bits) == 0;
}

/* $81:B3F8: scales the colour in $15 by the multiplier set in $4202. Each
 * 5-bit component goes through the hardware multiplier, whose product is read
 * back a few instructions later: red and green come back as the product
 * shifted down, blue in the middle bits. The colour word is rebuilt in $15
 * and the exit registers are those the original leaves. */
static void ScaleColor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t color;
    uint16_t product;
    uint16_t high_word;
    uint16_t blue_bits;
    uint16_t old;

    WramWrite(wram, SNES_WRMPYB, WramRead(wram, DP_COLOR) & COMPONENT_MASK);
    color = WramRead16(wram, DP_COLOR);
    product = WramRead16(wram, SNES_RDMPYL);
    WramWrite(wram, SNES_WRMPYB, (color >> 5) & COMPONENT_MASK);
    /* The word at $16 holds the high byte of the colour; it is read before
     * the first product is stored over the colour. */
    high_word = WramRead16(wram, DP_COLOR_HIGH);
    WramWrite16(wram, DP_COLOR, product >> 5);
    product = WramRead16(wram, SNES_RDMPYL);
    WramWrite(wram, SNES_WRMPYB, (high_word >> 2) & COMPONENT_MASK);
    (void)SetBits16(wram, DP_COLOR, product & MIDDLE_BITS);
    blue_bits = (uint16_t)((WramRead16(wram, SNES_RDMPYL) & MIDDLE_BITS) << 4);
    old = WramRead16(wram, DP_COLOR);
    RewriteWord(wram, DP_COLOR, (uint16_t)(old >> 1));
    cpu->zero = SetBits16(wram, DP_COLOR, blue_bits);
    cpu->negative = false;
    cpu->carry = (old & 1u) != 0;
    cpu->y = product;
    cpu->accumulator = blue_bits;
    SetAccumulatorWidth(cpu, 0);
}

Lufia2ExecutionResult Lufia2BattleScaleColor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81b3f8u);
    ScaleColor(memory, cpu);
    return ExecutionReturned(0x81b443u);
}

/* JSR $B3F8 from inside the brightness loop. */
static void ScaleEntry(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    ScaleColor(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $81:B396: fades the 15 palette entries after entry $11 * 16 towards black
 * or white. The sign of $13 selects the direction, its low bits the level. */
Lufia2ExecutionResult Lufia2BattlePaletteBrightness(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint8_t level;
    uint16_t offset;
    uint16_t color;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        !DirectWorkWordAvailable(cpu, DP_REMAINING))
        return ExecutionHandoff(cpu, 0x81b396u);
    level = WramRead(wram, DP_LEVEL);
    /* The entry number and the high byte of the direct page, as one word. */
    offset = (uint16_t)((((uint16_t)WramRead(wram, DP_FIRST_ENTRY) << 8) |
                            (cpu->direct_page >> 8)) >> 3);
    WramWrite16(wram, DP_REMAINING, ENTRY_COUNT);
    if ((level & 0x80u) != 0) {
        const Byte8Result multiplier = Sum8Mode(level, LEVEL_BASE, false, cpu->decimal);

        WramWrite(wram, SNES_WRMPYA, multiplier.value);
        do {
            offset = (uint16_t)(offset + 2u);
            WramWrite16(wram, DP_COLOR,
                WramRead16At(wram, SOURCE_PALETTE, offset));
            ScaleEntry(memory, cpu, 0xb3bdu);
            color = WramRead16(wram, DP_COLOR);
            WramWrite16At(wram, PALETTE_BUFFER, offset, color);
        } while (WramStep8(wram, DP_REMAINING, -1) != 0);
        cpu->overflow = multiplier.overflow;
    } else {
        const Byte8Result multiplier = Difference8Mode(LEVEL_BASE, level, cpu->decimal);
        Word16Result lightened;

        WramWrite(wram, SNES_WRMPYA, multiplier.value);
        do {
            offset = (uint16_t)(offset + 2u);
            lightened = Difference16Mode(MAX_COLOR,
                WramRead16At(wram, SOURCE_PALETTE, offset), cpu->decimal);
            WramWrite16(wram, DP_COLOR, lightened.value);
            ScaleEntry(memory, cpu, 0xb3e3u);
            lightened = Difference16Mode(MAX_COLOR, WramRead16(wram, DP_COLOR), cpu->decimal);
            WramWrite16At(wram, PALETTE_BUFFER, offset, lightened.value);
        } while (WramStep16(wram, DP_REMAINING, -1) != 0);
        color = lightened.value;
        SetSumFlags(cpu, lightened);
    }
    cpu->x = offset;
    SetAccumulatorWidth(cpu, 1);
    cpu->accumulator = color;
    LoadA8(cpu, PALETTE_UPLOAD);
    WramWrite(wram, DP_PALETTE_DIRTY, PALETTE_UPLOAD);
    return ExecutionReturned(0x81b3f7u);
}
