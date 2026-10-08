#include "core/cpu_ops.h"
#include "system/wram.h"
#include "core/child_call.h"
#include "lufia2/world_map.h"

enum {
    DP_STREAM_SUBCELL = 0x00u,
    DP_STREAM_BLOCK = 0x08u,
    DP_STREAM_CELL = 0x0bu,
    DP_STREAM_PARITY = 0x13u,
    DP_STREAM_REMAINING = 0x26u,
    DP_STREAM_X = 0x58u,
    DP_STREAM_Y = 0x5au,
    DP_STREAM_LOW_BLOCKS = 0xdfu,
    DP_STREAM_HIGH_BLOCKS = 0xe3u,
    DP_STREAM_TOP_LEFT = 0xe7u,
    DP_STREAM_TOP_RIGHT = 0xeau,
    DP_STREAM_BOTTOM_RIGHT = 0xedu,
    STREAM_ROW_ADDRESS_LONG = WRAM_WORLD_MAP_ROW_STAGE & 0xffffu,
    STREAM_COLUMN_ADDRESS_LONG = WRAM_WORLD_MAP_COLUMN_STAGE & 0xffffu,
    STREAM_COLUMN_TOP = 0xdf00u,
    STREAM_COLUMN_BOTTOM = 0xdf80u,
    STREAM_CELL_COUNT = 64u
};

static uint8_t StreamArithmeticEntry(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x86u && !cpu->accumulator_is_8_bit &&
        !cpu->decimal;
}

Lufia2ExecutionResult Lufia2WorldMapBuildCellOffset(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!StreamArithmeticEntry(cpu))
        return ExecutionHandoff(cpu, 0x86adeeu);
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_Y));
    OpAndValue(cpu, 0x3fu);
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_CELL));
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_X));
    OpAndValue(cpu, 0x3fu);
    OpAdcValue(cpu, OpRead16(memory, OpDp(cpu, DP_STREAM_CELL)));
    OpAslA(cpu);
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_CELL));
    return ExecutionReturned(0x86ae04u);
}

Lufia2ExecutionResult Lufia2WorldMapBuildBlockPointer(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!StreamArithmeticEntry(cpu))
        return ExecutionHandoff(cpu, 0x86ae05u);
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_X));
    OpAndValue(cpu, 0xffu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_BLOCK));
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_Y));
    OpAndValue(cpu, 0xffu);
    OpLsrA(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    OpAdcValue(cpu, OpRead16(memory, OpDp(cpu, DP_STREAM_BLOCK)));
    OpAslA(cpu);
    OpAdcValue(cpu, 0x4040u);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_BLOCK));
    return ExecutionReturned(0x86ae1du);
}

static uint8_t StreamEntry(const Lufia2CpuState *cpu, Lufia2PushedChildCall child) {
    return cpu->program_bank == 0x86u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu && child;
}

static Lufia2ExecutionResult StreamUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void SelectStreamMetatile(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t block = Read16Direct(memory, cpu, DP_STREAM_BLOCK);
    OpLda(memory, cpu, OpAbs(cpu, block));
    for (unsigned shift = 0; shift < 3u; ++shift)
        OpAslA(cpu);
    OpAdcValue(cpu, Read16Direct(memory, cpu, DP_STREAM_SUBCELL));
    OpTay(cpu);
    const uint8_t table = cpu->negative ? DP_STREAM_HIGH_BLOCKS : DP_STREAM_LOW_BLOCKS;
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, table));
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_SUBCELL));
    OpAslA(cpu);
    OpAslA(cpu);
    OpAdcValue(cpu, Read16Direct(memory, cpu, DP_STREAM_SUBCELL));
    OpTay(cpu);
    OpLdx(cpu, Read16Direct(memory, cpu, DP_STREAM_CELL));
}

static void WriteStreamRowCell(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_STREAM_TOP_LEFT));
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, 0x80u));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_STREAM_TOP_RIGHT));
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_STREAM_BOTTOM_RIGHT));
    OpSta(memory, cpu, OpAbsX(cpu, 0x81u));
    OpRepWidths(cpu, 0x20u);
}

static void WriteStreamColumnCell(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_STREAM_TOP_LEFT));
    OpSta(memory, cpu, OpAbsX(cpu, STREAM_COLUMN_TOP));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, DP_STREAM_TOP_RIGHT));
    OpSta(memory, cpu, OpAbsX(cpu, STREAM_COLUMN_BOTTOM));
}

