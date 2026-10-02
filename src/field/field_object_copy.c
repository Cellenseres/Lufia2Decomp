/* Copy selected map-tile bits from an object's source rectangle. */

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

static void ObjectCopyTileBits(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t loop_pc) {
    if (loop_pc == 0x838c29u) {
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpAndValue(cpu, 0x3000u);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_TILE_BITS));
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, 0xcfffu);
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        OpAndValue(cpu, loop_pc == 0x838c0cu ? 0x03ffu : 0x0c00u);
        OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_TILE_BITS));
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        OpAndValue(cpu, loop_pc == 0x838c0cu ? 0xfc00u : 0xf3ffu);
    }
    OpOra(memory, cpu, OpDp(cpu, OBJECT_COPY_TILE_BITS));
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
}

Lufia2ExecutionResult Lufia2FieldCopyObjectTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned copied = 0;
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSetDataBank(memory, cpu, 0x7fu);
    ObjectCopyRectangleOffsets(memory, cpu);
    OpLdx(cpu, 0u);

    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, (WRAM_FIELD_LAYER_SECTION_WORD & 0xffffu)));
        if (cpu->zero) {
            OpLda(memory, cpu, OpLongX(cpu, OBJECT_COPY_LAYER_MASKS));
            OpAndValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_FLAGS));
            if (!cpu->zero) {
                OpWriteX(memory, cpu, OpDp(cpu, OBJECT_COPY_LAYER), cpu->x);
                OpStz(memory, cpu, OpDp(cpu, OBJECT_COPY_MODE));
                OpStz(memory, cpu, OpDp(cpu, OBJECT_COPY_MODE + 1u));
                OpCmp(memory, cpu, OpLongX(cpu, OBJECT_COPY_LAYER_MASKS));
                if (!cpu->zero) {
                    OpStepMem(memory, cpu, OpDp(cpu, OBJECT_COPY_MODE), -1);
                    OpBitValue(cpu, 0x000fu);
                    if (cpu->zero)
                        OpStepMem(memory, cpu,
                            OpDp(cpu, OBJECT_COPY_MODE + 1u), -1);
                }
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

                for (;;) {
                    OpLda(memory, cpu, OpAbs(cpu, (WRAM_FIELD_OBJECT_WIDTH & 0xffffu)));
                    OpAndValue(cpu, 0x00ffu);
                    OpSta(memory, cpu, OpDp(cpu, OBJECT_COPY_COLUMNS_LEFT));
                    OpLda(memory, cpu, OpDp(cpu, OBJECT_COPY_MODE));
                    const uint32_t loop_pc = cpu->zero ? 0x838c29u :
                        cpu->negative ? 0x838c46u : 0x838c0cu;
                    for (;;) {
                        if (copied == OBJECT_COPY_TILE_LIMIT)
                            return ExecutionHandoff(cpu, loop_pc);
                        ObjectCopyTileBits(memory, cpu, loop_pc);
                        ++copied;
                        OpStepMem(memory, cpu,
                            OpDp(cpu, OBJECT_COPY_COLUMNS_LEFT), -1);
                        if (cpu->zero)
                            break;
                        OpInx(cpu);
                        OpInx(cpu);
                        OpIny(cpu);
                        OpIny(cpu);
                    }
                    OpStepMem(memory, cpu, OpDp(cpu, OBJECT_COPY_ROWS_LEFT), -1);
                    if (cpu->zero)
                        break;
                    cpu->carry = 0;
                    OpTya(cpu);
                    OpAdc(memory, cpu, OpDp(cpu, OBJECT_COPY_ROW_ADVANCE));
                    OpTay(cpu);
                    OpTxa(cpu);
                    OpAdc(memory, cpu, OpDp(cpu, OBJECT_COPY_ROW_ADVANCE));
                    OpTax(cpu);
                }
                OpSepWidths(cpu, 0x20u);
                OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, OBJECT_COPY_LAYER)));
            }
        }
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 8u);
        if (cpu->zero)
            break;
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x838c81u);
}
