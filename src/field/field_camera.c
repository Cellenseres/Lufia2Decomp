#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum { CAMERA_CELL_X = 0x56u, CAMERA_CELL_Y = 0x58u };

Lufia2ExecutionResult Lufia2FieldPrepareCameraScroll(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x8eu || cpu->decimal)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xb09cu);
    OpSepWidths(cpu, 0x10u);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, Read8(memory, OpAbs(cpu, WRAM_FIELD_CAMERA_ACTOR)));
    /* Pixel offsets use actor zero; cell positions use the selected actor. */
    OpLda(memory, cpu, WRAM_ACTOR_FINE_X);
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_CAMERA_SCROLL_X));
    OpLda(memory, cpu, WRAM_ACTOR_FINE_Y);
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x70u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_CAMERA_SCROLL_Y));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_FINE_X));
    for (unsigned i = 0u; i < 4u; ++i)
        OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, CAMERA_CELL_X));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_FINE_Y));
    for (unsigned i = 0u; i < 4u; ++i)
        OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, CAMERA_CELL_Y));
    SimulateJslFrame(memory, cpu, 0x8eu, 0xb0d0u);
    if (!child(context, cpu, 0x838d42u, 0x8eb0cdu, 3u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x8eb0cdu);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8eb0d3u);
}
