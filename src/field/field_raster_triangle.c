#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/raster_edge.h"
#include "system/wram.h"

enum {
    TRIANGLE_STRIDE = 0x00u,
    TRIANGLE_FIRST_VERTEX = 0x02u,
    TRIANGLE_HORIZONTAL_SPAN = 0x04u,
    TRIANGLE_VERTICAL_SPAN = 0x05u,
    TRIANGLE_SECOND_VERTEX = 0x06u,
    TRIANGLE_POSITION = 0x09u,
    TRIANGLE_DIRECTION = 0x0cu,
    TRIANGLE_VERTEX_A = 0x16u,
    TRIANGLE_VERTEX_B = 0x18u,
    TRIANGLE_VERTEX_C = 0x1au,
    TRIANGLE_REMAINING = 0x22u,
    TRIANGLE_ROWS = 0xf1u,
    TRIANGLE_TOP = 0xf3u,
    TRIANGLE_BOTTOM = 0xf4u,
    TRIANGLE_VERTEX_ROW = 0xf6u,
    TRIANGLE_BOUNDS = WRAM_RASTER_EDGE_SAMPLE_BASE & 0xffffu
};

static void PrepareTriangleRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, (uint16_t)(cpu->x + 3u));
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE));
    OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 1u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 2u));
    OpLdx(cpu, (uint16_t)(cpu->x + 3u));
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_A + 1u));
    OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_B + 1u)));
    if (!cpu->carry)
        OpLda(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_B + 1u));
    OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_C + 1u)));
    if (!cpu->carry)
        OpLda(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_C + 1u));
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_BOTTOM));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_TOP)));
    OpIncA(cpu);
    OpOraValue(cpu, 0x80u);
    OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
    OpInx(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_ROWS), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_A + 1u));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_TOP)));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, TRIANGLE_ROWS));
    OpTax(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_ROW), cpu->x);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 1u));
}

static void OrderTriangleVertices(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_A));
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_POSITION));
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_B + 1u));
    OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_C + 1u)));
    const uint8_t first = cpu->carry ? TRIANGLE_VERTEX_C : TRIANGLE_VERTEX_B;
    const uint8_t second = cpu->carry ? TRIANGLE_VERTEX_B : TRIANGLE_VERTEX_C;
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, first)));
    OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_FIRST_VERTEX), cpu->y);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, second)));
    OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_SECOND_VERTEX), cpu->y);
}

static void PrepareFirstTriangleEdge(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_FIRST_VERTEX));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_A)));
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_HORIZONTAL_SPAN));
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_FIRST_VERTEX + 1u));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_A + 1u)));
    if (cpu->negative) {
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpIncA(cpu);
        OpLdy(cpu, 0xfffeu);
        OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE), cpu->y);
    } else {
        OpLdy(cpu, 2u);
        OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE), cpu->y);
        OpInx(cpu);
    }
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_VERTICAL_SPAN));
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_REMAINING));
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_POSITION));
    OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_DIRECTION));
}

static void PrepareSecondTriangleEdge(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_ROW)));
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_A));
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_POSITION));
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_SECOND_VERTEX));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_A)));
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_HORIZONTAL_SPAN));
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_SECOND_VERTEX + 1u));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_VERTEX_A + 1u)));
    if (cpu->negative) {
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpSta(memory, cpu, OpDp(cpu, TRIANGLE_VERTICAL_SPAN));
        OpSta(memory, cpu, OpDp(cpu, TRIANGLE_REMAINING));
        OpLdy(cpu, 0xfffeu);
        OpInx(cpu);
        OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE), cpu->y);
        OpLda(memory, cpu, OpDp(cpu, TRIANGLE_POSITION));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS)));
        if (!cpu->carry) {
            OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
            OpStepMem(memory, cpu, OpDp(cpu, TRIANGLE_REMAINING), 1);
        }
    } else {
        OpSta(memory, cpu, OpDp(cpu, TRIANGLE_VERTICAL_SPAN));
        OpSta(memory, cpu, OpDp(cpu, TRIANGLE_REMAINING));
        OpLdy(cpu, 2u);
        OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE), cpu->y);
        OpLda(memory, cpu, OpDp(cpu, TRIANGLE_POSITION));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS)));
        if (!cpu->carry)
            OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
    }
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_DIRECTION));
}

