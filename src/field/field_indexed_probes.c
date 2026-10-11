#include "lufia2/field.h"
#include "field/indexed_probe_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum { DP_COLLISION_SPAN = 0x9a };

static uint8_t IndexedProbeContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page;
}

static uint8_t IndexedProbeChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x83u)) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static Lufia2ExecutionResult ProbeLowerHeight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t entry, int horizontal_step, unsigned vertical_steps) {
    if (!IndexedProbeContext(cpu) || !child)
        return ExecutionHandoff(cpu, entry);
    if (horizontal_step)
        OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_X), horizontal_step);
    for (unsigned step = 0; step < vertical_steps; ++step)
        OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), 1);
    Lufia2ExecutionResult result;
    if (!IndexedProbeChild(memory, cpu, child, context,
            0x83be49u, 0x83f988u, 2u, &result))
        return result;
    OpCmpValue(cpu, 3u);
    if (!cpu->zero) {
        OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        if (!cpu->zero) {
            if (!IndexedProbeChild(memory, cpu, child, context,
                    0x83be54u, 0x83fb71u, 3u, &result))
                return result;
            OpCmpValue(cpu, 9u);
            if (!cpu->zero) {
                OpCmpValue(cpu, 8u);
                if (!cpu->zero) {
                    cpu->carry = 1u;
                    return ExecutionReturned(0x83be61u);
                }
            }
        }
    }
    cpu->carry = 0u;
    return ExecutionReturned(0x83be63u);
}

Lufia2ExecutionResult Lufia2FieldProbeDownRightHeight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return ProbeLowerHeight(memory, cpu, child, context, 0x83be3bu, 1, 2u);
}

Lufia2ExecutionResult Lufia2FieldProbeDownLeftHeight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return ProbeLowerHeight(memory, cpu, child, context, 0x83be3fu, -1, 2u);
}

Lufia2ExecutionResult Lufia2FieldProbeDownThreeHeight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return ProbeLowerHeight(memory, cpu, child, context, 0x83be43u, 0, 3u);
}

static void ProbeCollisionMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t mask, unsigned plane) {
    OpLoadA(cpu, mask);
    OpAndValue(cpu, OpReadM(memory, cpu,
        OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES + plane)));
}

static Lufia2ExecutionResult FieldProbeDownCollision(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), 1);
    Lufia2ExecutionResult result;
    if (!IndexedProbeChild(memory, cpu, child, context,
            0x83d8adu, 0x83f9adu, 2u, &result))
        return result;
    ProbeCollisionMask(memory, cpu, 0x9bu, 0u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_COLLISION_SPAN));
        OpCmpValue(cpu, 2u);
        if (!cpu->carry) {
            TransferDirectToA(cpu);
            return ExecutionReturned(0x83d8bfu);
        }
        ProbeCollisionMask(memory, cpu, 0x9bu, 1u);
    }
    return ExecutionReturned(0x83d8c6u);
}

static Lufia2ExecutionResult FieldProbeLeftCollision(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!IndexedProbeChild(memory, cpu, child, context,
            0x83d8c7u, 0x83f9adu, 2u, &result))
        return result;
    ProbeCollisionMask(memory, cpu, 0x20u, 0u);
    if (cpu->zero) {
        OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_X), -1);
        if (!IndexedProbeChild(memory, cpu, child, context,
                0x83d8d4u, 0x83f9adu, 2u, &result))
            return result;
        ProbeCollisionMask(memory, cpu, 0x8bu, 0u);
    }
    return ExecutionReturned(0x83d8ddu);
}

static Lufia2ExecutionResult FieldProbeUpCollision(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!IndexedProbeChild(memory, cpu, child, context,
            0x83d8deu, 0x83f9adu, 2u, &result))
        return result;
    ProbeCollisionMask(memory, cpu, 0x10u, 0u);
    if (!cpu->zero)
        return ExecutionReturned(0x83d912u);
    OpLda(memory, cpu, OpDp(cpu, DP_COLLISION_SPAN));
    OpCmpValue(cpu, 2u);
    if (cpu->carry) {
        ProbeCollisionMask(memory, cpu, 0x10u, 1u);
        if (!cpu->zero)
            return ExecutionReturned(0x83d912u);
    }
    OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), -1);
    if (!IndexedProbeChild(memory, cpu, child, context,
            0x83d8f9u, 0x83f9adu, 2u, &result))
        return result;
    ProbeCollisionMask(memory, cpu, 0x8bu, 0u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_COLLISION_SPAN));
        OpCmpValue(cpu, 2u);
        if (!cpu->carry) {
            TransferDirectToA(cpu);
            return ExecutionReturned(0x83d90bu);
        }
        ProbeCollisionMask(memory, cpu, 0x8bu, 1u);
    }
    return ExecutionReturned(0x83d912u);
}

static Lufia2ExecutionResult FieldProbeRightCollision(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpDp(cpu, DP_COLLISION_SPAN));
    OpCmpValue(cpu, 2u);
    if (cpu->carry)
        OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_X), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_X), 1);
    Lufia2ExecutionResult result;
    if (!IndexedProbeChild(memory, cpu, child, context,
            0x83d91du, 0x83f9adu, 2u, &result))
        return result;
    ProbeCollisionMask(memory, cpu, 0xabu, 0u);
    return ExecutionReturned(0x83d926u);
}

Lufia2ExecutionResult Lufia2FieldProbeCollisionBody(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry) {
    switch (entry) {
    case 0x83d8abu:
        return FieldProbeDownCollision(memory, cpu, child, context);
    case 0x83d8c7u:
        return FieldProbeLeftCollision(memory, cpu, child, context);
    case 0x83d8deu:
        return FieldProbeUpCollision(memory, cpu, child, context);
    case 0x83d913u:
        return FieldProbeRightCollision(memory, cpu, child, context);
    default:
        return ExecutionHandoff(cpu, entry);
    }
}

Lufia2ExecutionResult Lufia2FieldProbeDownCollision(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!IndexedProbeContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83d8abu);
    return FieldProbeDownCollision(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2FieldProbeLeftCollision(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!IndexedProbeContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83d8c7u);
    return FieldProbeLeftCollision(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2FieldProbeUpCollision(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!IndexedProbeContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83d8deu);
    return FieldProbeUpCollision(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2FieldProbeRightCollision(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!IndexedProbeContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83d913u);
    return FieldProbeRightCollision(memory, cpu, child, context);
}
