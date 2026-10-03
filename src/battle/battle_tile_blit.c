/* Battle tile copy: rows of 64 bytes, each with a second plane $40 bytes
 * further on, copied into the two buffers $200 bytes apart. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"

enum {
    ROW_COUNT_LOW = 0x00u,
    ROW_COUNT_HIGH = 0x01u,
    SCALE_X = 0x02u,
    SOURCE = 0x08u,
    TARGET = 0x0bu,
    ROWS_LEFT = 0x12u,
    TARGET_OFFSET = 0x13u,
    COLUMN_SKIP = 0x15u,
    ROW_BYTES = 0x17u,
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
    WORK_BANK = 0x7eu
};

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

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        !DirectWorkByteAvailable(cpu, ROW_BYTES) ||
        !DirectWorkByteAvailable(cpu, ROWS_LEFT))
        return ExecutionHandoff(cpu, 0x81bcccu);
    WramWrite16(wram, HARDWARE_A, WramRead16(wram, SCALE_X));
    position = WramRead16(wram, ROW_COUNT_LOW);
    WramWrite16(wram, COLUMN_SKIP, (uint16_t)(position & 0x0007u));
    row_sum = Sum16Mode(position & 0x00f8u, position, false,
        cpu->decimal).value & 0x00ffu;
    target_offset = (uint16_t)(row_sum << 6);
    PushStackWord(memory, cpu, target_offset);
    WramWrite16(wram, TARGET_OFFSET, target_offset);
    rows = WramRead(wram, HARDWARE_PRODUCT);
    WramWrite(wram, ROW_COUNT_HIGH, rows);
    WramWrite(wram, ROWS_LEFT, rows);
    WramWrite(wram, COLUMN_SKIP,
        Difference8Mode(BLOCK_ROWS, WramRead(wram, COLUMN_SKIP), cpu->decimal).value);
    target_offset = PullStackWord(memory, cpu);
    last_sum = Sum16Mode(target_offset, WramRead16(wram, TARGET), false, cpu->decimal);
    target = last_sum.value;
    source = WramRead16(wram, SOURCE);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, WORK_BANK);
    for (;;) {
        WramWrite(wram, ROW_BYTES, ROW_LENGTH);
        do {
            WramWriteAt(wram, FIRST_BUFFER, target,
                WramReadAt(wram, TILE_PLANE, source));
            plane_byte = WramReadAt(wram, TILE_SECOND_PLANE, source);
            WramWriteAt(wram, SECOND_BUFFER, target, plane_byte);
            target = (uint16_t)(target + 1u);
            source = (uint16_t)(source + 1u);
        } while (WramStep8(wram, ROW_BYTES, -1) != 0);
        (void)WramStep8(wram, ROW_COUNT_LOW, 1);
        if (WramStep8(wram, ROWS_LEFT, -1) == 0)
            break;
        last_sum = Sum16Mode(source, PLANE_STEP, false, cpu->decimal);
        source = last_sum.value;
        if (WramStep8(wram, COLUMN_SKIP, -1) != 0)
            continue;
        WramWrite(wram, COLUMN_SKIP, BLOCK_ROWS);
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
