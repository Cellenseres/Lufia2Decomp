/* Copy selected map-tile bits from an object's source rectangle. */

#include <stdbool.h>

#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    OBJECT_COPY_ROW_WIDTH = 0x54,
    OBJECT_COPY_TILE_BITS = 0x54,
    OBJECT_COPY_ROWS_LEFT = 0x56,
    OBJECT_COPY_COLUMNS_LEFT = 0x58,
    OBJECT_COPY_ROW_ADVANCE = 0x5a,
    OBJECT_COPY_SOURCE_OFFSET = 0x5d,
    OBJECT_COPY_DESTINATION_OFFSET = 0x60,
    OBJECT_COPY_LAYER = 0x63,
    OBJECT_COPY_MODE = 0x65,
    OBJECT_COPY_LAYER_MASKS = 0x838c82,
    OBJECT_COPY_TILE_LIMIT = 262144,
};

static void ObjectCopyRectangleOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_OBJECT_WIDTH & 0xffffu)));
    OpRepWidths(cpu, 0x20u);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_ROW_WIDTH));
    OpLda(memory, cpu, WRAM_FIELD_SECTION_WIDTH);
    OpAslA(cpu);
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu,
        OpDp(cpu, OBJECT_COPY_ROW_WIDTH)));
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_ROW_ADVANCE));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, WRAM_FIELD_SECTION_WIDTH);
    OpSta(memory, cpu, 0x004202u);
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_OBJECT_SOURCE_Y & 0xffffu)));
    OpSta(memory, cpu, 0x004203u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_OBJECT_SOURCE_X & 0xffffu)));
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 0;
    OpAdc(memory, cpu, 0x004216u);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_SOURCE_OFFSET));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_PENDING_OBJECT_Y & 0xffffu)));
    OpSta(memory, cpu, 0x004203u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_PENDING_OBJECT_X & 0xffffu)));
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 0;
    OpAdc(memory, cpu, 0x004216u);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_DESTINATION_OFFSET));
    OpSepWidths(cpu, 0x20u);
}

/* Which tile word bits a layer copy moves. */
typedef enum {
    OBJECT_COPY_ALL_BUT_PRIORITY, /* everything except bits 12-13 */
    OBJECT_COPY_TILE_NUMBER,      /* bits 0-9 */
    OBJECT_COPY_ATTRIBUTE_BITS,   /* bits 10-11 */
} ObjectCopyKind;

enum {
    OBJECT_TILE_NUMBER_MASK = 0x03ff,
    OBJECT_TILE_ATTRIBUTE_MASK = 0x0c00,
    OBJECT_TILE_PRIORITY_MASK = 0x3000,
};

/* ROM continuation at the start of each copy loop. */
static uint32_t ObjectCopyLoopPc(ObjectCopyKind kind) {
    switch (kind) {
    case OBJECT_COPY_ALL_BUT_PRIORITY:
        return 0x838c29u;
    case OBJECT_COPY_ATTRIBUTE_BITS:
        return 0x838c46u;
    default:
        return 0x838c0cu;
    }
}

/* Copy kind from the sign of DP $65. */
static ObjectCopyKind ObjectCopySelectKind(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, OBJECT_COPY_MODE));
    if (cpu->zero)
        return OBJECT_COPY_ALL_BUT_PRIORITY;
    return cpu->negative ? OBJECT_COPY_ATTRIBUTE_BITS : OBJECT_COPY_TILE_NUMBER;
}

/* Move the selected bits from DB:X into DB:Y. */
static void ObjectCopyTileBits(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               ObjectCopyKind kind) {
    switch (kind) {
    case OBJECT_COPY_ALL_BUT_PRIORITY:
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpAndValue(cpu, OBJECT_TILE_PRIORITY_MASK);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_TILE_BITS));
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, (uint16_t)~OBJECT_TILE_PRIORITY_MASK);
        break;
    case OBJECT_COPY_TILE_NUMBER:
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, OBJECT_TILE_NUMBER_MASK);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_TILE_BITS));
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpAndValue(cpu, (uint16_t)~OBJECT_TILE_NUMBER_MASK);
        break;
    default:
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, OBJECT_TILE_ATTRIBUTE_MASK);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_TILE_BITS));
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpAndValue(cpu, (uint16_t)~OBJECT_TILE_ATTRIBUTE_MASK);
        break;
    }
    OpOra(memory, cpu, OpDp(cpu, OBJECT_COPY_TILE_BITS));
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
}

