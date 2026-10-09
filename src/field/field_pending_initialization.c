#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/field.h"

enum {
    PENDING_TILE_RECORD_SIZE = 4u,
    PENDING_TILE_RECORDS = WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu,
    PENDING_RECORD_FLAGS = WRAM_FIELD_PENDING_RECORD_FLAGS & 0xffffu,
    PENDING_RECORD_X = WRAM_FIELD_PENDING_RECORD_X & 0xffffu,
    PENDING_RECORD_Y = WRAM_FIELD_PENDING_RECORD_Y & 0xffffu,
    PENDING_OBJECT_RECORD = WRAM_FIELD_PENDING_OBJECT_RECORD & 0xffffu,
    MAP_OBJECT_RECORDS = 0x7ef000u,
    MAP_OBJECT_RECORD_SIZE = 3u,
    DP_PENDING_SOURCE_X = 0x8fu,
    DP_PENDING_SOURCE_Y = 0x91u
};

static Lufia2ExecutionResult PendingInitializationUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void ClearPendingObjectTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, (WRAM_FIELD_PENDING_OBJECT_TILES_COUNT - 1u) * PENDING_TILE_RECORD_SIZE);
    OpRepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_TILE_RECORDS));
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_TILE_RECORDS + 2u));
        for (unsigned byte = 0u; byte < PENDING_TILE_RECORD_SIZE; ++byte)
            OpDey(cpu);
    } while (!cpu->negative);
}

static void ClearPendingObjectCoordinates(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, WRAM_FIELD_PENDING_RECORD_X_COUNT - PENDING_TILE_RECORD_SIZE);
    do {
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_RECORD_FLAGS));
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_RECORD_FLAGS + 2u));
        OpLoadA(cpu, 0xffffu);
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_RECORD_X));
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_RECORD_X + 2u));
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_RECORD_Y));
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_RECORD_Y + 2u));
        for (unsigned byte = 0u; byte < PENDING_TILE_RECORD_SIZE; ++byte)
            OpDey(cpu);
    } while (!cpu->negative);
}

Lufia2ExecutionResult Lufia2FieldInitializePendingObjects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80ea5bu);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    ClearPendingObjectTiles(memory, cpu);
    ClearPendingObjectCoordinates(memory, cpu);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_LIST_OFFSET);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    for (;;) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, MAP_OBJECT_RECORDS));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) break;
        OpTay(cpu);
        OpLda(memory, cpu, OpLongX(cpu, MAP_OBJECT_RECORDS + 1u));
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_RECORD_X));
        OpSta(memory, cpu, OpDp(cpu, DP_PENDING_SOURCE_X));
        OpLda(memory, cpu, OpLongX(cpu, MAP_OBJECT_RECORDS + 2u));
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_RECORD_Y));
        OpSta(memory, cpu, OpDp(cpu, DP_PENDING_SOURCE_Y));
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_RECORD_FLAGS));
        PushIndex(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x80eabfu, 0x83fb51u, 3u, cpu->program_bank))
            return PendingInitializationUnwound(0x80eabfu);
        OpTxa(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, PENDING_OBJECT_RECORD));
        OpPullX(memory, cpu);
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, MAP_OBJECT_RECORD_SIZE);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
    }
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 4u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x80eadcu, 0x80e722u, 3u, cpu->program_bank))
        return PendingInitializationUnwound(0x80eadcu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x80eae0u, 0x80cbaeu, 3u, cpu->program_bank))
        return PendingInitializationUnwound(0x80eae0u);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80eae6u);
}
