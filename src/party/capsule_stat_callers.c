#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "system/wram.h"

enum {
    DP_MODIFIER_RECORD = 0x2au,
    PRIMARY_MODIFIERS = 0x74u,
    PRIMARY_MODIFIER_COUNT = 9u,
    SECONDARY_MODIFIERS = 0x86u,
    SECONDARY_MODIFIER_COUNT = 7u
};

static void ClearRecordModifiers(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t first, unsigned count) {
    OpLdx(cpu, cpu->index_is_8_bit ?
        Read8(memory, DirectAddress(cpu, DP_MODIFIER_RECORD)) :
        Read16Direct(memory, cpu, DP_MODIFIER_RECORD));
    for (unsigned modifier = 0u; modifier < count; ++modifier)
        OpStz(memory, cpu, OpAbsX(cpu, (uint16_t)(first + 2u * modifier)));
}

Lufia2ExecutionResult Lufia2PartyClearSecondaryModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f6a4u);
    ClearRecordModifiers(memory, cpu, SECONDARY_MODIFIERS,
        SECONDARY_MODIFIER_COUNT);
    return ExecutionReturned(0x82f6bbu);
}

Lufia2ExecutionResult Lufia2PartyClearPrimaryModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82f6d4u);
    ClearRecordModifiers(memory, cpu, PRIMARY_MODIFIERS,
        PRIMARY_MODIFIER_COUNT);
    return ExecutionReturned(0x82f6f1u);
}

Lufia2ExecutionResult Lufia2CapsuleRebuildStatBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x82u || cpu->index_is_8_bit ||
        cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x82d270u);
    OpRepWidths(cpu, 0x20u);
    LoadX16(cpu, WRAM_CAPSULE_WORK_STATS);
    Write16Direct(memory, cpu, DP_MODIFIER_RECORD, cpu->x);
    const uint32_t calls[] = {0x82d277u, 0x82d27au, 0x82d27fu};
    const uint32_t targets[] = {0x82f6a4u, 0x82f6d4u, 0x82d283u};
    for (unsigned phase = 0u; phase < 3u; ++phase) {
        if (phase == 2u)
            OpSepWidths(cpu, 0x20u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                calls[phase], targets[phase], 2u, 0x82u)) {
            Lufia2ExecutionResult result = ExecutionReturned(calls[phase]);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
    }
    return ExecutionReturned(0x82d282u);
}