static void PrepareClosingTriangleEdge(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_SECOND_VERTEX + 1u));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_FIRST_VERTEX + 1u)));
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_VERTICAL_SPAN));
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_REMAINING));
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_FIRST_VERTEX));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_SECOND_VERTEX)));
    uint8_t start_vertex;
    if (cpu->zero || cpu->negative) {
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, TRIANGLE_HORIZONTAL_SPAN));
        OpLdy(cpu, 0xfffeu);
        OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE), cpu->y);
        start_vertex = TRIANGLE_SECOND_VERTEX;
    } else {
        OpSta(memory, cpu, OpDp(cpu, TRIANGLE_HORIZONTAL_SPAN));
        OpLdy(cpu, 2u);
        OpWriteX(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE), cpu->y);
        start_vertex = TRIANGLE_FIRST_VERTEX;
    }
    OpLda(memory, cpu, OpDp(cpu, start_vertex));
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_POSITION));
    OpLda(memory, cpu, OpDp(cpu, start_vertex + 1u));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TRIANGLE_TOP)));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, TRIANGLE_ROWS));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpInx(cpu);
    OpLda(memory, cpu, OpDp(cpu, TRIANGLE_POSITION));
    OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_DIRECTION));
}

static void FinishTriangleRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, TRIANGLE_ROWS)));
    OpDex(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x7fu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, TRIANGLE_ROWS));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    static const uint8_t tail[] = {0x0eu, 0xffu, 0u, 0u};
    for (unsigned field = 0u; field < sizeof tail; ++field) {
        OpLoadA(cpu, tail[field]);
        OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(TRIANGLE_BOUNDS + field)));
    }
}

static void ResolveTriangleCrossings(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, TRIANGLE_ROWS)));
    OpLda(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS - 1u));
    OpAndValue(cpu, 0x7fu);
    OpDecA(cpu);
    if (cpu->zero || cpu->negative)
        return;
    OpSta(memory, cpu, OpDp(cpu, TRIANGLE_REMAINING));
    do {
        OpLda(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 3u)));
        if (cpu->carry) {
            OpSta(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE));
            OpStz(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE + 1u));
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 3u));
            OpAndValue(cpu, 0xffu);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE));
            OpLsrA(cpu);
            OpSepWidths(cpu, 0x20u);
            OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS));
            OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 3u));
        }
        OpLda(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 1u));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 2u)));
        if (!cpu->carry) {
            OpSta(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE));
            OpStz(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE + 1u));
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 2u));
            OpAndValue(cpu, 0xffu);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, TRIANGLE_STRIDE));
            OpLsrA(cpu);
            OpSepWidths(cpu, 0x20u);
            OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 1u));
            OpSta(memory, cpu, OpAbsX(cpu, TRIANGLE_BOUNDS + 2u));
        }
        OpLdx(cpu, (uint16_t)(cpu->x + 2u));
        OpStepMem(memory, cpu, OpDp(cpu, TRIANGLE_REMAINING), -1);
    } while (!cpu->zero);
}

static bool TriangleEdge(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, 0x83fec4u, 2u, 0x83u)) {
        *result = (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x83u || cpu->data_bank != 0x7fu ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal) {
        *result = ExecutionHandoff(cpu,
            ((uint32_t)cpu->program_bank << 16) | ((site + 3u) & 0xffffu));
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2RasterizePreparedTriangle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x83u || cpu->data_bank != 0x7fu ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal || cpu->stack < 0x1f04u ||
        cpu->stack > 0x1ff8u)
        return ExecutionHandoff(cpu, 0x83fd0bu);
    Lufia2ExecutionResult result;
    PrepareTriangleRows(memory, cpu);
    OrderTriangleVertices(memory, cpu);
    PrepareFirstTriangleEdge(memory, cpu);
    if (!TriangleEdge(memory, cpu, child, context, 0x83fd9fu, &result))
        return result;
    PrepareSecondTriangleEdge(memory, cpu);
    if (!TriangleEdge(memory, cpu, child, context, 0x83fde7u, &result))
        return result;
    PrepareClosingTriangleEdge(memory, cpu);
    if (!TriangleEdge(memory, cpu, child, context, 0x83fe44u, &result))
        return result;
    FinishTriangleRows(memory, cpu);
    ResolveTriangleCrossings(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83fec3u);
}
