/* Battle circle window: the scanline table of an expanding or shrinking
 * circular window. The half widths of the circle are computed first, then
 * the left and right edge of every scanline is derived from them. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
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
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint8_t across;
    uint8_t down = 0;
    uint16_t error = 0;

    Push8(memory, cpu, PackStatus(cpu));
    WramWrite(wram, DP_ERROR_HIGH, 0);
    across = WramRead(wram, DP_RADIUS);
    do {
        /* One step down: the error falls by twice the row, less one. */
        WramWriteAt(wram, WINDOW_WIDTHS, down, across);
        error = Sum16((uint16_t)~(uint16_t)(down << 1),
            WramRead16(wram, DP_ERROR), false).value;
        down = (uint8_t)(down + 1u);
        WramWrite16(wram, DP_ERROR, error);
        if ((error & 0x8000u) != 0) {
            /* The edge moved in: one step across, the error grows. */
            across = (uint8_t)(across - 1u);
            WramWriteAt(wram, WINDOW_WIDTHS, across, down);
            error = Sum16((uint16_t)(across << 1),
                WramRead16(wram, DP_ERROR), false).value;
            WramWrite16(wram, DP_ERROR, error);
        }
        WramWrite(wram, DP_WIDTHS_END, down);
    } while (across >= WramRead(wram, DP_WIDTHS_END));
    cpu->x = across;
    cpu->y = down;
    cpu->accumulator = error;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x85b2a0u);
}

/* $85:B208: rebuilds the window table when the radius at $1B4A changed: for
 * each row the left edge ($7F less the half width, at least 0) and
 * the right edge ($80 plus it, at most $FF); rows beyond the radius are left
 * open ($FF, 0). M1X0 only (else handed back). Returns before RTS $85B26C.
 * The carry leaves set on every path; the accumulator keeps the high byte
 * of whatever last filled it. */
Lufia2ExecutionResult Lufia2BattleCircleWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint8_t radius;
    uint8_t last_radius;
    uint8_t high_byte;
    uint16_t table = 0;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85b208u);
    PushDataBank(memory, cpu);
    PushStackWord(memory, cpu, cpu->x);
    SelectDataBank(memory, cpu, WINDOW_BANK);
    wram = WramViewInBank(memory, cpu, WINDOW_BANK);
    high_byte = (uint8_t)(cpu->direct_page >> 8);
    radius = WramRead(wram, WINDOW_RADIUS);
    last_radius = WramRead(wram, WINDOW_LAST_RADIUS);
    if (radius != last_radius) {
        const uint16_t extent = (uint16_t)((high_byte << 8) | radius);
        bool overflow = cpu->overflow;

        WramWrite(wram, WINDOW_LAST_RADIUS, radius);
        if (extent != 0) {
            uint16_t row = 0;

            WramWrite16(wram, DP_RADIUS, extent);
            /* The width routine pushes these flags. */
            cpu->carry = radius >= last_radius;
            SetNz16(cpu, extent);
            Push8(memory, cpu, radius);
            SimulateJsrFrame(memory, cpu, WIDTHS_CALL_RETURN);
            (void)Lufia2BattleCircleWidths(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            high_byte = (uint8_t)(cpu->accumulator >> 8);
            WramWrite(wram, DP_RADIUS, Pull8(memory, cpu));
            do {
                const uint8_t half = WramReadAt(wram, WINDOW_WIDTHS, row);
                const Byte8Result left = Difference8(WINDOW_HALF, half);
                const Byte8Result edge = Sum8(WINDOW_RIGHT_BASE, half, false);

                overflow = edge.overflow;
                if (!left.carry)
                    high_byte = (uint8_t)(cpu->direct_page >> 8);
                WramWriteAt(wram, WINDOW_TABLE, table,
                    left.carry ? left.value : (uint8_t)cpu->direct_page);
                table = (uint16_t)(table + 1u);
                WramWriteAt(wram, WINDOW_TABLE, table,
                    edge.carry ? WINDOW_OPEN : edge.value);
                table = (uint16_t)(table + 1u);
                row = (uint16_t)(row + 1u);
            } while (WramStep8(wram, DP_RADIUS, -1) != 0);
            cpu->y = row;
        }
        /* The rest of the table is open: $FF, then 0, for every row. */
        while (table != WINDOW_TABLE_END) {
            WramWriteAt(wram, WINDOW_TABLE, table, WINDOW_OPEN);
            table = (uint16_t)(table + 1u);
            WramWriteAt(wram, WINDOW_TABLE, table, 0);
            table = (uint16_t)(table + 1u);
        }
        cpu->overflow = overflow;
    }
    WramWrite(wram, WINDOW_READY, 1u);
    WramWrite(wram, WINDOW_ENABLE_BANK, WINDOW_BANK);
    cpu->x = PullStackWord(memory, cpu);
    cpu->carry = true;
    cpu->accumulator = (uint16_t)((high_byte << 8) | WINDOW_BANK);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85b26cu);
}
