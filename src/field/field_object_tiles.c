/* Object attributes, saved tiles and redraw queue requests. */

#include "core/cpu_ops.h"
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
