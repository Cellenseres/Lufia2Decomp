#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "system/dp_scratch.h"

enum { FIELD_CONTROL_LATCH = 0x099cu };

static bool ActorEventContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2FieldProbeLeaderPosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ActorEventContext(cpu))
        return ExecutionHandoff(cpu, 0x83c0efu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    return ExecutionReturned(0x83c0f9u);
}

Lufia2ExecutionResult Lufia2FieldAcknowledgeControlChange(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ActorEventContext(cpu))
        return ExecutionHandoff(cpu, 0x83c0fau);
    OpLoadA(cpu, 0x20u);
    OpTestBits(memory, cpu, OpAbs(cpu, FIELD_CONTROL_LATCH), 0u);
    if (!cpu->zero) {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, WRAM_FIELD_CONTROL_CHANGE_PENDING);
    }
    return ExecutionReturned(0x83c107u);
}

Lufia2ExecutionResult Lufia2FieldSetFollowingObjectDrawFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ActorEventContext(cpu))
        return ExecutionHandoff(cpu, 0x83f0bcu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_DRAW_FLAGS));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpBitValue(cpu, 4u);
    if (!cpu->zero) {
        OpLdx(cpu, 0u);
        do {
            TransferDirectToA(cpu);
            OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD0A6));
            OpCmpValue(cpu, 0xffu);
            if (!cpu->zero) {
                OpPushX(memory, cpu);
                OpTax(cpu);
                OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
                OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_DRAW_FLAGS));
                OpPullX(memory, cpu);
            }
            OpInx(cpu);
            OpCpx(cpu, 8u);
        } while (!cpu->zero);
    }
    return ExecutionReturned(0x83f0e7u);
}
