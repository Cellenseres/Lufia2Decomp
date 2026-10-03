/* Battle background wave: the table of horizontal scroll offsets that the
 * wave effect streams to the scroll registers, rebuilt every frame from a
 * ripple pattern in ROM and the current base offset. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

/* Absolute work RAM, read through the routine's own data bank ($85). */
enum {
    WAVE_BANK = 0x85u,
    WAVE_BASE = 0x059eu,
    WAVE_PHASE = 0x1b22u,
    WAVE_TABLE_READY = 0x1b20u,
    WAVE_PATTERN = 0x85a29cu,
    WAVE_PATTERN_END = 0x0040u,
    WAVE_LAST_PHASE = 0x003eu,
    WAVE_TABLE = 0x7e4000u,
    WAVE_TABLE_END = 0x0160u,
    WAVE_TABLE_READY_FLAG = 1u
};

/* The table is complete and the last entry added; the phase after it. */
typedef struct {
    uint16_t phase;
    Word16Result last;
} WaveFill;

/* Fills the 176-word table: each word is the base offset plus the ripple
 * pattern word at the phase, which advances by one word per entry and wraps at
 * the end of the pattern. */
static WaveFill FillWaveTable(
    const Lufia2Memory *memory, Lufia2Wram wram, uint16_t phase) {
    WaveFill fill;
    uint16_t offset = 0;

    fill.phase = phase;
    do {
        fill.last = Sum16(
            Read16Long(memory, LongIndexedAddress(WAVE_PATTERN, fill.phase)),
            WramRead16(wram, WAVE_BASE), false);
        WramWrite16At(wram, WAVE_TABLE, offset, fill.last.value);
        fill.phase = (uint16_t)(fill.phase + 2u);
        if (fill.phase == WAVE_PATTERN_END)
            fill.phase = 0;
        offset = (uint16_t)(offset + 2u);
    } while (offset != WAVE_TABLE_END);
    return fill;
}

/* Enters the routine's data bank the way PHB, LDA #$85, PHA, PLB does. */
static Lufia2Wram EnterWaveBank(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    SelectDataBank(memory, cpu, WAVE_BANK);
    return WramViewInBank(memory, cpu, WAVE_BANK);
}

/* Marks the table ready and leaves the data bank. The accumulator keeps the
 * high byte of `accumulator`. */
static void LeaveWaveBank(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2Wram wram, uint16_t accumulator) {
    cpu->accumulator = accumulator;
    LoadA8(cpu, WAVE_TABLE_READY_FLAG);
    WramWrite(wram, WAVE_TABLE_READY, WAVE_TABLE_READY_FLAG);
    PullDataBank(memory, cpu);
}

/* $85:AEEB: the table from phase 0, without touching the stored phase.
 * M1X0 only (else handed back). Returns before RTS $85AF1C. */
Lufia2ExecutionResult Lufia2BattleWaveFill(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    WaveFill fill;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85aeebu);
    wram = EnterWaveBank(memory, cpu);
    PushStackWord(memory, cpu, cpu->x);
    fill = FillWaveTable(memory, wram, 0);
    cpu->x = PullStackWord(memory, cpu);
    cpu->y = fill.phase;
    cpu->carry = true;
    cpu->overflow = fill.last.overflow;
    LeaveWaveBank(memory, cpu, wram, fill.last.value);
    return ExecutionReturned(0x85af1cu);
}

/* $85:AE68: the table from the stored phase, then the phase advances one
 * word. M1X0 only (else handed back). Returns before RTS $85AEB0. */
Lufia2ExecutionResult Lufia2BattleWaveForward(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    WaveFill fill;
    uint16_t next;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85ae68u);
    wram = EnterWaveBank(memory, cpu);
    PushStackWord(memory, cpu, cpu->x);
    fill = FillWaveTable(memory, wram, WramRead16(wram, WAVE_PHASE));
    cpu->x = PullStackWord(memory, cpu);
    next = (uint16_t)(WramRead16(wram, WAVE_PHASE) + 2u);
    cpu->carry = next >= WAVE_PATTERN_END;
    cpu->overflow = fill.last.overflow;
    if (next == WAVE_PATTERN_END)
        next = 0;
    cpu->y = next;
    WramWrite16(wram, WAVE_PHASE, next);
    LeaveWaveBank(memory, cpu, wram, next);
    return ExecutionReturned(0x85aeb0u);
}

/* $85:ADE1: the table from the stored phase, then the phase steps back one
 * word, wrapping to the last. M1X0 only (else handed back). Returns before
 * RTS $85AE26. */
Lufia2ExecutionResult Lufia2BattleWaveBackward(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    WaveFill fill;
    uint16_t previous;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85ade1u);
    wram = EnterWaveBank(memory, cpu);
    PushStackWord(memory, cpu, cpu->x);
    fill = FillWaveTable(memory, wram, WramRead16(wram, WAVE_PHASE));
    cpu->x = PullStackWord(memory, cpu);
    previous = (uint16_t)(WramRead16(wram, WAVE_PHASE) - 2u);
    if ((previous & 0x8000u) != 0)
        previous = WAVE_LAST_PHASE;
    cpu->y = previous;
    cpu->carry = true;
    cpu->overflow = fill.last.overflow;
    WramWrite16(wram, WAVE_PHASE, previous);
    LeaveWaveBank(memory, cpu, wram, previous);
    return ExecutionReturned(0x85ae26u);
}

