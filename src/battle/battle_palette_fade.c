/* Battle palette brightness: scales a 15-bit colour component-wise with the
 * hardware multiplier, and applies that to a run of palette entries. */

#include <stdbool.h>

#include "core/cpu_internal.h"
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

static void ShiftA16Right(Lufia2CpuState *cpu, unsigned count) {
    while (count--)
        LsrA16(cpu);
}

/* TSB direct, 16-bit. */
static void SetBitsDirect16(Lufia2Wram wram, Lufia2CpuState *cpu, uint32_t location) {
    const uint16_t value = WramRead16(wram, location);

    cpu->zero = (value & cpu->accumulator) == 0;
    RewriteWord(wram, location, (uint16_t)(value | cpu->accumulator));
}

/* $81:B3F8: scales the colour in $15 by the multiplier set in $4202. Each
 * 5-bit component goes through the hardware multiplier, whose product is read
 * back a few instructions later. Entered with X=0, leaves M=0. */
static void ScaleColor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t old;

    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, WramRead(wram, DP_COLOR));
    And8(cpu, COMPONENT_MASK);
    WramWrite(wram, SNES_WRMPYB, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, DP_COLOR));
    ShiftA16Right(cpu, 5);
    SetAccumulatorWidth(cpu, 1);
    And8(cpu, COMPONENT_MASK);
    LoadY16(cpu, WramRead16(wram, SNES_RDMPYL));
    WramWrite(wram, SNES_WRMPYB, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    ShiftA16Right(cpu, 5);
    LoadY16(cpu, WramRead16(wram, DP_COLOR_HIGH));
    WramWrite16(wram, DP_COLOR, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LsrA8(cpu);
    LsrA8(cpu);
    And8(cpu, COMPONENT_MASK);
    LoadY16(cpu, WramRead16(wram, SNES_RDMPYL));
    WramWrite(wram, SNES_WRMPYB, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    And16(cpu, MIDDLE_BITS);
    SetBitsDirect16(wram, cpu, DP_COLOR);
    LoadA16(cpu, WramRead16(wram, SNES_RDMPYL));
    And16(cpu, MIDDLE_BITS);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    old = WramRead16(wram, DP_COLOR);
    cpu->carry = (old & 1u) != 0;
    RewriteWord(wram, DP_COLOR, (uint16_t)(old >> 1));
    SetNz16(cpu, (uint16_t)(old >> 1));
    SetBitsDirect16(wram, cpu, DP_COLOR);
}

Lufia2ExecutionResult Lufia2BattleScaleColor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81b3f8u);
    ScaleColor(memory, cpu);
    return ExecutionReturned(0x81b443u);
}

/* $81:B396: fades the 15 palette entries after entry $11 * 16 towards black
 * or white. The sign of $13 selects the direction, its low bits the level. */
Lufia2ExecutionResult Lufia2BattlePaletteBrightness(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81b396u);
    TransferDirectToA(cpu);
    LoadA8(cpu, WramRead(wram, DP_FIRST_ENTRY));
    SetAccumulatorWidth(cpu, 0);
    ExchangeAccumulatorBytes(cpu);
    ShiftA16Right(cpu, 3);
    TransferAToX(cpu);
    LoadA16(cpu, ENTRY_COUNT);
    WramWrite16(wram, DP_REMAINING, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, WramRead(wram, DP_LEVEL));
    if (cpu->negative) {
        cpu->carry = false;
        Adc8(cpu, LEVEL_BASE);
        WramWrite(wram, SNES_WRMPYA, A8(cpu));
        do {
            cpu->x = (uint16_t)(cpu->x + 2u);
            SetNz16(cpu, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, WramRead16At(wram, SOURCE_PALETTE, cpu->x));
            WramWrite16(wram, DP_COLOR, cpu->accumulator);
            SimulateJsrFrame(memory, cpu, 0xb3bdu);
            ScaleColor(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            LoadA16(cpu, WramRead16(wram, DP_COLOR));
            WramWrite16At(wram, PALETTE_BUFFER, cpu->x, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            {
                const uint8_t left = (uint8_t)(WramRead(wram, DP_REMAINING) - 1u);

                WramWrite(wram, DP_REMAINING, left);
                SetNz8(cpu, left);
            }
        } while (!cpu->zero);
    } else {
        LoadA8(cpu, LEVEL_BASE);
        cpu->carry = true;
        Sbc8(cpu, WramRead(wram, DP_LEVEL));
        WramWrite(wram, SNES_WRMPYA, A8(cpu));
        SetAccumulatorWidth(cpu, 0);
        do {
            cpu->x = (uint16_t)(cpu->x + 2u);
            SetNz16(cpu, cpu->x);
            LoadA16(cpu, MAX_COLOR);
            Subtract16(cpu, WramRead16At(wram, SOURCE_PALETTE, cpu->x));
            WramWrite16(wram, DP_COLOR, cpu->accumulator);
            SimulateJsrFrame(memory, cpu, 0xb3e3u);
            ScaleColor(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            LoadA16(cpu, MAX_COLOR);
            Subtract16(cpu, WramRead16(wram, DP_COLOR));
            WramWrite16At(wram, PALETTE_BUFFER, cpu->x, cpu->accumulator);
            SetNz16(cpu, WramStep16(wram, DP_REMAINING, -1));
        } while (!cpu->zero);
        SetAccumulatorWidth(cpu, 1);
    }
    LoadA8(cpu, PALETTE_UPLOAD);
    WramWrite(wram, DP_PALETTE_DIRTY, A8(cpu));
    return ExecutionReturned(0x81b3f7u);
}
