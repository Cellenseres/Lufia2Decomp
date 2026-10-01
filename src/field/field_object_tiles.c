/* Object attributes, saved tiles and redraw queue requests. */

#include "core/cpu_ops.h"
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

static Lufia2ExecutionResult FieldTileUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

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

Lufia2ExecutionResult Lufia2FieldCopyCellTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    enum { CELL_TILE = 0x5au };

    OpLda(memory, cpu, OpAbsX(cpu, 0u));                         /* F91F */
    OpAndValue(cpu, 0x03ffu);
    OpSta(memory, cpu, OpDp(cpu, CELL_TILE));
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpAndValue(cpu, 0xfc00u);
    OpOra(memory, cpu, OpDp(cpu, CELL_TILE));
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    return ExecutionReturned(0x83f932u);
}

Lufia2ExecutionResult Lufia2FieldClearObjectTileIds(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    enum {
        OBJECT_LAYER_INDEX = 0x54,
        OBJECT_ROW_SKIP = 0x54,
        OBJECT_COLUMNS_REMAINING = 0x56,
        OBJECT_ROWS_REMAINING = 0x58,
    };
    unsigned cells_cleared = 0;

    OpSta(memory, cpu, OpDp(cpu, OBJECT_LAYER_INDEX));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_LAYER_INDEX + 1u));
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    SimulateJsrFrame(memory, cpu, 0x8a7eu);
    (void)Lufia2MapCellOffset(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_LAYER_INDEX)));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_CELL_BASE));
    OpTax(cpu);

    /* The row skip reads the caller's data bank, unlike the cell multiplier. */
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_SECTION_WIDTH & 0xffffu));
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_WIDTH));
    OpRepWidths(cpu, 0x20u);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ROW_SKIP));
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ROWS_REMAINING));
    do {
        OpLda(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_COLUMNS_REMAINING));
        do {
            /* Leave long or self-modifying rectangles at the original loop PC. */
            if (cells_cleared == 262144u)
                return ExecutionHandoff(cpu, 0x838aacu);
            ++cells_cleared;
            OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
            OpAndValue(cpu, 0xfc00u);
            OpSta(memory, cpu, OpLongX(cpu, 0x7f0000u));
            OpInx(cpu);
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, OBJECT_COLUMNS_REMAINING), -1);
        } while (!cpu->zero);
        OpTxa(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, OBJECT_ROW_SKIP));
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, OBJECT_ROWS_REMAINING), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x838ac8u);
}

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
