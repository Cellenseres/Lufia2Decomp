#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "battle/sprite_render_internal.h"

Lufia2ExecutionResult Lufia2BattleDrawSpriteGrid(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    bool mirrored) {
    PushAndSetDataBank(memory, cpu, 0x7eu);
    if (mirrored) {
        LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_WIDTH));
        DecrementA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        OpAdcValue(cpu, DirectByte(memory, cpu, BATTLE_GRID_X));
        StoreADirect8(memory, cpu, BATTLE_GRID_X);
    }
    Write8(memory, DirectAddress(cpu, BATTLE_GRID_EMITTED), 0x00u);
    TransferDirectToA(cpu);
    cpu->carry = 1;
    OpSbcValue(cpu, DirectByte(memory, cpu, BATTLE_GRID_X_SIGN));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_X));
    TransferAToY(cpu);
    StoreYDirect16(memory, cpu, BATTLE_GRID_COLUMN_X);
    StoreYDirect16(memory, cpu, BATTLE_GRID_ROW_X);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_TILE));
    AslA8(cpu);
    StoreADirect8(memory, cpu, BATTLE_GRID_ROW_TILE);
    And8(cpu, 0xf0u);
    OpAdcValue(cpu, DirectByte(memory, cpu, BATTLE_GRID_ROW_TILE));
    StoreADirect8(memory, cpu, BATTLE_GRID_ROW_TILE);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_Y));
    DecrementA8(cpu);
    StoreADirect8(memory, cpu, BATTLE_GRID_ROW_Y);
    LoadXDirect16(memory, cpu, BATTLE_GRID_WRITE_OFFSET);
    do {
        LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_WIDTH));
        StoreADirect8(memory, cpu, BATTLE_GRID_COLUMNS_LEFT);
        do {
            static const uint8_t fields[4] = {BATTLE_GRID_COLUMN_X, BATTLE_GRID_ROW_Y, BATTLE_GRID_ROW_TILE, BATTLE_GRID_ATTRIBUTES};
            unsigned i;

            for (i = 0; i < 4u; ++i) {
                LoadA8(cpu, DirectByte(memory, cpu, fields[i]));
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
                IncrementX16(cpu);
            }
            LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_COLUMN_X_HIGH));
            And8(cpu, 0x03u);
            StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
            IncrementX16(cpu);
            IncrementDirect8(memory, cpu, BATTLE_GRID_EMITTED);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x17u));
            if (mirrored) {
                cpu->carry = 1;
                OpSbcValue(cpu, 0x0010u);
            } else {
                cpu->carry = 0;
                OpAdcValue(cpu, 0x0010u);
            }
            Write16Direct(memory, cpu, BATTLE_GRID_COLUMN_X, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_ROW_TILE));
            cpu->carry = 0;
            OpAdcValue(cpu, 0x02u);
            BitImmediate8(cpu, 0x0fu);
            if (cpu->zero)
                OpAdcValue(cpu, 0x10u);
            StoreADirect8(memory, cpu, BATTLE_GRID_ROW_TILE);
            DecrementDirect8(memory, cpu, BATTLE_GRID_COLUMNS_LEFT);
        } while (!cpu->zero);
        LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_ROW_Y));
        cpu->carry = 0;
        OpAdcValue(cpu, 0x10u);
        StoreADirect8(memory, cpu, BATTLE_GRID_ROW_Y);
        LoadYDirect16(memory, cpu, BATTLE_GRID_ROW_X);
        StoreYDirect16(memory, cpu, BATTLE_GRID_COLUMN_X);
        DecrementDirect8(memory, cpu, BATTLE_GRID_HEIGHT);
    } while (!cpu->zero);
    StoreXDirect16(memory, cpu, BATTLE_GRID_WRITE_OFFSET);
    LoadA8(cpu, DirectByte(memory, cpu, BATTLE_GRID_EMITTED));
    PullDataBank(memory, cpu);
    return ExecutionReturned(mirrored ? 0x81be53u : 0x81bdc7u);
}

Lufia2ExecutionResult Lufia2BattleMirrorSpriteBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x81bdccu);
    return Lufia2BattleDrawSpriteGrid(memory, cpu, true);
}

Lufia2ExecutionResult Lufia2BattleMirrorSpriteBlockFar(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x81bdc8u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81bdc8u, 0x81bdccu, 2u, 0x81u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x81bdc8u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    return ExecutionReturned(0x81bdcbu);
}
