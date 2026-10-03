/* Object attributes, saved tiles and redraw queue requests. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    OBJECT_ATTRIBUTE_MASK = 0x54,
    OBJECT_ATTRIBUTE_BITS = 0x55,
    OBJECT_UPPER_ATTRIBUTE_MASK = 0x56,
    OBJECT_UPPER_ATTRIBUTE_BITS = 0x57,
    OBJECT_VERTICAL_OFFSET = 0x58,
    OBJECT_ORIGINAL_ATTRIBUTE = 0x5a,
    OBJECT_REDRAW_COLUMN_OFFSET = 0x54,
    OBJECT_REDRAW_VRAM_BASE = 0x56,
    OBJECT_REDRAW_SOURCE_BASE = 0x58,
    OBJECT_REDRAW_ORIGIN_Y = 0xa0,
    OBJECT_PENDING_SLOT = 0x65,
    OBJECT_PENDING_CELL = 0x63,
};

/* Run target via the pushed-child hook; 0 when unwound. */
static uint8_t FieldTileChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t target, uint32_t site, uint8_t frame_size) {
    const uint16_t return_word = (uint16_t)(site + frame_size);
    if (frame_size == 3u)
        SimulateJslFrame(memory, cpu, 0x83u, return_word);
    else
        SimulateJsrFrame(memory, cpu, return_word);
    return child(context, cpu, target, site, frame_size);
}

/* Result for a caller whose child unwound past it. */
static Lufia2ExecutionResult FieldTileUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

/* Rewrite cell X's attribute with the $54/$55 masks. */
Lufia2ExecutionResult Lufia2FieldMarkObjectAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FieldTileChild(memory, cpu, child, context,
            0x83f9adu, 0x83f80du, 2u))
        return FieldTileUnwound(0x83f80du);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ORIGINAL_ATTRIBUTE));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_MASK)));
    OpOra(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_BITS));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT));
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET + 1u));
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpCmpValue(cpu, 2u);
    cpu->carry = 0;
    if (cpu->zero) {
        OpLoadA(cpu, 0x00f0u);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
        OpLoadA(cpu, 0x00ffu);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET + 1u));
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_MASK));
        cpu->carry = 1;
        OpSbcValue(cpu, OpReadM(memory, cpu, 0x0005b9u));
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
        OpAndValue(cpu, OpReadM(memory, cpu,
            OpDp(cpu, OBJECT_UPPER_ATTRIBUTE_MASK)));
        OpOra(memory, cpu, OpDp(cpu, OBJECT_UPPER_ATTRIBUTE_BITS));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
        cpu->carry = 1;
    }
    OpLda(memory, cpu, OpDp(cpu, OBJECT_ORIGINAL_ATTRIBUTE));
    return ExecutionReturned(0x83f859u);
}

/* Write back the tile words saved for the pending object. */
Lufia2ExecutionResult Lufia2FieldRestoreObjectTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_PENDING_OBJECT));
    OpAslA(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    if (!FieldTileChild(memory, cpu, child, context,
            0x83f9d4u, 0x83f753u, 2u))
        return FieldTileUnwound(0x83f753u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu,
        (WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu)));
    if (!FieldTileChild(memory, cpu, child, context,
            0x83f784u, 0x83f760u, 2u))
        return FieldTileUnwound(0x83f760u);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpAndValue(cpu, 0x00ffu);
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        OpTxa(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, 0x0005b9u);
        OpAdc(memory, cpu, 0x0005b9u);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsY(cpu,
            (WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu) + 2u));
        if (!FieldTileChild(memory, cpu, child, context,
                0x83f784u, 0x83f77du, 2u))
            return FieldTileUnwound(0x83f77du);
    }
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x83f783u);
}

/* Queue a column upload for the object's rectangle. */
Lufia2ExecutionResult Lufia2FieldQueueObjectRedraw(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FieldTileChild(memory, cpu, child, context,
            0x83f611u, 0x83f933u, 2u))
        return FieldTileUnwound(0x83f933u);
    if (!FieldTileChild(memory, cpu, child, context,
            0x838e85u, 0x83f936u, 3u))
        return FieldTileUnwound(0x83f936u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x838ff8u));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_REDRAW_VRAM_BASE));
    OpLda(memory, cpu, OpLongX(cpu, 0x838ff0u));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_REDRAW_SOURCE_BASE));
    OpLda(memory, cpu, WRAM_FIELD_COLUMN_UPLOAD_COUNT_BYTES);
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, OBJECT_REDRAW_ORIGIN_Y));
    OpAndValue(cpu, 0x000fu);
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_REDRAW_COLUMN_OFFSET));
    OpLsrA(cpu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, OBJECT_REDRAW_VRAM_BASE));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_COLUMN_UPLOAD_DESTINATION));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_REDRAW_COLUMN_OFFSET));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, OBJECT_REDRAW_SOURCE_BASE));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_COLUMN_UPLOAD_SOURCE));
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_COLUMN_UPLOAD_ROWS));
    OpTxa(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_COLUMN_UPLOAD_COUNT_BYTES);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x83f97bu);
}

