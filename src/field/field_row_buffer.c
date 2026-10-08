#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    DP_ROW_BUFFER_LEFT = 0x26u,
    DP_ROW_BUFFER_BASE = 0x2au,
    DP_ROW_BUFFER_UPPER = 0x5du,
    DP_ROW_BUFFER_LOWER = 0x60u,
    DP_ROW_METATILE_BASE = 0x65u,
    DP_ROW_WIDTH = 0x83u,
    DP_ROW_CELL_X = 0x87u,
    ROW_METATILE_COUNT = 16u,
    ROW_BUFFER_BYTES = 0x40u,
    ROW_COLUMN_MASK = 0x3fu,
    ROW_SHARED_METATILE = 0x3000u,
    ROW_METATILE_MASK = 0x3ffu
};

static void AdvanceBufferColumn(Lufia2CpuState *cpu) {
    OpTya(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpAndValue(cpu, ROW_COLUMN_MASK);
    OpTay(cpu);
}

static void SelectRowMetatile(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, ROW_SHARED_METATILE);
    OpCmpValue(cpu, ROW_SHARED_METATILE);
    if (cpu->zero)
        OpLdx(cpu, OpRead16(memory, OpAbs(cpu, WRAM_FIELD_LAYER_CELL_BASE & 0xffffu)));
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, ROW_METATILE_MASK);
    for (unsigned shift = 0; shift < 3u; ++shift)
        OpAslA(cpu);
    OpAdcValue(cpu, Read16Direct(memory, cpu, DP_ROW_METATILE_BASE));
    OpTax(cpu);
}

static void WriteRowMetatile(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t source_offsets[] = {0u, 2u, 4u, 6u};
    static const uint8_t buffers[] = {
        DP_ROW_BUFFER_UPPER, DP_ROW_BUFFER_LOWER,
        DP_ROW_BUFFER_UPPER, DP_ROW_BUFFER_LOWER
    };
    for (unsigned tile = 0; tile < 4u; ++tile) {
        if (tile == 2u)
            AdvanceBufferColumn(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, source_offsets[tile]));
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, buffers[tile]));
    }
}

Lufia2ExecutionResult Lufia2FieldRenderRowBuffers(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x80f6c6u);
    OpLoadA(cpu, ROW_METATILE_COUNT);
    OpSta(memory, cpu, OpDp(cpu, DP_ROW_BUFFER_LEFT));
    OpTya(cpu);
    OpAndValue(cpu, 0xffc0u);
    cpu->carry = 0u;
    OpAdcValue(cpu, Read16Direct(memory, cpu, DP_ROW_BUFFER_BASE));
    OpSta(memory, cpu, OpDp(cpu, DP_ROW_BUFFER_UPPER));
    cpu->carry = 0u;
    OpAdcValue(cpu, ROW_BUFFER_BYTES);
    OpSta(memory, cpu, OpDp(cpu, DP_ROW_BUFFER_LOWER));
    OpTya(cpu);
    OpAndValue(cpu, ROW_COLUMN_MASK);
    OpTay(cpu);
    do {
        PushIndex(memory, cpu);
        SelectRowMetatile(memory, cpu);
        WriteRowMetatile(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        AdvanceBufferColumn(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_ROW_CELL_X));
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_ROW_CELL_X));
        uint8_t wrap = cpu->zero;
        if (!wrap) {
            OpCmpValue(cpu, Read16Direct(memory, cpu, DP_ROW_WIDTH));
            if (cpu->carry) {
                OpStz(memory, cpu, OpDp(cpu, DP_ROW_CELL_X));
                wrap = 1u;
            }
        }
        if (wrap && !CallChildWithFrame(memory, cpu, child, context,
                0x80f72cu, 0x80f6aau, 2u, 0x80u)) {
            Lufia2ExecutionResult result = ExecutionReturned(0x80f72cu);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        OpStepMem(memory, cpu, OpDp(cpu, DP_ROW_BUFFER_LEFT), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x80f733u);
}