static void FinishStream(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_X));
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2WorldMapStreamRow(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!StreamEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86ac6cu);
    Push8(memory, cpu, cpu->data_bank);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x86ac73u, 0x86adeeu, 2u, 0x86u))
        return StreamUnwound(0x86ac73u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x86ac76u, 0x86ae05u, 2u, 0x86u))
        return StreamUnwound(0x86ac76u);
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_CELL));
    OpAndValue(cpu, 0xff80u);
    OpSta(memory, cpu, STREAM_ROW_ADDRESS_LONG);
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_Y));
    OpAndValue(cpu, 1u);
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_PARITY));
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_X));
    PushAccumulator16(memory, cpu);
    OpLoadA(cpu, STREAM_CELL_COUNT);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_REMAINING));
    do {
        OpLda(memory, cpu, OpDp(cpu, DP_STREAM_X));
        OpAndValue(cpu, 1u);
        OpAslA(cpu);
        OpAdcValue(cpu, Read16Direct(memory, cpu, DP_STREAM_PARITY));
        OpSta(memory, cpu, OpDp(cpu, DP_STREAM_SUBCELL));
        SelectStreamMetatile(memory, cpu);
        WriteStreamRowCell(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_STREAM_CELL));
        OpIncA(cpu);
        OpIncA(cpu);
        cpu->zero = (cpu->accumulator & 0x7fu) == 0u;
        if (cpu->zero) {
            cpu->carry = 1u;
            OpSbcValue(cpu, 0x80u);
        }
        OpSta(memory, cpu, OpDp(cpu, DP_STREAM_CELL));
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_STREAM_X), 1);
        OpRepWidths(cpu, 0x20u);
        if (cpu->zero) {
            if (!CallChildWithFrame(memory, cpu, child, context, 0x86ace4u, 0x86ae05u, 2u, 0x86u))
                return StreamUnwound(0x86ace4u);
        } else {
            OpLda(memory, cpu, OpDp(cpu, DP_STREAM_X));
            OpLsrA(cpu);
            if (!cpu->carry) {
                OpStepMem(memory, cpu, OpDp(cpu, DP_STREAM_BLOCK), 1);
                OpStepMem(memory, cpu, OpDp(cpu, DP_STREAM_BLOCK), 1);
            }
        }
        OpStepMem(memory, cpu, OpDp(cpu, DP_STREAM_REMAINING), -1);
    } while (!cpu->zero);
    FinishStream(memory, cpu);
    return ExecutionReturned(0x86acfdu);
}

Lufia2ExecutionResult Lufia2WorldMapStreamColumn(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (!StreamEntry(cpu, child))
        return ExecutionHandoff(cpu, 0x86acfeu);
    Push8(memory, cpu, cpu->data_bank);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x86ad05u, 0x86adeeu, 2u, 0x86u))
        return StreamUnwound(0x86ad05u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x86ad08u, 0x86ae05u, 2u, 0x86u))
        return StreamUnwound(0x86ad08u);
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_CELL));
    OpAndValue(cpu, 0xc07fu);
    OpSta(memory, cpu, STREAM_COLUMN_ADDRESS_LONG);
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_X));
    OpAndValue(cpu, 1u);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_PARITY));
    OpLda(memory, cpu, OpDp(cpu, DP_STREAM_Y));
    PushAccumulator16(memory, cpu);
    OpAndValue(cpu, 0x3fu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_CELL));
    OpLoadA(cpu, STREAM_CELL_COUNT);
    OpSta(memory, cpu, OpDp(cpu, DP_STREAM_REMAINING));
    do {
        OpLda(memory, cpu, OpDp(cpu, DP_STREAM_Y));
        OpAndValue(cpu, 1u);
        OpAslA(cpu);
        OpAslA(cpu);
        OpAdcValue(cpu, Read16Direct(memory, cpu, DP_STREAM_PARITY));
        OpSta(memory, cpu, OpDp(cpu, DP_STREAM_SUBCELL));
        SelectStreamMetatile(memory, cpu);
        WriteStreamColumnCell(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_STREAM_CELL));
        OpIncA(cpu);
        OpIncA(cpu);
        OpAndValue(cpu, 0x7fu);
        OpSta(memory, cpu, OpDp(cpu, DP_STREAM_CELL));
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_STREAM_Y), 1);
        if (cpu->zero) {
            OpRepWidths(cpu, 0x20u);
            if (!CallChildWithFrame(memory, cpu, child, context, 0x86ad72u, 0x86ae05u, 2u, 0x86u))
                return StreamUnwound(0x86ad72u);
        } else {
            OpLda(memory, cpu, OpDp(cpu, DP_STREAM_Y));
            OpLsrA(cpu);
            if (!cpu->carry)
                OpStepMem(memory, cpu, OpDp(cpu, DP_STREAM_BLOCK + 1u), 1);
        }
        OpRepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_STREAM_REMAINING), -1);
    } while (!cpu->zero);
    FinishStream(memory, cpu);
    return ExecutionReturned(0x86ad81u);
}