/* Layer copies when its section is zero and flags match. */
static bool ObjectCopyLayerSelected(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, (WRAM_FIELD_LAYER_SECTION_WORD & 0xffffu)));
    if (!cpu->zero)
        return false;
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_COPY_LAYER_MASKS));
    OpAndValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_FLAGS));
    return !cpu->zero;
}

/* DP $65: zero, $00FF or $FFFF from the matching flags. */
static void ObjectCopySetMode(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpWriteX(memory, cpu, OpDp(cpu, OBJECT_COPY_LAYER), cpu->x);
    OpStz(memory, cpu, OpDp(cpu, OBJECT_COPY_MODE));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_COPY_MODE + 1u));
    OpCmp(memory, cpu, OpLongX(cpu, OBJECT_COPY_LAYER_MASKS));
    if (cpu->zero)
        return;
    OpStepMem(memory, cpu, OpDp(cpu, OBJECT_COPY_MODE), -1);
    OpBitValue(cpu, 0x000fu);
    if (cpu->zero)
        OpStepMem(memory, cpu, OpDp(cpu, OBJECT_COPY_MODE + 1u), -1);
}

/* Y at destination, X at the layer's source rectangle. */
static void ObjectCopySetCursors(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_OBJECT_HEIGHT & 0xffffu)));
    OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_ROWS_LEFT));
    OpStz(memory, cpu, OpDp(cpu, OBJECT_COPY_ROWS_LEFT + 1u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, (WRAM_FIELD_LAYER_CELL_BASE & 0xffffu)));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, OBJECT_COPY_DESTINATION_OFFSET));
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, (WRAM_FIELD_LAYER_CELL_BASE & 0xffffu)));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, OBJECT_COPY_SOURCE_OFFSET));
    OpTax(cpu);
}

/* Copy the rectangle row by row within the tile budget. */
static Lufia2ExecutionResult ObjectCopyRows(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu, unsigned *copied) {
    for (;;) {
        ObjectCopyKind kind;

        OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_OBJECT_WIDTH & 0xffffu)));
        OpAndValue(cpu, 0x00ffu);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_COLUMNS_LEFT));
        kind = ObjectCopySelectKind(memory, cpu);
        for (;;) {
            if (*copied == OBJECT_COPY_TILE_LIMIT)
                return ExecutionHandoff(cpu, ObjectCopyLoopPc(kind));
            ObjectCopyTileBits(memory, cpu, kind);
            ++*copied;
            OpStepMem(memory, cpu, OpDp(cpu, OBJECT_COPY_COLUMNS_LEFT), -1);
            if (cpu->zero)
                break;
            OpInx(cpu);
            OpInx(cpu);
            OpIny(cpu);
            OpIny(cpu);
        }
        OpStepMem(memory, cpu, OpDp(cpu, OBJECT_COPY_ROWS_LEFT), -1);
        if (cpu->zero)
            return ExecutionReturned(0);
        /* The second add takes the first add's carry. */
        cpu->carry = 0;
        OpTya(cpu);
        OpAdc(memory, cpu, OpDp(cpu, OBJECT_COPY_ROW_ADVANCE));
        OpTay(cpu);
        OpTxa(cpu);
        OpAdc(memory, cpu, OpDp(cpu, OBJECT_COPY_ROW_ADVANCE));
        OpTax(cpu);
    }
}

Lufia2ExecutionResult Lufia2FieldCopyObjectTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned copied = 0;

    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7fu);
    ObjectCopyRectangleOffsets(memory, cpu);
    OpLdx(cpu, 0u);

    do {
        if (ObjectCopyLayerSelected(memory, cpu)) {
            Lufia2ExecutionResult result;

            ObjectCopySetMode(memory, cpu);
            ObjectCopySetCursors(memory, cpu);
            result = ObjectCopyRows(memory, cpu, &copied);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
            OpSepWidths(cpu, 0x20u);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_COPY_LAYER)));
        }
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 8u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x838c81u);
}
