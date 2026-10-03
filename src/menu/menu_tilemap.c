/* Menu tile map buffers: clearing both layers, filling a block of rising tile
 * numbers, and recoloring a rectangle. Each of the three that wait for a
 * redraw stops at the frame wait of $82:93C2. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"

enum {
    LAYER_FRONT = 0x7e2000u,
    LAYER_BACK = 0x7e3000u,
    TILEMAP = 0x7e2800u,
    LAYER_END = 0x0800u,
    ROW_STRIDE = 0x0040u,
    BLOCK_SIZE = 4u,
    GRID_SIZE = 8u,
    GRID_BASE_TILE = 0x0c10u,
    GRID_TILE_OFFSET = 0x155eu,
    GRID_ROW_SKIP = 0x00c0u,
    GRID_BLOCK_STEP = 0x0008u,
    COLOR_KEEP = 0xe3ffu,
    REDRAW_REQUEST = 0x20u,
    CLEAR_REDRAW = 0x88u,
    CLEAR_BANK = 0x7eu,
    WORK_POSITION = 0x54u,
    WORK_WIDTH = 0x56u,
    WORK_HEIGHT = 0x58u,
    WORK_TILE = 0x5au,
    WORK_COLUMNS = 0x63u,
    WORK_ROWS = 0x65u,
    REDRAW_FLAGS = 0x74u,
    REDRAW_WAIT = 0x8293c2u
};

/* Stops at the frame wait with the routine's own return pushed. */
static Lufia2ExecutionResult WaitForRedraw(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    return ExecutionHandoff(cpu, REDRAW_WAIT);
}

/* $82:80A5: the 4 x 4 block at position $54 receives the tile number $5A
 * counting up along each row and carrying on into the next. M0X0. Leaves the
 * tile in A and Y, the start of the row after the block in X. */
static void FillTileBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t position;
    uint16_t tile;
    uint16_t rows_left;
    Word16Result next_row;

    WramWrite16(wram, WORK_WIDTH, BLOCK_SIZE);
    WramWrite16(wram, WORK_HEIGHT, BLOCK_SIZE);
    position = WramRead16(wram, WORK_POSITION);
    tile = WramRead16(wram, WORK_TILE);
    do {
        uint16_t columns = WramRead16(wram, WORK_WIDTH);
        uint16_t cell = position;

        PushStackWord(memory, cpu, position);
        do {
            WramWrite16At(wram, TILEMAP, cell, tile);
            cell = (uint16_t)(cell + 2u);
            tile = (uint16_t)(tile + 1u);
            columns = (uint16_t)(columns - 1u);
        } while (columns != 0);
        next_row = Sum16Mode(PullStackWord(memory, cpu), ROW_STRIDE, false, cpu->decimal);
        position = next_row.value;
        rows_left = WramStep16(wram, WORK_HEIGHT, -1);
    } while (rows_left != 0);
    cpu->x = position;
    cpu->y = tile;
    cpu->accumulator = tile;
    SetSumFlags(cpu, next_row);
    SetNz16(cpu, rows_left);
}

Lufia2ExecutionResult Lufia2MenuTileBlockFill(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8280a5u);
    FillTileBlock(memory, cpu);
    return ExecutionReturned(0x8280c9u);
}

/* $82:8069: the 8 x 8 blocks of the picture grid, each numbered from the base
 * tile plus the tile offset, then the redraw wait; any M, X16. */
