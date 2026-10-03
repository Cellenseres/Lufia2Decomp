/* Battle tile copy: rows of 64 bytes, each with a second plane $40 bytes
 * further on, copied into the two buffers $200 bytes apart. */

#include "core/cpu_internal.h"
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
    PLANE_STEP = 0x0040u,
    BLOCK_STEP = 0x0200u
};

/* $81:BCCC: copies rows from the source word at $08 into the buffers at
 * $7E:X and $7E:X+$200. The first byte of the product of the two bytes at
 * $02/$03 gives the number of rows, and the position in $00 selects the
 * start inside the 8-row block of the target. M8/X16, JSL. */
Lufia2ExecutionResult Lufia2BattleBlitTileRows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x81bcccu);
    LoadXDirect16(memory, cpu, SCALE_X);
    Write16Absolute(memory, cpu, HARDWARE_A, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, ROW_COUNT_LOW));
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x0007u);
    StoreADirect16(memory, cpu, COLUMN_SKIP);
    LoadADirect16(memory, cpu, ROW_COUNT_LOW);
    And16(cpu, 0x00f8u);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, ROW_COUNT_LOW));
    And16(cpu, 0x00ffu);
    ExchangeAccumulatorBytes(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    PushAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, TARGET_OFFSET);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, AbsoluteByte(memory, cpu, HARDWARE_PRODUCT, 0));
    StoreADirect8(memory, cpu, ROW_COUNT_HIGH);
    StoreADirect8(memory, cpu, ROWS_LEFT);
    LoadA8(cpu, 0x08u);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, COLUMN_SKIP));
    StoreADirect8(memory, cpu, COLUMN_SKIP);
    SetAccumulatorWidth(cpu, 0);
    PullAccumulator16(memory, cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, TARGET));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadYDirect16(memory, cpu, SOURCE);
    PushDataBank(memory, cpu);
    SelectDataBank(memory, cpu, 0x7eu);
    for (;;) {
        LoadA8(cpu, 0x40u);
        StoreADirect8(memory, cpu, ROW_BYTES);
        do {
            LoadA8(cpu, AbsoluteByte(memory, cpu, 0x0000u, cpu->y));
            StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
            LoadA8(cpu, AbsoluteByte(memory, cpu, PLANE_STEP, cpu->y));
            StoreAAbsolute8(memory, cpu, BLOCK_STEP, cpu->x);
            IncrementX16(cpu);
            IncrementY16(cpu);
            DecrementDirect8(memory, cpu, ROW_BYTES);
        } while (!cpu->zero);
        IncrementDirect8(memory, cpu, ROW_COUNT_LOW);
        DecrementDirect8(memory, cpu, ROWS_LEFT);
        if (cpu->zero)
            break;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, PLANE_STEP);
        TransferAToY(cpu);
        SetAccumulatorWidth(cpu, 1);
        DecrementDirect8(memory, cpu, COLUMN_SKIP);
        if (!cpu->zero)
            continue;
        LoadA8(cpu, 0x08u);
        StoreADirect8(memory, cpu, COLUMN_SKIP);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, cpu->x);
        cpu->carry = 0;
        Add16Value(cpu, BLOCK_STEP);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81bd46u);
}
