#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    OBJECT_ATTRIBUTE_ROW_SKIP = 0x54,
    OBJECT_ATTRIBUTE_COLUMNS = 0x56,
    OBJECT_ATTRIBUTE_ROWS = 0x57,
    OBJECT_ATTRIBUTE_TILE_BITS = 0x58,
    OBJECT_ATTRIBUTE_CLASS_BITS = 0x59,
    OBJECT_ATTRIBUTE_SOURCE_ROW_SKIP = 0x5a,
    OBJECT_ATTRIBUTE_CATALOG = 0x60,
    OBJECT_ATTRIBUTE_CATALOG_BANK = 0x62,
};

static Lufia2ExecutionResult LocateObjectAttributeRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint8_t low, high;
    uint16_t frame;

    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    SimulateJsrFrame(memory, cpu, 0x8c9du);
    (void)Lufia2MapCellOffset(memory, cpu);
    low = Pull8(memory, cpu);
    high = Pull8(memory, cpu);
    frame = (uint16_t)(low | ((uint16_t)high << 8));
    if (frame != 0x8c9du)
        return ExecutionHandoff(cpu, 0x830000u | (uint16_t)(frame + 1u));
    OpWrite16(memory, OpDp(cpu, OBJECT_ATTRIBUTE_ROW_SKIP), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET)));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_ROW_SKIP));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_CELL_BASE));
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_ROW_SKIP));
    OpLsrA(cpu);
    /* LSR carry feeds the attribute-grid offset. */
    OpAdcValue(cpu, WRAM_FIELD_MAP_ATTRIBUTES & 0xffffu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_SECTION_WIDTH));
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_WIDTH));
    OpRepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_ROW_SKIP));
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_SOURCE_ROW_SKIP));
    OpLda(memory, cpu, WRAM_FIELD_METATILE_ATTRIBUTE_BASE);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_CATALOG));
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_CATALOG_BANK));
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_ROWS));
    return ExecutionReturned(0x838cdcu);
}

/* Tile words of the object layer in bank $7F: low ten bits pick the tile, bits
 * 10-11 are copied into the cell attribute. */
enum {
    OBJECT_TILE_PLANE = 0x7f0000,
    OBJECT_TILE_HIGH_BITS = 0x03,
    OBJECT_TILE_PALETTE_MASK = 0x0c,
    OBJECT_ATTRIBUTE_KEEP_MASK = 0x45,
    OBJECT_CATALOG_HIGH_NIBBLE = 0xf0,
};

/* Class bits of a cell attribute from the metatile's catalog entry. */
static uint8_t ObjectCellClass(uint8_t entry) {
    if (entry & OBJECT_CATALOG_HIGH_NIBBLE)
        return 0x08;
    if (entry == 8u)
        return 0x02;
    if (entry == 9u)
        return 0x80;
    return 0x00;
}

/* Rebuilds the attribute byte at DB:Y for the tile word at $7F:X, then moves
 * both on by one cell. The remaining register state is that of the original
 * routine: the accumulator keeps the two high tile bits in its high byte. */
static void WriteObjectCellAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t tile_word = LongIndexedAddress(OBJECT_TILE_PLANE, cpu->x);
    const uint16_t cell = cpu->y;
    uint8_t tile_high;
    uint8_t tile_low;
    uint16_t tile;
    uint8_t entry_class;
    uint8_t attribute;

    PushY(memory, cpu);
    tile_high = Read8(memory, tile_word + 1u);
    tile_low = Read8(memory, tile_word);
    tile = (uint16_t)(((tile_high & OBJECT_TILE_HIGH_BITS) << 8) | tile_low);
    cpu->y = tile;
    Write8(memory, DirectAddress(cpu, OBJECT_ATTRIBUTE_CLASS_BITS), 0u);
    entry_class = ObjectCellClass(
        Read8(memory, DirectLongIndirectY(memory, cpu, OBJECT_ATTRIBUTE_CATALOG)));
    if (entry_class)
        Write8(memory, DirectAddress(cpu, OBJECT_ATTRIBUTE_CLASS_BITS), entry_class);

    cpu->y = PullIndexValue(memory, cpu);
    Write8(memory, DirectAddress(cpu, OBJECT_ATTRIBUTE_TILE_BITS),
           (uint8_t)((Read8(memory, tile_word + 1u) & OBJECT_TILE_PALETTE_MASK) << 2));
    attribute = Read8(memory, AbsoluteIndexedAddress(cpu, 0u, cell));
    attribute &= OBJECT_ATTRIBUTE_KEEP_MASK;
    attribute |= Read8(memory, DirectAddress(cpu, OBJECT_ATTRIBUTE_TILE_BITS));
    attribute |= Read8(memory, DirectAddress(cpu, OBJECT_ATTRIBUTE_CLASS_BITS));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0u, cell), attribute);

    cpu->accumulator =
        (uint16_t)(((tile_high & OBJECT_TILE_HIGH_BITS) << 8) | attribute);
    cpu->x = (uint16_t)(cpu->x + 2u);
    cpu->y = (uint16_t)(cell + 1u);
    SetNz16(cpu, cpu->y);
    cpu->carry = 0;
}

Lufia2ExecutionResult Lufia2FieldRefreshObjectAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned cells = 0;
    Lufia2ExecutionResult result;

    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    result = LocateObjectAttributeRegion(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    for (;;) {
        OpLda(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_COLUMNS));
        for (;;) {
            if (cells == 262144u)
                return ExecutionHandoff(cpu, 0x838ce2u);
            ++cells;
            WriteObjectCellAttributes(memory, cpu);
            OpStepMem(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_COLUMNS), -1);
            if (cpu->zero)
                break;
        }
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_SOURCE_ROW_SKIP));
        OpTax(cpu);
        OpTya(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_ROW_SKIP));
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_ROWS), -1);
        if (cpu->zero)
            break;
    }
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x838d41u);
}