/* Map cell word: low ten bits metatile, rest attributes. */
enum {
    CELL_METATILE_MASK = 0x03ff,
    CELL_ATTRIBUTE_MASK = 0xfc00,
    CELL_SIZE = 2,
    DP_CELL_METATILE = 0x5a,
};

/* Cells of every layer; 16-bit offsets that wrap. */
#define MAP_LAYER_CELLS 0x7f0000u

/* Cell Y takes cell X's metatile, keeps its attributes. */
Lufia2ExecutionResult Lufia2FieldCopyCellTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint32_t target = AbsoluteIndexedAddress(cpu, 0, cpu->y);
    const uint16_t metatile =
        Read16AbsoluteIndexed(memory, cpu, 0, cpu->x) & CELL_METATILE_MASK;
    uint16_t cell;

    WramWrite16(wram, DP_CELL_METATILE, metatile);
    cell = Read16Long(memory, target) & CELL_ATTRIBUTE_MASK;
    cell |= WramRead16(wram, DP_CELL_METATILE);
    Write16Long(memory, target, cell);
    LoadA16(cpu, cell);
    return ExecutionReturned(0x83f932u);
}

/* Counters of the clearing loop live in the direct page. */
enum {
    DP_CLEAR_LAYER_INDEX = 0x54,
    DP_CLEAR_ROW_SKIP = 0x54,
    DP_CLEAR_COLUMNS_LEFT = 0x56,
    DP_CLEAR_ROWS_LEFT = 0x58,
    /* Rectangles this large go to the interpreter. */
    CLEAR_CELL_LIMIT = 262144,
};

/* Offset kept in A keeps the last sum flags. */
static uint16_t AddKeepingFlags(Lufia2CpuState *cpu, uint16_t base, uint16_t addend) {
    cpu->accumulator = base;
    cpu->carry = 0;
    Add16Value(cpu, addend);
    return cpu->accumulator;
}

/* Clear loop registers at the cell limit. */
static Lufia2ExecutionResult HandOffClearLoop(Lufia2CpuState *cpu, uint16_t cell,
                                              uint16_t columns_left, bool row_start,
                                              uint16_t last_cell) {
    cpu->x = cell;
    if (row_start) {
        LoadA16(cpu, columns_left);
    } else {
        cpu->accumulator = last_cell;
        SetNz16(cpu, columns_left);
    }
    return ExecutionHandoff(cpu, 0x838aacu);
}

/* Clear metatile numbers under the pending object; A = layer. */
Lufia2ExecutionResult Lufia2FieldClearObjectTileIds(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t cell;
    uint16_t columns_left;
    uint16_t rows_left;
    uint16_t last_cell = 0;
    unsigned cells_cleared = 0;

    WramWrite(wram, DP_CLEAR_LAYER_INDEX, A8(cpu));
    WramWrite(wram, DP_CLEAR_LAYER_INDEX + 1u, 0);
    LoadA8(cpu, WramRead(wram, WRAM_FIELD_PENDING_OBJECT_X));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, WramRead(wram, WRAM_FIELD_PENDING_OBJECT_Y));
    SimulateJsrFrame(memory, cpu, 0x8a7eu);
    (void)Lufia2MapCellOffset(memory, cpu);
    SimulateRtsFrame(memory, cpu);

    /* First cell of the rectangle in the selected layer. */
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    LoadX16(cpu, WramRead16(wram, DP_CLEAR_LAYER_INDEX));
    cpu->carry = 0;
    Add16Value(cpu, WramRead16At(wram, WRAM_FIELD_LAYER_CELL_BASE, cpu->x));
    TransferAToX(cpu);
    cell = cpu->x;

    /* Row skip in bytes; width read via the caller's DB. */
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, WramRead(wram, WRAM_FIELD_SECTION_WIDTH));
    cpu->carry = 1;
    Sbc8(cpu, WramRead(wram, WRAM_FIELD_OBJECT_WIDTH));
    SetAccumulatorWidth(cpu, 0);
    AslA16(cpu);
    WramWrite16(wram, DP_CLEAR_ROW_SKIP, cpu->accumulator);
    rows_left = WramRead(wram, WRAM_FIELD_OBJECT_HEIGHT);
    WramWrite16(wram, DP_CLEAR_ROWS_LEFT, rows_left);

    do {
        /* Clearing may overwrite the width; reread it per row. */
        bool row_start = true;

        columns_left = WramRead(wram, WRAM_FIELD_OBJECT_WIDTH);
        WramWrite16(wram, DP_CLEAR_COLUMNS_LEFT, columns_left);
        do {
            if (cells_cleared == CLEAR_CELL_LIMIT)
                return HandOffClearLoop(cpu, cell, columns_left, row_start, last_cell);
            row_start = false;
            ++cells_cleared;
            last_cell = WramRead16At(wram, MAP_LAYER_CELLS, cell) & CELL_ATTRIBUTE_MASK;
            WramWrite16At(wram, MAP_LAYER_CELLS, cell, last_cell);
            cell = (uint16_t)(cell + CELL_SIZE);
            columns_left = WramStep16(wram, DP_CLEAR_COLUMNS_LEFT, -1);
        } while (columns_left != 0);
        cell = AddKeepingFlags(cpu, cell, WramRead16(wram, DP_CLEAR_ROW_SKIP));
        rows_left = WramStep16(wram, DP_CLEAR_ROWS_LEFT, -1);
    } while (rows_left != 0);

    cpu->x = cell;
    SetNz16(cpu, rows_left);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x838ac8u);
}

