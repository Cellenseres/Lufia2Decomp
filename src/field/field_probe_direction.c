#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "field/field_coordinates_internal.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    ROM_PROBE_DIRECTION_TABLE = 0xfbc6
};

static bool ProbeDirectionContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x83u &&
        cpu->stack >= 0x1f04u && cpu->stack <= 0x1ffcu;
}

static bool ProbeShortReturn(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint16_t expected) {
    uint16_t returned = Pull8(memory, cpu);
    returned |= (uint16_t)Pull8(memory, cpu) << 8;
    if (returned == expected)
        return true;
    cpu->resume_pc = 0x830000u | (uint16_t)(returned + 1u);
    return false;
}

Lufia2ExecutionResult Lufia2FieldProbeDirectionBlocked(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    uint16_t target, back;
    uint8_t mask;
    if (!ProbeDirectionContext(cpu) || A8(cpu) > 6u || (A8(cpu) & 1u))
        return ExecutionHandoff(cpu, 0x83fbbdu);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpTax(cpu);
    target = Read16ProgramIndexed(memory, cpu, ROM_PROBE_DIRECTION_TABLE, cpu->x);
    SimulateJsrFrame(memory, cpu, 0xfbc4u);
    switch (target) {
    case 0xfbceu:
        OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), 1);
        back = 0xfbd2u;
        mask = 0x10u;
        break;
    case 0xfbd7u:
        back = 0xfbd9u;
        mask = 0x20u;
        break;
    case 0xfbdeu:
        back = 0xfbe0u;
        mask = 0x10u;
        break;
    case 0xfbe5u:
        OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_X), 1);
        back = 0xfbe9u;
        mask = 0x20u;
        break;
    default:
        return ExecutionHandoff(cpu, 0x830000u | target);
    }
    SimulateJsrFrame(memory, cpu, back);
    Lufia2ExecutionResult attributes = Lufia2FieldReadProbeAttribute(memory, cpu);
    if (attributes.flow != LUFIA2_EXECUTION_RETURNED)
        return attributes;
    if (!ProbeShortReturn(memory, cpu, back))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpAndValue(cpu, mask);
    cpu->carry = !cpu->zero;
    if (!ProbeShortReturn(memory, cpu, 0xfbc4u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    return ExecutionReturned(0x83fbc5u);
}

Lufia2ExecutionResult Lufia2FieldSaveProbePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeDirectionContext(cpu))
        return ExecutionHandoff(cpu, 0x83f49au);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpSta(memory, cpu, WRAM_FIELD_SAVED_PROBE_POSITION);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpSta(memory, cpu, WRAM_FIELD_SAVED_PROBE_POSITION + 1u);
    return ExecutionReturned(0x83f4a6u);
}

Lufia2ExecutionResult Lufia2FieldRestoreProbePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeDirectionContext(cpu))
        return ExecutionHandoff(cpu, 0x83f4a7u);
    OpLda(memory, cpu, WRAM_FIELD_SAVED_PROBE_POSITION);
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpLda(memory, cpu, WRAM_FIELD_SAVED_PROBE_POSITION + 1u);
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    return ExecutionReturned(0x83f4b3u);
}

Lufia2ExecutionResult Lufia2ObjectProbeNextTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ProbeDirectionContext(cpu))
        return ExecutionHandoff(cpu, 0x83ec5fu);
    SimulateJsrFrame(memory, cpu, 0xec61u);
    Lufia2ObjectRoundedProbe(memory, cpu);
    if (!ProbeShortReturn(memory, cpu, 0xec61u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FD9CC));
    SimulateJslFrame(memory, cpu, cpu->program_bank, 0xec6bu);
    if (!Lufia2ActorMovementStep(memory, cpu))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    uint16_t returned = Pull8(memory, cpu);
    returned |= (uint16_t)Pull8(memory, cpu) << 8;
    uint8_t bank = Pull8(memory, cpu);
    if (returned != 0xec6bu || bank != 0x83u)
        return ExecutionHandoff(cpu, ((uint32_t)bank << 16) | (uint16_t)(returned + 1u));
    return ExecutionReturned(0x83ec6cu);
}
