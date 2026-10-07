#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    PROBE_INPUT_BUTTONS = 0x47,
    PROBE_SPECIAL_ACTION = 0x10,
    PROBE_NORMAL_ACTION = 0x08,
    PROBE_ACTOR_SLOTS = 40
};

static bool ProbeCollisionContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x83u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2FieldReadProbeAttribute(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeCollisionContext(cpu))
        return ExecutionHandoff(cpu, 0x83fbf1u);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    Lufia2MapCellIndex(memory, cpu, 0xfbf8u, 0u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    return ExecutionReturned(0x83fbfdu);
}

Lufia2ExecutionResult Lufia2FieldClearPendingOccupancy(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeCollisionContext(cpu))
        return ExecutionHandoff(cpu, 0x83fbfeu);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    cpu->carry = false;
    OpAdc(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpDecA(cpu);
    OpDecA(cpu);
    Lufia2MapCellIndex(memory, cpu, 0xfc10u, 0u);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    OpAndValue(cpu, 0xf7u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    return ExecutionReturned(0x83fc1bu);
}

Lufia2ExecutionResult Lufia2FieldFindSpecialActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeCollisionContext(cpu))
        return ExecutionHandoff(cpu, 0x83fc3cu);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
        OpBitValue(cpu, 4u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
            OpCmpValue(cpu, 0xffu);
            cpu->carry = false;
            if (cpu->zero)
                return ExecutionReturned(0x83fc55u);
        }
        OpInx(cpu);
        OpCpx(cpu, PROBE_ACTOR_SLOTS);
    } while (!cpu->zero);
    cpu->carry = true;
    return ExecutionReturned(0x83fc55u);
}

Lufia2ExecutionResult Lufia2FieldProbeInputAllowed(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeCollisionContext(cpu))
        return ExecutionHandoff(cpu, 0x83fc56u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
    cpu->carry = false;
    if (!cpu->zero) {
        OpLoadA(cpu, 0x80u);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PROBE_INPUT_BUTTONS)));
        cpu->carry = false;
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
            if (cpu->zero)
                cpu->carry = true;
        }
    }
    return ExecutionReturned(0x83fc68u);
}

Lufia2ExecutionResult Lufia2FieldUpdateProbeAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeCollisionContext(cpu))
        return ExecutionHandoff(cpu, 0x83fc69u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E057C));
    if (!cpu->zero) {
        OpLoadA(cpu, 0x80u);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PROBE_INPUT_BUTTONS)));
        if (!cpu->zero) {
            OpLoadA(cpu, PROBE_SPECIAL_ACTION);
            OpSta(memory, cpu, WRAM_UNK_7FE4DE);
        } else {
            OpLda(memory, cpu, WRAM_UNK_7FE4DE);
            OpCmpValue(cpu, PROBE_SPECIAL_ACTION);
            if (cpu->zero) {
                OpLoadA(cpu, PROBE_NORMAL_ACTION);
                OpSta(memory, cpu, WRAM_UNK_7FE4DE);
            }
        }
    }
    return ExecutionReturned(0x83fc8au);
}