/* Place the pending object and save the tiles beneath it. */
Lufia2ExecutionResult Lufia2FieldPlacePendingObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    uint8_t skip_tile_copy = 0;

    /* Publish the pending slot before saving the entry status. */
    OpSta(memory, cpu, OpDp(cpu, OBJECT_PENDING_SLOT));
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, OBJECT_PENDING_SLOT));
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_PENDING_RECORD_X & 0xffffu));
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_PENDING_RECORD_Y & 0xffffu));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_MASK));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_UPPER_ATTRIBUTE_MASK));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_BITS));
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_UPPER_ATTRIBUTE_BITS));
    if (!FieldTileChild(memory, cpu, child, context,
            0x83f80du, 0x83f88fu, 2u))
        return FieldTileUnwound(0x83f88fu);
    BitImmediate8(cpu, 0x40u);

    /* Preserve the tiles belonging to an object already covering this cell. */
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
        if (!cpu->zero)
            OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), 1);
        if (!FieldTileChild(memory, cpu, child, context,
                0x83fb9fu, 0x83f89cu, 2u))
            return FieldTileUnwound(0x83f89cu);
        OpTxy(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), 1);
        if (!FieldTileChild(memory, cpu, child, context,
                0x83fb9fu, 0x83f8a2u, 2u))
            return FieldTileUnwound(0x83f8a2u);
        if (!FieldTileChild(memory, cpu, child, context,
                0x83f85au, 0x83f8a5u, 2u))
            return FieldTileUnwound(0x83f8a5u);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu));
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu));
        OpSepWidths(cpu, 0x20u);
        OpTxy(cpu);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_OBJECT_SOURCE_X & 0xffffu));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_OBJECT_SOURCE_Y & 0xffffu));
        cpu->carry = 0;
        OpAdc(memory, cpu, OpAbs(cpu, WRAM_FIELD_OBJECT_HEIGHT & 0xffffu));
        OpDecA(cpu);
        if (!FieldTileChild(memory, cpu, child, context,
                0x83f9d9u, 0x83f8bdu, 2u))
            return FieldTileUnwound(0x83f8bdu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu));
        OpLda(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
        skip_tile_copy = cpu->zero;
        if (!skip_tile_copy) {
            OpStz(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
            OpSepWidths(cpu, 0x20u);
            OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), -1);
            OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), -1);
        }
    }

    /* Save the map cells, then replace only their tile ID bits. */
    if (!skip_tile_copy) {
        if (!FieldTileChild(memory, cpu, child, context,
                0x83f9d4u, 0x83f8d4u, 2u))
            return FieldTileUnwound(0x83f8d4u);
        OpTxy(cpu);
        OpLda(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_Y);
        if (!FieldTileChild(memory, cpu, child, context,
                0x83f9d9u, 0x83f8e1u, 2u))
            return FieldTileUnwound(0x83f8e1u);
        OpWriteX(memory, cpu, OpDp(cpu, OBJECT_PENDING_CELL), cpu->x);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, OBJECT_PENDING_SLOT));
        OpAslA(cpu);
        OpAslA(cpu);
        OpRepWidths(cpu, 0x20u);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_UPPER_ATTRIBUTE_MASK));
        OpTax(cpu);
        OpLda(memory, cpu, WRAM_FIELD_SECTION_WIDTH);
        OpAslA(cpu);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_MASK));
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu));
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_PENDING_CELL)));
        if (!FieldTileChild(memory, cpu, child, context,
                0x83f91fu, 0x83f8ffu, 2u))
            return FieldTileUnwound(0x83f8ffu);
        OpLda(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
        if (!cpu->zero) {
            OpTya(cpu);
            cpu->carry = 0;
            OpAdc(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_MASK));
            OpTay(cpu);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_UPPER_ATTRIBUTE_MASK)));
            OpLda(memory, cpu, OpAbsY(cpu, 0u));
            OpSta(memory, cpu, OpAbsX(cpu,
                (WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu) + 2u));
            OpLda(memory, cpu, OpDp(cpu, OBJECT_PENDING_CELL));
            cpu->carry = 0;
            OpAdc(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_MASK));
            OpTax(cpu);
            if (!FieldTileChild(memory, cpu, child, context,
                    0x83f91fu, 0x83f919u, 2u))
                return FieldTileUnwound(0x83f919u);
        }
    }
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83f91eu);
}
