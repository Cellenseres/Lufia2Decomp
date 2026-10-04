/* Battle tile copy: rows of 64 bytes, each with a second plane $40 bytes
 * further on, copied into the two buffers $200 bytes apart. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

enum {
    ROW_POSITION = 0x00u,
    ROW_COUNT = 0x01u,
    ROW_FACTORS = 0x02u,
    SOURCE = 0x08u,
    TARGET = 0x0bu,
    ROWS_LEFT = 0x12u,
    TARGET_OFFSET = 0x13u,
    ROWS_TO_BLOCK_END = 0x15u,
    BYTES_LEFT = 0x17u,
    HARDWARE_A = 0x4202u,
    HARDWARE_PRODUCT = 0x4216u,
    ROW_LENGTH = 0x40u,
    BLOCK_ROWS = 8u,
    TILE_PLANE = 0x7e0000u,             /* source rows */
    TILE_SECOND_PLANE = 0x7e0040u,      /* $40 bytes on in the source */
    FIRST_BUFFER = 0x7e0000u,
    SECOND_BUFFER = 0x7e0200u,
    PLANE_STEP = 0x0040u,
    BLOCK_STEP = 0x0200u,
    WORK_BANK = 0x7eu,
    WORK_END = 0x2000u,
    LAST_RAM_BYTE = 0xffffu,
    STACK_MIN = 0x1f00u,
    STACK_MAX = 0x1ffcu
};

/* These banks expose the multiplier rather than RAM at $4202. */
static bool BlitMultiplierBank(uint8_t bank) {
    return bank < 0x40u || (bank >= 0x80u && bank < 0xc0u);
}

/* Keep the forward copy clear of its work bytes and caller frame. */
static bool BlitOutputsAvoidWorkArea(Lufia2Wram wram) {
    const uint16_t position = WramRead16(wram, ROW_POSITION);
    const uint16_t factors = WramRead16(wram, ROW_FACTORS);
    const uint16_t target_base = WramRead16(wram, TARGET);
    const uint8_t product = (uint8_t)((factors & 0xffu) * (factors >> 8));
    const unsigned rows = product == 0u ? 256u : product;
    const uint16_t offset = (uint16_t)(((position + (position & 0x00f8u)) &
        0x00ffu) << 6);
    const uint16_t first = (uint16_t)(target_base + offset);
    const unsigned block_steps = (rows - 1u + (position & 7u)) / BLOCK_ROWS;
    const uint32_t last = first + (rows - 1u) * ROW_LENGTH +
        block_steps * BLOCK_STEP + ROW_LENGTH - 1u;

    return first >= WORK_END && last <= LAST_RAM_BYTE;
}

/* $81:BCCC: copies rows from the source word at $08 into the buffers at
 * $7E:X and $7E:X+$200. The first byte of the product of the two bytes at
 * $02/$03 gives the number of rows, and the position in $00 selects the
 * start inside the 8-row block of the target. M8/X16, JSL. */
Lufia2ExecutionResult Lufia2BattleBlitTileRows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t position;
    uint16_t row_sum;
    uint16_t target_offset;
    uint16_t source;
    uint16_t target;
    uint8_t rows;
    uint8_t plane_byte;
    Word16Result last_sum;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal ||
        cpu->direct_page != 0u || cpu->stack < STACK_MIN || cpu->stack > STACK_MAX ||
        !BlitMultiplierBank(cpu->data_bank) || !BlitOutputsAvoidWorkArea(wram))
        return ExecutionHandoff(cpu, 0x81bcccu);
    WramWrite16(wram, HARDWARE_A, WramRead16(wram, ROW_FACTORS));
    position = WramRead16(wram, ROW_POSITION);
    WramWrite16(wram, ROWS_TO_BLOCK_END, (uint16_t)(position & 0x0007u));
    row_sum = Sum16Mode(position & 0x00f8u, position, false,
        cpu->decimal).value & 0x00ffu;
    target_offset = (uint16_t)(row_sum << 6);
    PushStackWord(memory, cpu, target_offset);
    WramWrite16(wram, TARGET_OFFSET, target_offset);
    rows = WramRead(wram, HARDWARE_PRODUCT);
    WramWrite(wram, ROW_COUNT, rows);
    WramWrite(wram, ROWS_LEFT, rows);
    WramWrite(wram, ROWS_TO_BLOCK_END,
        Difference8Mode(BLOCK_ROWS, WramRead(wram, ROWS_TO_BLOCK_END), cpu->decimal).value);
    target_offset = PullStackWord(memory, cpu);
    last_sum = Sum16Mode(target_offset, WramRead16(wram, TARGET), false, cpu->decimal);
    target = last_sum.value;
    source = WramRead16(wram, SOURCE);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, WORK_BANK);
    for (;;) {
        WramWrite(wram, BYTES_LEFT, ROW_LENGTH);
        do {
            WramWriteAt(wram, FIRST_BUFFER, target,
                WramReadAt(wram, TILE_PLANE, source));
            plane_byte = WramReadAt(wram, TILE_SECOND_PLANE, source);
            WramWriteAt(wram, SECOND_BUFFER, target, plane_byte);
            target = (uint16_t)(target + 1u);
            source = (uint16_t)(source + 1u);
        } while (WramStep8(wram, BYTES_LEFT, -1) != 0);
        (void)WramStep8(wram, ROW_POSITION, 1);
        if (WramStep8(wram, ROWS_LEFT, -1) == 0)
            break;
        last_sum = Sum16Mode(source, PLANE_STEP, false, cpu->decimal);
        source = last_sum.value;
        if (WramStep8(wram, ROWS_TO_BLOCK_END, -1) != 0)
            continue;
        WramWrite(wram, ROWS_TO_BLOCK_END, BLOCK_ROWS);
        last_sum = Sum16Mode(target, BLOCK_STEP, false, cpu->decimal);
        target = last_sum.value;
    }
    cpu->x = target;
    cpu->y = source;
    cpu->accumulator = (uint16_t)((last_sum.value & 0xff00u) | plane_byte);
    SetSumFlags(cpu, last_sum);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81bd46u);
}