Lufia2ExecutionResult Lufia2MenuTileGridFill(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Word16Result tile;
    Word16Result position;

    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x828069u);
    tile = Sum16Mode(GRID_BASE_TILE, WramRead16(wram, GRID_TILE_OFFSET), false, cpu->decimal);
    WramWrite16(wram, WORK_TILE, tile.value);
    WramWrite16(wram, WORK_POSITION, 0);
    WramWrite16(wram, WORK_ROWS, GRID_SIZE);
    do {
        WramWrite16(wram, WORK_COLUMNS, GRID_SIZE);
        do {
            SimulateJsrFrame(memory, cpu, 0x8082u);
            FillTileBlock(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            position = Sum16Mode(
                WramRead16(wram, WORK_POSITION), GRID_BLOCK_STEP, false, cpu->decimal);
            WramWrite16(wram, WORK_POSITION, position.value);
        } while (WramStep16(wram, WORK_COLUMNS, -1) != 0);
        position = Sum16Mode(WramRead16(wram, WORK_POSITION), GRID_ROW_SKIP, false, cpu->decimal);
        WramWrite16(wram, WORK_POSITION, position.value);
    } while (WramStep16(wram, WORK_ROWS, -1) != 0);
    SetAccumulatorWidth(cpu, 1);
    cpu->accumulator = position.value;
    SetSumFlags(cpu, position);
    LoadA8(cpu, REDRAW_REQUEST);
    WramWrite(wram, REDRAW_FLAGS, REDRAW_REQUEST);
    return WaitForRedraw(memory, cpu, 0x80a3u);
}

/* $82:80CA: the rectangle at A, X = width << 8 | rows, takes the palette
 * number in Y; then the redraw wait. M0X0. */
Lufia2ExecutionResult Lufia2MenuRecolorRect(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t cell;
    uint16_t attributes;
    uint16_t position;
    Word16Result next_row;
    uint8_t redraw;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8280cau);
    position = cpu->accumulator;
    WramWrite16(wram, WORK_POSITION, position);
    WramWrite16(wram, WORK_WIDTH, 0);
    WramWrite16(wram, WORK_HEIGHT, 0);
    /* The palette number moves from the low byte to the high byte of the
     * attribute word, two places up. */
    attributes = (uint16_t)((((cpu->y & 0x00ffu) << 8) | (cpu->y >> 8)) << 2);
    WramWrite16(wram, WORK_TILE, attributes);
    WramWrite(wram, WORK_HEIGHT, (uint8_t)cpu->x);
    WramWrite(wram, WORK_WIDTH, (uint8_t)(cpu->x >> 8));
    do {
        uint16_t columns = WramRead16(wram, WORK_WIDTH);

        position = WramRead16(wram, WORK_POSITION);
        cell = position;
        do {
            const uint16_t old = Read16Long(memory,
                LongIndexedAddress(TILEMAP, cell));

            WramWrite16At(wram, TILEMAP, cell,
                (uint16_t)((old & COLOR_KEEP) | WramRead16(wram, WORK_TILE)));
            cell = (uint16_t)(cell + 2u);
            columns = (uint16_t)(columns - 1u);
        } while (columns != 0);
        next_row = Sum16Mode(WramRead16(wram, WORK_POSITION), ROW_STRIDE, false, cpu->decimal);
        WramWrite16(wram, WORK_POSITION, next_row.value);
    } while (WramStep16(wram, WORK_HEIGHT, -1) != 0);
    cpu->x = cell;
    cpu->y = 0;
    SetAccumulatorWidth(cpu, 1);
    cpu->accumulator = next_row.value;
    SetSumFlags(cpu, next_row);
    LoadA8(cpu, REDRAW_REQUEST);
    redraw = WramRead(wram, REDRAW_FLAGS);
    WramWrite(wram, REDRAW_FLAGS, (uint8_t)(redraw | REDRAW_REQUEST));
    cpu->zero = (redraw & REDRAW_REQUEST) == 0;
    return WaitForRedraw(memory, cpu, 0x810au);
}

/* $82:838F: zeroes both menu layers, then the redraw wait with $74 = $88;
 * any M, X16. The layers are written through the bank $7E data bank the
 * original enters and leaves. */
Lufia2ExecutionResult Lufia2MenuClearLayers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t offset = 0;

    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x82838fu);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, CLEAR_BANK);
    do {
        WramWrite16At(wram, LAYER_FRONT, offset, 0);
        WramWrite16At(wram, LAYER_BACK, offset, 0);
        offset = (uint16_t)(offset + 2u);
    } while (offset != LAYER_END);
    PullDataBank(memory, cpu);
    cpu->x = offset;
    cpu->carry = true;
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, CLEAR_REDRAW);
    WramWrite(wram, REDRAW_FLAGS, CLEAR_REDRAW);
    return WaitForRedraw(memory, cpu, 0x83b3u);
}
