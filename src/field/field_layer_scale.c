/* Field layer coordinate scaling ($83:8DDA-$83:8E43). */

#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    LAYER_SCALE_COORDINATE = 0x54,
    LAYER_SCROLL_CELL_X = 0x56,
    LAYER_SCROLL_CELL_Y = 0x58,
};

Lufia2ExecutionResult Lufia2FieldLayerScaleMode(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SECTION_WORD + 1u));
    OpAndValue(cpu, 0x0fu);
    return ExecutionReturned(0x838e08u);
}

Lufia2ExecutionResult Lufia2FieldPrepareCoordinateScale(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    /* Both Y indices retain the preceding accumulator high byte. */
    OpAndValue(cpu, 0x07u);
    OpTay(cpu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, 0x8e44u));
    OpTay(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, LAYER_SCALE_COORDINATE));
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    Compare16(cpu, cpu->y, 0u);
    cpu->carry = 0;
    if (cpu->zero) {
        OpTya(cpu);
        cpu->carry = 1;
    }
    return ExecutionReturned(0x838e43u);
}

/* $83:8E0B / $83:8E1C: shifts A right (arithmetic) or left by the count that
 * PrepareCoordinateScale derives from the layer scale mode. */
static Lufia2ExecutionResult ShiftCoordinate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t right) {
    unsigned shifts = 0u;
    SimulateJsrFrame(memory, cpu, right ? 0x8e0bu : 0x8e1cu);
    (void)Lufia2FieldPrepareCoordinateScale(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    if (!cpu->carry) {
        do {
            /* Leave long shifts at the next original loop instruction. */
            if (shifts == 4096u)
                return ExecutionHandoff(cpu, right ? 0x838e0eu : 0x838e1fu);
            ++shifts;
            OpOraValue(cpu, 0u);
            cpu->carry = 0;
            if (cpu->negative)
                cpu->carry = 1;
            if (right)
                RorA16(cpu);
            else
                OpAslA(cpu);
            OpDey(cpu);
        } while (!cpu->zero);
    }
    return ExecutionReturned(right ? 0x838e19u : 0x838e2au);
}

Lufia2ExecutionResult Lufia2FieldScaleCoordinateRight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return ShiftCoordinate(memory, cpu, 1u);
}

Lufia2ExecutionResult Lufia2FieldScaleCoordinateLeft(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return ShiftCoordinate(memory, cpu, 0u);
}

Lufia2ExecutionResult Lufia2FieldPrepareLayerScrollX(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, LAYER_SCROLL_CELL_X));
    cpu->carry = 1;
    OpSbcValue(cpu, 0x0008u);
    OpSta(memory, cpu, OpDp(cpu, LAYER_SCALE_COORDINATE));
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SECTION_WORD + 1u));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    return ExecutionReturned(0x838defu);
}

Lufia2ExecutionResult Lufia2FieldPrepareLayerScrollY(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0080u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_SCROLL_X & 0xffffu));
    OpLda(memory, cpu, OpDp(cpu, LAYER_SCROLL_CELL_Y));
    cpu->carry = 1;
    OpSbcValue(cpu, 0x0007u);
    OpSta(memory, cpu, OpDp(cpu, LAYER_SCALE_COORDINATE));
    OpSepWidths(cpu, 0x20u);
    return Lufia2FieldLayerScaleMode(memory, cpu);
}