/* Ripple row: scroll bytes and tables. The direct page byte $33 is the
 * counter of both ripple routines. */
enum {
    SCRATCH_COUNTER = 0x33u,
    RIPPLE_FINE_SCROLL = 0x0596u,
    RIPPLE_STEP = 0x1b1au,
    RIPPLE_ROW_DELAY = 0x1b18u,
    RIPPLE_SCROLL = 0x0594u,
    RIPPLE_PATTERN = 0x9f42u,
    RIPPLE_ROW_TABLE = 0x7e4400u,
    RIPPLE_ROWS = 0x20u,
    RIPPLE_ROW_DELAY_VALUE = 0x04u,
    RIPPLE_PHASE_MASK = 0x1fu
};

/* $85:A736: 32 entries of the ripple table at $7E:4400, taken from the
 * ROM table at $85:9F42 and offset by the scroll at $0594. M8/X16, JSR. */
Lufia2ExecutionResult Lufia2BattleRippleRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    Byte8Result start;
    Byte8Result sum;
    uint16_t row = 0;
    uint16_t pattern;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85a736u);
    wram = EnterWaveBank(memory, cpu);
    start = Sum8((uint8_t)(WramRead(wram, RIPPLE_FINE_SCROLL) << 1),
        WramRead(wram, RIPPLE_STEP), false);
    /* The transfer to Y takes the whole accumulator, which still holds the
     * direct page in its high byte. */
    pattern = (uint16_t)((cpu->direct_page & 0xff00u) |
        (start.value & RIPPLE_PHASE_MASK));
    PushStackWord(memory, cpu, cpu->x);
    WramWrite(wram, SCRATCH_COUNTER, RIPPLE_ROWS);
    do {
        sum = Sum8(WramReadAt(wram, RIPPLE_PATTERN, pattern),
            WramRead(wram, RIPPLE_SCROLL), false);
        WramWriteAt(wram, RIPPLE_ROW_TABLE, row, sum.value);
        pattern = (uint16_t)(pattern + 1u);
        row = (uint16_t)(row + 1u);
    } while (WramStep8(wram, SCRATCH_COUNTER, -1) != 0);
    cpu->x = PullStackWord(memory, cpu);
    (void)WramStep8(wram, RIPPLE_STEP, 1);
    (void)WramStep8(wram, RIPPLE_STEP, 1);
    WramWrite(wram, RIPPLE_ROW_DELAY, RIPPLE_ROW_DELAY_VALUE);
    cpu->y = pattern;
    cpu->carry = sum.carry;
    cpu->overflow = sum.overflow;
    cpu->accumulator = (uint16_t)(cpu->direct_page & 0xff00u);
    LoadA8(cpu, RIPPLE_ROW_DELAY_VALUE);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85a76du);
}

/* Ripple words: the table written, its source in ROM and its geometry. */
enum {
    WORDS_PHASE = 0x1b22u,
    WORDS_READY = 0x1b20u,
    WORDS_SOURCE = 0x85a04bu,
    WORDS_TABLE = 0x7e40deu,
    WORDS_END = 0xa8u,
    WORDS_RUN = 0x0cu,
    WORDS_RUN_STEP = 0x0004u,
    WORDS_BASE_OFFSET = 0x08u
};

/* $85:AA3D: the 84-word table at $7E:40DE, taken from $85:A04B by a base
 * entry chosen by $1B22; a word repeats while Y counts down. M8/X16, JSR. */
Lufia2ExecutionResult Lufia2BattleRippleWords(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Byte8Result base;
    uint8_t phase;
    uint8_t run_left;
    uint8_t source;
    uint8_t index = 0;
    uint16_t word;
    bool overflow;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85aa3du);
    phase = WramRead(wram, WORDS_PHASE);
    PushStackWord(memory, cpu, cpu->x);
    /* From here to the end the index registers are eight bits wide. */
    WramWrite(wram, SCRATCH_COUNTER, phase & 3u);
    run_left = (uint8_t)(4u - (phase & 3u));
    WramWrite(wram, SCRATCH_COUNTER, run_left);
    run_left = (uint8_t)(3u * run_left);
    base = Sum8((uint8_t)(phase << 1), WORDS_BASE_OFFSET, false);
    source = base.value;
    word = Read16Long(memory, LongIndexedAddress(WORDS_SOURCE, source));
    overflow = base.overflow;
    do {
        WramWrite16At(wram, WORDS_TABLE, index, word);
        run_left = (uint8_t)(run_left - 1u);
        if (run_left == 0) {
            const Word16Result run = Sum16(word, WORDS_RUN_STEP, false);

            run_left = WORDS_RUN;
            word = run.value;
            overflow = run.overflow;
        }
        index = (uint8_t)(index + 2u);
    } while (index != WORDS_END);
    cpu->x = PullStackWord(memory, cpu);
    cpu->y = run_left;
    cpu->carry = true;
    cpu->overflow = overflow;
    cpu->accumulator = word;
    LoadA8(cpu, 1u);
    WramWrite(wram, WORDS_READY, 1u);
    return ExecutionReturned(0x85aa7eu);
}
