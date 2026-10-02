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

static void WriteObjectCellAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushY(memory, cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7f0001u));
    OpAndValue(cpu, 3u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
    OpTay(cpu);
    OpStz(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_CLASS_BITS));
    OpLda(memory, cpu,
        DirectLongIndirectY(memory, cpu, OBJECT_ATTRIBUTE_CATALOG));
    OpBitValue(cpu, 0xf0u);
    if (!cpu->zero) {
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_CLASS_BITS));
    } else {
        OpCmpValue(cpu, 8u);
        if (cpu->zero) {
            OpLoadA(cpu, 2u);
            OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_CLASS_BITS));
        } else {
            OpCmpValue(cpu, 9u);
            if (cpu->zero) {
                OpLoadA(cpu, 0x80u);
                OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_CLASS_BITS));
            }
        }
    }
    OpPullY(memory, cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7f0001u));
    OpAndValue(cpu, 0x0cu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_TILE_BITS));
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpAndValue(cpu, 0x45u);
    OpOra(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_TILE_BITS));
    OpOra(memory, cpu, OpDp(cpu, OBJECT_ATTRIBUTE_CLASS_BITS));
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpInx(cpu);
    OpInx(cpu);
    OpIny(cpu);
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
