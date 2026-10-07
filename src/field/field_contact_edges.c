#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    DP_CONTACT_EDGE_X = 0x8fu,
    DP_CONTACT_EDGE_Y = 0x91u,
    ROM_CONTACT_EDGE_TABLE = 0xd92au,
    CONTACT_EDGE_VERTICAL = 0x10u,
    CONTACT_EDGE_HORIZONTAL = 0x20u
};

static Lufia2ExecutionResult ProbeContactEdge(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t entry, uint32_t site, uint8_t coordinate, uint8_t edge) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || !child)
        return ExecutionHandoff(cpu, entry);
    if (coordinate)
        OpStepMem(memory, cpu, OpDp(cpu, coordinate), 1);
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, 0x83f9adu, 2u, 0x83u)) {
        Lufia2ExecutionResult result = ExecutionReturned(site);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_MAP_ATTRIBUTES));
    OpBitValue(cpu, edge);
    return ExecutionReturned(site + 9u);
}

Lufia2ExecutionResult Lufia2FieldProbeContactEdgeDown(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return ProbeContactEdge(memory, cpu, child, context,
        0x83d932u, 0x83d934u, DP_CONTACT_EDGE_Y, CONTACT_EDGE_VERTICAL);
}

Lufia2ExecutionResult Lufia2FieldProbeContactEdgeLeft(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return ProbeContactEdge(memory, cpu, child, context,
        0x83d93eu, 0x83d93eu, 0u, CONTACT_EDGE_HORIZONTAL);
}

Lufia2ExecutionResult Lufia2FieldProbeContactEdgeUp(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return ProbeContactEdge(memory, cpu, child, context,
        0x83d948u, 0x83d948u, 0u, CONTACT_EDGE_VERTICAL);
}

Lufia2ExecutionResult Lufia2FieldProbeContactEdgeRight(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    return ProbeContactEdge(memory, cpu, child, context,
        0x83d952u, 0x83d954u, DP_CONTACT_EDGE_X, CONTACT_EDGE_HORIZONTAL);
}

Lufia2ExecutionResult Lufia2FieldProbeContactEdge(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || !child ||
        cpu->x > 6u || (cpu->x & 1u))
        return ExecutionHandoff(cpu, 0x83d927u);
    const uint32_t target = JumpProgramTable(memory, cpu, ROM_CONTACT_EDGE_TABLE);
    switch (target) {
    case 0x83d932u:
        return Lufia2FieldProbeContactEdgeDown(memory, cpu, child, context);
    case 0x83d93eu:
        return Lufia2FieldProbeContactEdgeLeft(memory, cpu, child, context);
    case 0x83d948u:
        return Lufia2FieldProbeContactEdgeUp(memory, cpu, child, context);
    case 0x83d952u:
        return Lufia2FieldProbeContactEdgeRight(memory, cpu, child, context);
    default:
        return ExecutionHandoff(cpu, target);
    }
}
