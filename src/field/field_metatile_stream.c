#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    STREAM_ROW_LEFT = 0x26u,
    STREAM_COLUMN_LEFT = 0x28u,
    STREAM_BUFFER = 0x2au,
    STREAM_METATILES = 0x65u,
    STREAM_WIDTH = 0x83u,
    STREAM_HEIGHT = 0x85u,
    STREAM_CELL_X = 0x87u,
    STREAM_CELL_Y = 0x89u,
    STREAM_ROW_BYTES = 0x8bu,
    STREAM_SHARED_CELL = 0x3000u,
    STREAM_METATILE_MASK = 0x03ffu,
    STREAM_ROW_STEP = 0x003eu,
    STREAM_BUFFER_MASK = 0x07ffu
};

static uint8_t MetatileStreamReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && !cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static void WriteMetatile(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0u, cpu->x));
    And16(cpu, STREAM_SHARED_CELL);
    Compare16(cpu, cpu->accumulator, STREAM_SHARED_CELL);
    if (cpu->zero)
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu,
            WRAM_FIELD_LAYER_CELL_BASE & 0xffffu, 0u));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0u, cpu->x));
    And16(cpu, STREAM_METATILE_MASK);
    for (unsigned bit = 0u; bit < 3u; ++bit)
        AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_METATILES));
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER), cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 4u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER), cpu->accumulator);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0u;
    Add16Value(cpu, STREAM_ROW_STEP);
    TransferAToY(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 2u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER), cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 6u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER), cpu->accumulator);
}

static uint8_t AdvanceStreamCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint8_t coordinate, uint8_t limit, uint32_t site) {
    LoadA16(cpu, Read16Direct(memory, cpu, coordinate));
    IncrementA16(cpu);
    Write16Direct(memory, cpu, coordinate, cpu->accumulator);
    if (!cpu->zero) {
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, limit));
        if (!cpu->carry)
            return 1u;
        Write16Direct(memory, cpu, coordinate, 0u);
    }
    return CallChildWithFrame(memory, cpu, child, context, site, 0x80f6aau, 2u, 0x80u);
}

static Lufia2ExecutionResult MetatileStreamUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2FieldRenderMetatileColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MetatileStreamReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x80f5edu);
    LoadA16(cpu, 16u);
    Write16Direct(memory, cpu, STREAM_COLUMN_LEFT, cpu->accumulator);
    do {
        PushIndex(memory, cpu);
        WriteMetatile(memory, cpu);
        LoadA16(cpu, cpu->y);
        cpu->carry = 0u;
        Add16Value(cpu, STREAM_ROW_STEP);
        And16(cpu, STREAM_BUFFER_MASK);
        TransferAToY(cpu);
        PullAccumulator16(memory, cpu);
        cpu->carry = 0u;
        Add16Value(cpu, Read16Direct(memory, cpu, STREAM_ROW_BYTES));
        TransferAToX(cpu);
        if (!AdvanceStreamCell(memory, cpu, child, context,
                STREAM_CELL_Y, STREAM_HEIGHT, 0x80f646u))
            return MetatileStreamUnwound(0x80f646u);
        OpStepMem(memory, cpu, OpDp(cpu, STREAM_COLUMN_LEFT), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x80f64du);
}

Lufia2ExecutionResult Lufia2FieldRenderMetatileRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MetatileStreamReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x80f64eu);
    Write16Direct(memory, cpu, STREAM_ROW_LEFT, cpu->accumulator);
    do {
        PushIndex(memory, cpu);
        WriteMetatile(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        LoadA16(cpu, cpu->y);
        Subtract16(cpu, STREAM_ROW_STEP);
        And16(cpu, STREAM_BUFFER_MASK);
        TransferAToY(cpu);
        if (!AdvanceStreamCell(memory, cpu, child, context,
                STREAM_CELL_X, STREAM_WIDTH, 0x80f6a2u))
            return MetatileStreamUnwound(0x80f6a2u);
        OpStepMem(memory, cpu, OpDp(cpu, STREAM_ROW_LEFT), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x80f6a9u);
}
