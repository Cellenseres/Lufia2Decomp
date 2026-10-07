#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

static uint8_t CellAddressReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && !cpu->index_is_8_bit && !cpu->decimal;
}

static Lufia2ExecutionResult CellAddressUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2FieldDivisionDelay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!CellAddressReady(cpu) || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x80f81cu);
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0x80f820u);
}

enum {
    STREAM_METATILE_BASE = 0x65u,
    STREAM_LAYER_WIDTH = 0x83u,
    STREAM_LAYER_HEIGHT = 0x85u,
    STREAM_CELL_X = 0x87u,
    STREAM_CELL_Y = 0x89u,
    STREAM_CELL_BASE = 0x8du,
    STREAM_CELL_ADDRESS = 0x30u,
    STREAM_BUFFER_COLUMN = 0x22u,
    STREAM_BUFFER_ROW = 0x24u,
    STREAM_BUFFER_OFFSET = 0x2du,
    STREAM_BUFFER = 0x2au,
    STREAM_BUFFER_BANK = 0x2cu,
    STREAM_ROW_TILES_LEFT = 0x26u,
    STREAM_COLUMN_TILES_LEFT = 0x28u,
    STREAM_BUFFER_COLUMN_BYTES = 0x003eu,
    STREAM_COORDINATE_SIGN = 0x0800u,
    STREAM_COORDINATE_SIGN_EXTEND = 0xf000u,
    STREAM_FIRST_ALTERNATE_LAYER = 0x0004u,
    LAYER_BUFFER_TABLE = 0x838ff0u,
};

static uint32_t WrapCellCoordinate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint8_t divisor,
    uint16_t negative_return,
    uint16_t positive_return,
    uint8_t short_cut) {
    cpu->zero = (cpu->accumulator & STREAM_COORDINATE_SIGN) == 0;
    if (!cpu->zero) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator | STREAM_COORDINATE_SIGN_EXTEND));
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        IncrementA16(cpu);
        Write16Long(memory, SNES_WRDIVL, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, divisor)));
        Write8(memory, SNES_WRDIVB, A8(cpu));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x800000u | (uint16_t)(negative_return - 2u), 0x80f81cu, 2u, 0x80u))
            return 0x800000u | (uint16_t)(negative_return - 2u);
        LoadA16(cpu, Read16Direct(memory, cpu, divisor));
        Subtract16(cpu, Read16Long(memory, SNES_RDMPYL));
        return 0u;
    }
    if (short_cut) {
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, divisor));
        if (!cpu->carry)
            return 0u;
    }
    Write16Long(memory, SNES_WRDIVL, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, divisor)));
    Write8(memory, SNES_WRDIVB, A8(cpu));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x800000u | (uint16_t)(positive_return - 2u), 0x80f81cu, 2u, 0x80u))
        return 0x800000u | (uint16_t)(positive_return - 2u);
    LoadA16(cpu, Read16Long(memory, SNES_RDMPYL));
    return 0u;
}


Lufia2ExecutionResult Lufia2FieldLocateCell(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CellAddressReady(cpu) || cpu->accumulator_is_8_bit || !child)
        return ExecutionHandoff(cpu, 0x80f734u);
    for (unsigned bit = 0u; bit < 4u; ++bit)
        LsrA16(cpu);
    Write16Direct(memory, cpu, STREAM_CELL_ADDRESS, cpu->accumulator);
    Write16Direct(memory, cpu, STREAM_BUFFER_COLUMN, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_WIDTH & 0xffffu), cpu->x));
    Write16Direct(memory, cpu, STREAM_LAYER_WIDTH, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_HEIGHT & 0xffffu), cpu->x));
    Write16Direct(memory, cpu, STREAM_LAYER_HEIGHT, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_COLUMN));
    const uint32_t unwind_0 = WrapCellCoordinate(memory, cpu, child, context,
        STREAM_LAYER_WIDTH, 0xf762u, 0xf77au, 0);
    if (unwind_0)
        return CellAddressUnwound(unwind_0);
    Write16Direct(memory, cpu, STREAM_CELL_X, cpu->accumulator);
    Write16Direct(memory, cpu, STREAM_ROW_TILES_LEFT, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_CELL_ADDRESS));
    for (unsigned bit = 0u; bit < 2u; ++bit)
        AslA16(cpu);
    And16(cpu, STREAM_BUFFER_COLUMN_BYTES);
    Write16Direct(memory, cpu, STREAM_BUFFER_COLUMN, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    for (unsigned bit = 0u; bit < 4u; ++bit)
        LsrA16(cpu);
    Write16Direct(memory, cpu, STREAM_COLUMN_TILES_LEFT, cpu->accumulator);
    const uint32_t unwind_1 = WrapCellCoordinate(memory, cpu, child, context,
        STREAM_LAYER_HEIGHT, 0xf7adu, 0xf7c9u, 1);
    if (unwind_1)
        return CellAddressUnwound(unwind_1);
    Write16Direct(memory, cpu, STREAM_CELL_Y, cpu->accumulator);
    Write16Direct(memory, cpu, STREAM_COLUMN_TILES_LEFT, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0x00f0u);
    for (unsigned bit = 0u; bit < 3u; ++bit)
        AslA16(cpu);
    Write16Direct(memory, cpu, STREAM_BUFFER_ROW, cpu->accumulator);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_COLUMN));
    Write16Direct(memory, cpu, STREAM_BUFFER_OFFSET, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(LAYER_BUFFER_TABLE, cpu->x)));
    Write16Direct(memory, cpu, STREAM_BUFFER, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, STREAM_COLUMN_TILES_LEFT)));
    Write8(memory, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, STREAM_LAYER_WIDTH)));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    LoadA8(cpu, 0x7eu);
    Write8(memory, DirectAddress(cpu, STREAM_BUFFER_BANK), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_CELL_BASE & 0xffffu), cpu->x));
    Write16Direct(memory, cpu, STREAM_CELL_BASE, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, SNES_RDMPYL));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_ROW_TILES_LEFT));
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_CELL_BASE));
    Write16Direct(memory, cpu, STREAM_CELL_ADDRESS, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_METATILE_BASE & 0xffffu), 0));
    Write16Direct(memory, cpu, STREAM_METATILE_BASE, cpu->accumulator);
    Compare16(cpu, cpu->x, STREAM_FIRST_ALTERNATE_LAYER);
    if (cpu->carry) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_ALTERNATE_METATILE_BASE & 0xffffu), 0));
        Write16Direct(memory, cpu, STREAM_METATILE_BASE, cpu->accumulator);
    }
    cpu->carry = 0;
    return ExecutionReturned(0x80f81bu);
}

Lufia2ExecutionResult Lufia2FieldCellIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (!CellAddressReady(cpu))
        return ExecutionHandoff(cpu, 0x80f6aau);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, STREAM_CELL_Y)));
    Write8(memory, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, STREAM_LAYER_WIDTH)));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_CELL_X));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, SNES_RDMPYL));
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_CELL_BASE));
    TransferAToX(cpu);
    return ExecutionReturned(0x80f6c5u);
}
