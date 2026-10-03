/* Menu tile map buffers: clearing both layers, filling a block of rising tile
 * numbers, and recoloring a rectangle. Each of the three that wait for a
 * redraw stops at the frame wait of $82:93C2. */

#include "core/cpu_internal.h"
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
    WORK_POSITION = 0x54u,
    WORK_WIDTH = 0x56u,
    WORK_HEIGHT = 0x58u,
    WORK_TILE = 0x5au,
    WORK_COLUMNS = 0x63u,
    WORK_ROWS = 0x65u,
    REDRAW_FLAGS = 0x74u,
    REDRAW_WAIT = 0x8293c2u
};

/* Decrements a direct-page word the way a read-modify-write does. */
static void DecrementWork(Lufia2Wram wram, Lufia2CpuState *cpu, uint32_t location) {
    SetNz16(cpu, WramStep16(wram, location, -1));
}

/* Stops at the frame wait with the routine's own return pushed. */
static Lufia2ExecutionResult WaitForRedraw(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    return ExecutionHandoff(cpu, REDRAW_WAIT);
}

/* $82:80A5: the 4 x 4 block at position $54 receives the tile number $5A
 * counting up along each row and carrying on into the next. M0X0. */
static void FillTileBlock(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    LoadX16(cpu, BLOCK_SIZE);
    StoreXDirect16(memory, cpu, WORK_WIDTH);
    StoreXDirect16(memory, cpu, WORK_HEIGHT);
    LoadXDirect16(memory, cpu, WORK_POSITION);
    LoadADirect16(memory, cpu, WORK_TILE);
    do {
        LoadYDirect16(memory, cpu, WORK_WIDTH);
        PushIndex(memory, cpu);
        do {
            Write16Long(memory, LongIndexedAddress(TILEMAP, cpu->x),
                cpu->accumulator);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementA16(cpu);
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->zero);
        TransferAToY(cpu);
        PullAccumulator16(memory, cpu);
        cpu->carry = 0;
        Add16Value(cpu, ROW_STRIDE);
        TransferAToX(cpu);
        LoadA16(cpu, cpu->y);
        DecrementWork(wram, cpu, WORK_HEIGHT);
    } while (!cpu->zero);
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

    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x828069u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, GRID_BASE_TILE);
    cpu->carry = 0;
    Add16Value(cpu, WramRead16(wram, GRID_TILE_OFFSET));
    StoreADirect16(memory, cpu, WORK_TILE);
    Write16Direct(memory, cpu, WORK_POSITION, 0);
    LoadX16(cpu, GRID_SIZE);
    StoreXDirect16(memory, cpu, WORK_ROWS);
    do {
        LoadX16(cpu, GRID_SIZE);
        StoreXDirect16(memory, cpu, WORK_COLUMNS);
        do {
            SimulateJsrFrame(memory, cpu, 0x8082u);
            FillTileBlock(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            LoadADirect16(memory, cpu, WORK_POSITION);
            cpu->carry = 0;
            Add16Value(cpu, GRID_BLOCK_STEP);
            StoreADirect16(memory, cpu, WORK_POSITION);
            DecrementWork(wram, cpu, WORK_COLUMNS);
        } while (!cpu->zero);
        LoadADirect16(memory, cpu, WORK_POSITION);
        cpu->carry = 0;
        Add16Value(cpu, GRID_ROW_SKIP);
        StoreADirect16(memory, cpu, WORK_POSITION);
        DecrementWork(wram, cpu, WORK_ROWS);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x20u);
    StoreADirect8(memory, cpu, REDRAW_FLAGS);
    return WaitForRedraw(memory, cpu, 0x80a3u);
}

/* $82:80CA: the rectangle at A, X = width << 8 | rows, takes the palette
 * number in Y; then the redraw wait. M0X0. */
Lufia2ExecutionResult Lufia2MenuRecolorRect(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8280cau);
    StoreADirect16(memory, cpu, WORK_POSITION);
    Write16Direct(memory, cpu, WORK_WIDTH, 0);
    Write16Direct(memory, cpu, WORK_HEIGHT, 0);
    LoadA16(cpu, cpu->y);
    ExchangeAccumulatorBytes(cpu);
    AslA16(cpu);
    AslA16(cpu);
    StoreADirect16(memory, cpu, WORK_TILE);
    TransferXToA(cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, WORK_HEIGHT);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, WORK_WIDTH);
    SetAccumulatorWidth(cpu, 0);
    do {
        LoadYDirect16(memory, cpu, WORK_WIDTH);
        LoadXDirect16(memory, cpu, WORK_POSITION);
        do {
            LoadA16(cpu, Read16Long(memory,
                LongIndexedAddress(TILEMAP, cpu->x)));
            And16(cpu, COLOR_KEEP);
            Or16(cpu, Read16Direct(memory, cpu, WORK_TILE));
            Write16Long(memory, LongIndexedAddress(TILEMAP, cpu->x),
                cpu->accumulator);
            IncrementX16(cpu);
            IncrementX16(cpu);
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->zero);
        LoadADirect16(memory, cpu, WORK_POSITION);
        cpu->carry = 0;
        Add16Value(cpu, ROW_STRIDE);
        StoreADirect16(memory, cpu, WORK_POSITION);
        DecrementWork(wram, cpu, WORK_HEIGHT);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x20u);
    TestBitsDirect(memory, cpu, REDRAW_FLAGS, 1);
    return WaitForRedraw(memory, cpu, 0x810au);
}

/* $82:838F: zeroes both menu layers, then the redraw wait with $74 = $88;
 * any M, X16. The layers are written through the bank $7E data bank the
 * original enters and leaves. */
Lufia2ExecutionResult Lufia2MenuClearLayers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x82838fu);
    SetAccumulatorWidth(cpu, 0);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, 0);
    do {
        Write16Long(memory, LongIndexedAddress(LAYER_FRONT, cpu->x), 0);
        Write16Long(memory, LongIndexedAddress(LAYER_BACK, cpu->x), 0);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, LAYER_END);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x88u);
    StoreADirect8(memory, cpu, REDRAW_FLAGS);
    return WaitForRedraw(memory, cpu, 0x83b3u);
}
