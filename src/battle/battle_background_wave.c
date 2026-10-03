/* Battle background wave: the table of horizontal scroll offsets that the
 * wave effect streams to the scroll registers, rebuilt every frame from a
 * ripple pattern in ROM and the current base offset. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

/* Absolute work RAM, read through the routine's own data bank ($85). */
enum {
    WAVE_BANK = 0x85u,
    WAVE_BASE = 0x059eu,
    WAVE_PHASE = 0x1b22u,
    WAVE_TABLE_READY = 0x1b20u,
    WAVE_PATTERN = 0xa29cu,
    WAVE_PATTERN_END = 0x0040u,
    WAVE_LAST_PHASE = 0x003eu,
    WAVE_TABLE = 0x7e4000u,
    WAVE_TABLE_END = 0x0160u,
    WAVE_TABLE_READY_FLAG = 1u
};

/* Fills the 176-word table: each word is the base offset plus the ripple
 * pattern word at the phase, which advances by one word per entry and wraps at
 * the end of the pattern. Leaves Y at the phase after the last entry. */
static void FillWaveTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    cpu->x = 0;
    SetAccumulatorWidth(cpu, 0);
    do {
        const uint32_t pattern = ((uint32_t)WAVE_BANK << 16) + WAVE_PATTERN + cpu->y;
        const uint32_t entry = WAVE_TABLE + cpu->x;

        LoadA16(cpu, (uint16_t)(Read8(memory, pattern & 0x00ffffffu) |
            ((uint16_t)Read8(memory, (pattern + 1u) & 0x00ffffffu) << 8)));
        cpu->carry = false;
        Add16Value(cpu, WramRead16(wram, WAVE_BASE));
        Write8(memory, entry, (uint8_t)cpu->accumulator);
        Write8(memory, entry + 1u, (uint8_t)(cpu->accumulator >> 8));
        LoadY16(cpu, (uint16_t)(cpu->y + 2u));
        LoadX16(cpu, (uint16_t)(cpu->x + 2u));
        Compare16(cpu, cpu->y, WAVE_PATTERN_END);
        if (cpu->zero)
            LoadY16(cpu, 0);
        Compare16(cpu, cpu->x, WAVE_TABLE_END);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
}

/* Enters the routine's data bank the way PHB, LDA #$85, PHA, PLB does. */
static Lufia2Wram EnterWaveBank(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    LoadA8(cpu, WAVE_BANK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    return WramViewInBank(memory, cpu, WAVE_BANK);
}

static void LeaveWaveBank(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2Wram wram) {
    LoadA8(cpu, WAVE_TABLE_READY_FLAG);
    WramWrite(wram, WAVE_TABLE_READY, A8(cpu));
    PullDataBank(memory, cpu);
}

/* Stores Y as the next phase (a 16-bit store through A). */
static void StorePhase(Lufia2Wram wram, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    WramWrite16(wram, WAVE_PHASE, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $85:AEEB: the table from phase 0, without touching the stored phase.
 * M1X0 only (else handed back). Returns before RTS $85AF1C. */
Lufia2ExecutionResult Lufia2BattleWaveFill(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85aeebu);
    wram = EnterWaveBank(memory, cpu);
    PushIndex(memory, cpu);
    cpu->x = 0;
    cpu->y = cpu->x;
    SetNz16(cpu, cpu->y);
    FillWaveTable(memory, cpu, wram);
    cpu->x = PullIndexValue(memory, cpu);
    LeaveWaveBank(memory, cpu, wram);
    return ExecutionReturned(0x85af1cu);
}

/* $85:AE68: the table from the stored phase, then the phase advances one
 * word. M1X0 only (else handed back). Returns before RTS $85AEB0. */
Lufia2ExecutionResult Lufia2BattleWaveForward(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85ae68u);
    wram = EnterWaveBank(memory, cpu);
    LoadY16(cpu, WramRead16(wram, WAVE_PHASE));
    PushIndex(memory, cpu);
    FillWaveTable(memory, cpu, wram);
    cpu->x = PullIndexValue(memory, cpu);
    LoadY16(cpu, WramRead16(wram, WAVE_PHASE));
    LoadY16(cpu, (uint16_t)(cpu->y + 2u));
    Compare16(cpu, cpu->y, WAVE_PATTERN_END);
    if (cpu->zero)
        LoadY16(cpu, 0);
    StorePhase(wram, cpu);
    LeaveWaveBank(memory, cpu, wram);
    return ExecutionReturned(0x85aeb0u);
}

/* $85:ADE1: the table from the stored phase, then the phase steps back one
 * word, wrapping to the last. M1X0 only (else handed back). Returns before
 * RTS $85AE26. */
Lufia2ExecutionResult Lufia2BattleWaveBackward(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85ade1u);
    wram = EnterWaveBank(memory, cpu);
    LoadY16(cpu, WramRead16(wram, WAVE_PHASE));
    PushIndex(memory, cpu);
    FillWaveTable(memory, cpu, wram);
    cpu->x = PullIndexValue(memory, cpu);
    LoadY16(cpu, WramRead16(wram, WAVE_PHASE));
    LoadY16(cpu, (uint16_t)(cpu->y - 2u));
    if (cpu->negative)
        LoadY16(cpu, WAVE_LAST_PHASE);
    StorePhase(wram, cpu);
    LeaveWaveBank(memory, cpu, wram);
    return ExecutionReturned(0x85ae26u);
}
