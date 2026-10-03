/* Battle circle window: the scanline table of an expanding or shrinking
 * circular window. The half widths of the circle are computed first, then
 * the left and right edge of every scanline is derived from them. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

/* Direct page: the radius, which is then the running error of the table
 * (a word), and the table length. */
enum {
    DP_RADIUS = 0xc6u,
    DP_ERROR = 0xc6u,
    DP_ERROR_HIGH = 0xc7u,
    DP_WIDTHS_END = 0xc8u
};

/* Absolute work RAM of the routine's own data bank ($7E). */
enum {
    WINDOW_BANK = 0x7eu,
    WINDOW_RADIUS = 0x1b4au,
    WINDOW_LAST_RADIUS = 0x1b4bu,
    WINDOW_READY = 0x1b48u,
    WINDOW_WIDTHS = 0x4600u,
    WINDOW_TABLE = 0x4200u,
    WINDOW_TABLE_END = 0x0200u,
    WINDOW_ENABLE_BANK = 0x4367u,
    WINDOW_HALF = 0x7fu,
    WINDOW_RIGHT_BASE = 0x80u,
    WINDOW_OPEN = 0xffu,
    WIDTHS_CALL_RETURN = 0xb222u
};

/* $85:B26D: the half widths of the circle of the radius at $C6 by the
 * midpoint rule, one byte per row from $4600. Any entry widths; they are
 * restored. */
Lufia2ExecutionResult Lufia2BattleCircleWidths(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    WramWrite(wram, DP_ERROR_HIGH, 0);
    cpu->x = WramRead(wram, DP_RADIUS);
    SetNz8(cpu, (uint8_t)cpu->x);
    cpu->y = 0;
    SetNz8(cpu, 0);
    do {
        TransferXToA(cpu);
        SetAccumulatorWidth(cpu, 1);
        WramWriteAt(wram, WINDOW_WIDTHS, cpu->y, A8(cpu));
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, cpu->y);
        cpu->y = (uint8_t)(cpu->y + 1u);
        SetNz8(cpu, (uint8_t)cpu->y);
        AslA16(cpu);
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        Add16Value(cpu, WramRead16(wram, DP_ERROR));
        WramWrite16(wram, DP_ERROR, cpu->accumulator);
        if (cpu->negative) {
            LoadA16(cpu, cpu->y);
            cpu->x = (uint8_t)(cpu->x - 1u);
            SetNz8(cpu, (uint8_t)cpu->x);
            SetAccumulatorWidth(cpu, 1);
            WramWriteAt(wram, WINDOW_WIDTHS, cpu->x, A8(cpu));
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            AslA16(cpu);
            Add16Value(cpu, WramRead16(wram, DP_ERROR));
            WramWrite16(wram, DP_ERROR, cpu->accumulator);
        }
        WramWrite(wram, DP_WIDTHS_END, (uint8_t)cpu->y);
        Compare8(cpu, (uint8_t)cpu->x, WramRead(wram, DP_WIDTHS_END));
    } while (cpu->carry);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x85b2a0u);
}

/* $85:B208: rebuilds the window table when the radius at $1B4A changed: for
 * each row the left edge ($7F less the half width, at least 0) and
 * the right edge ($80 plus it, at most $FF); rows beyond the radius are left
 * open ($FF, 0). M1X0 only (else handed back). Returns before RTS $85B26C. */
Lufia2ExecutionResult Lufia2BattleCircleWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    bool fill_first;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85b208u);
    PushDataBank(memory, cpu);
    PushIndex(memory, cpu);
    LoadA8(cpu, WINDOW_BANK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    wram = WramViewInBank(memory, cpu, WINDOW_BANK);
    TransferDirectToA(cpu);
    LoadA8(cpu, WramRead(wram, WINDOW_RADIUS));
    Compare8(cpu, A8(cpu), WramRead(wram, WINDOW_LAST_RADIUS));
    if (!cpu->zero) {
        WramWrite(wram, WINDOW_LAST_RADIUS, A8(cpu));
        TransferAToX(cpu);
        fill_first = cpu->zero;
        if (!fill_first) {
            WramWrite16(wram, DP_RADIUS, cpu->x);
            PushAccumulator8(memory, cpu);
            SimulateJsrFrame(memory, cpu, WIDTHS_CALL_RETURN);
            (void)Lufia2BattleCircleWidths(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            LoadA8(cpu, Pull8(memory, cpu));
            WramWrite(wram, DP_RADIUS, A8(cpu));
            LoadX16(cpu, 0);
            cpu->y = cpu->x;
            SetNz16(cpu, cpu->y);
            do {
                uint8_t rows;

                LoadA8(cpu, WINDOW_HALF);
                cpu->carry = true;
                Sbc8(cpu, WramReadAt(wram, WINDOW_WIDTHS, cpu->y));
                if (!cpu->carry)
                    TransferDirectToA(cpu);
                WramWriteAt(wram, WINDOW_TABLE, cpu->x, A8(cpu));
                LoadX16(cpu, (uint16_t)(cpu->x + 1u));
                LoadA8(cpu, WINDOW_RIGHT_BASE);
                cpu->carry = false;
                Adc8(cpu, WramReadAt(wram, WINDOW_WIDTHS, cpu->y));
                if (cpu->carry)
                    LoadA8(cpu, WINDOW_OPEN);
                WramWriteAt(wram, WINDOW_TABLE, cpu->x, A8(cpu));
                LoadX16(cpu, (uint16_t)(cpu->x + 1u));
                LoadY16(cpu, (uint16_t)(cpu->y + 1u));
                rows = (uint8_t)(WramRead(wram, DP_RADIUS) - 1u);
                WramWrite(wram, DP_RADIUS, rows);
                SetNz8(cpu, rows);
            } while (!cpu->zero);
            LoadA8(cpu, WINDOW_OPEN);
            Compare16(cpu, cpu->x, WINDOW_TABLE_END);
        } else {
            LoadX16(cpu, 0);
            LoadA8(cpu, WINDOW_OPEN);
        }
        /* The rest of the table is open: $FF, then 0, for every row. */
        while (fill_first || !cpu->zero) {
            fill_first = false;
            WramWriteAt(wram, WINDOW_TABLE, cpu->x, A8(cpu));
            LoadX16(cpu, (uint16_t)(cpu->x + 1u));
            WramWriteAt(wram, WINDOW_TABLE, cpu->x, 0);
            LoadX16(cpu, (uint16_t)(cpu->x + 1u));
            Compare16(cpu, cpu->x, WINDOW_TABLE_END);
        }
    }
    LoadA8(cpu, 1u);
    WramWrite(wram, WINDOW_READY, A8(cpu));
    LoadA8(cpu, WINDOW_BANK);
    WramWrite(wram, WINDOW_ENABLE_BANK, A8(cpu));
    cpu->x = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85b26cu);
}
