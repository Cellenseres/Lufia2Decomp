#include "lufia2/battle.h"
#include "core/cpu_ops.h"

enum {
    DP_GRID_TILE = 0x00u,
    DP_GRID_COLUMNS = 0x02u,
    DP_GRID_ROWS = 0x03u,
    DP_GRID_ATTRIBUTES = 0x04u,
    DP_GRID_Y = 0x05u,
    DP_GRID_X_LOW = 0x06u,
    DP_GRID_X_SIGN = 0x07u,
    DP_GRID_OUTPUT = 0x08u,
    DP_GRID_TILE_CURSOR = 0x11u,
    DP_GRID_COLUMNS_LEFT = 0x13u,
    DP_GRID_SPRITE_COUNT = 0x15u,
    DP_GRID_ROW_Y = 0x16u,
    DP_GRID_COLUMN_X = 0x17u,
    DP_GRID_ROW_X = 0x1cu,
    GRID_CELL_SIZE = 16u
};

static void InitializeSpriteGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, DP_GRID_SPRITE_COUNT));
    TransferDirectToA(cpu);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_GRID_X_SIGN)));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_GRID_X_LOW));
    OpTay(cpu);
    Write16Direct(memory, cpu, DP_GRID_COLUMN_X, cpu->y);
    Write16Direct(memory, cpu, DP_GRID_ROW_X, cpu->y);
    OpLda(memory, cpu, OpDp(cpu, DP_GRID_TILE));
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_TILE_CURSOR));
    OpAndValue(cpu, 0xf0u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_GRID_TILE_CURSOR));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x10u);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_TILE_CURSOR));
    OpLda(memory, cpu, OpDp(cpu, DP_GRID_Y));
    OpDecA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_ROW_Y));
    OpLdx(cpu, Read16Direct(memory, cpu, DP_GRID_OUTPUT));
}

static void WriteSpriteGridCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint8_t fields[] = {
        DP_GRID_COLUMN_X, DP_GRID_ROW_Y, DP_GRID_TILE_CURSOR,
        DP_GRID_ATTRIBUTES, DP_GRID_COLUMN_X + 1u
    };
    for (unsigned field = 0u; field < 5u; ++field) {
        OpLda(memory, cpu, OpDp(cpu, fields[field]));
        if (field == 4u)
            OpAndValue(cpu, 3u);
        OpSta(memory, cpu, OpAbsX(cpu, 0u));
        OpInx(cpu);
    }
    OpStepMem(memory, cpu, OpDp(cpu, DP_GRID_SPRITE_COUNT), 1);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_GRID_COLUMN_X));
    cpu->carry = 0u;
    OpAdcValue(cpu, GRID_CELL_SIZE);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_COLUMN_X));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_GRID_TILE_CURSOR));
    cpu->carry = 0u;
    OpAdcValue(cpu, 2u);
    OpBitValue(cpu, 0x0fu);
    if (cpu->zero)
        OpAdcValue(cpu, 0x10u);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_TILE_CURSOR));
}

Lufia2ExecutionResult Lufia2BattleBuildSpriteGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859884u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    InitializeSpriteGrid(memory, cpu);
    do {
        OpLda(memory, cpu, OpDp(cpu, DP_GRID_COLUMNS));
        OpSta(memory, cpu, OpDp(cpu, DP_GRID_COLUMNS_LEFT));
        do {
            WriteSpriteGridCell(memory, cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_GRID_COLUMNS_LEFT), -1);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpDp(cpu, DP_GRID_ROW_Y));
        cpu->carry = 0u;
        OpAdcValue(cpu, GRID_CELL_SIZE);
        OpSta(memory, cpu, OpDp(cpu, DP_GRID_ROW_Y));
        OpLdy(cpu, Read16Direct(memory, cpu, DP_GRID_ROW_X));
        Write16Direct(memory, cpu, DP_GRID_COLUMN_X, cpu->y);
        OpStepMem(memory, cpu, OpDp(cpu, DP_GRID_ROWS), -1);
    } while (!cpu->zero);
    Write16Direct(memory, cpu, DP_GRID_OUTPUT, cpu->x);
    OpLda(memory, cpu, OpDp(cpu, DP_GRID_SPRITE_COUNT));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x859904u);
}
