#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/raster_edge.h"

enum {
    EDGE_OUTPUT_STRIDE = 0x00u,
    EDGE_HORIZONTAL_SPAN = 0x04u,
    EDGE_VERTICAL_SPAN = 0x05u,
    EDGE_POSITION = 0x09u,
    EDGE_DIRECTION = 0x0cu,
    EDGE_REMAINING = 0x22u,
    EDGE_SAMPLE_TABLE = 0x7f7bb3u
};

static void AdvanceEdgeOutput(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, EDGE_OUTPUT_STRIDE));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
}

static void DivideEdgeStep(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, cpu->accumulator ^ 0xffu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVL));
    OpStz(memory, cpu, OpAbs(cpu, SNES_WRDIVH));
    OpLda(memory, cpu, OpDp(cpu, EDGE_VERTICAL_SPAN));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVB));
    AdvanceEdgeOutput(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, EDGE_DIRECTION));
    if (cpu->negative) {
        OpLda(memory, cpu, OpDp(cpu, EDGE_POSITION));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SNES_RDDIVL)));
        OpDecA(cpu);
    } else {
        OpLda(memory, cpu, OpAbs(cpu, SNES_RDDIVL));
        OpIncA(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, EDGE_POSITION));
    }
    OpSta(memory, cpu, OpDp(cpu, EDGE_POSITION));
    OpLda(memory, cpu, OpDp(cpu, EDGE_VERTICAL_SPAN));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
    PushAccumulator8(memory, cpu);
}

Lufia2ExecutionResult Lufia2RasterizeEdge(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x83fec4u);
    OpLoadA(cpu, 0u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, EDGE_VERTICAL_SPAN));
    OpLsrA(cpu);
    for (;;) {
        OpStepMem(memory, cpu, OpDp(cpu, EDGE_REMAINING), -1);
        if (cpu->negative)
            break;
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, EDGE_HORIZONTAL_SPAN)));
        if (cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, EDGE_POSITION));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, EDGE_DIRECTION));
            OpSta(memory, cpu, OpDp(cpu, EDGE_POSITION));
            OpLda(memory, cpu, OpDp(cpu, EDGE_VERTICAL_SPAN));
            PushAccumulator8(memory, cpu);
            AdvanceEdgeOutput(memory, cpu);
        } else if (cpu->negative) {
            DivideEdgeStep(memory, cpu);
        } else {
            PushAccumulator8(memory, cpu);
            AdvanceEdgeOutput(memory, cpu);
        }
        OpLda(memory, cpu, OpDp(cpu, EDGE_POSITION));
        OpSta(memory, cpu, OpLongX(cpu, EDGE_SAMPLE_TABLE));
        LoadA8(cpu, Pull8(memory, cpu));
    }
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x83ff30u);
}
