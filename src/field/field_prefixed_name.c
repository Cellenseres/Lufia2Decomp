#include "lufia2/field.h"
#include "core/cpu_ops.h"

enum {
    DP_NAME_SOURCE = 0x5du,
    DP_NAME_PREFIX = 0x60u,
    NAME_LAST_CHARACTER = 11u,
    NAME_WORK_BUFFER = 0xe000u,
    NAME_PREFIX_BANK = 0x850000u
};

static void CopyNamePrefix(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushAndSetDataBank(memory, cpu, 0x7eu);
    OpLdx(cpu, Read16Direct(memory, cpu, DP_NAME_PREFIX));
    OpLdy(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, NAME_PREFIX_BANK | cpu->x);
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpAbsY(cpu, NAME_WORK_BUFFER));
        OpIny(cpu);
        OpInx(cpu);
    }
    PullDataBank(memory, cpu);
}

static void TrimNameSpaces(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, NAME_LAST_CHARACTER);
    do {
        OpLda(memory, cpu, AbsoluteIndexedAddress(cpu,
            Read16Direct(memory, cpu, DP_NAME_SOURCE), cpu->y));
        OpCmpValue(cpu, 0x20u);
        if (!cpu->zero)
            break;
        TransferDirectToA(cpu);
        OpSta(memory, cpu, AbsoluteIndexedAddress(cpu,
            Read16Direct(memory, cpu, DP_NAME_SOURCE), cpu->y));
        OpDey(cpu);
    } while (!cpu->negative);
}

static void AppendName(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushAndSetDataBank(memory, cpu, 0x7eu);
    OpLdy(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, AbsoluteIndexedAddress(cpu,
            Read16Direct(memory, cpu, DP_NAME_SOURCE), cpu->y));
        OpSta(memory, cpu, OpAbsX(cpu, NAME_WORK_BUFFER));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpIny(cpu);
    }
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2FieldBuildPrefixedName(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8eb5fbu);
    Write16Direct(memory, cpu, DP_NAME_SOURCE, cpu->x);
    CopyNamePrefix(memory, cpu);
    PushY(memory, cpu);
    TrimNameSpaces(memory, cpu);
    OpPullX(memory, cpu);
    AppendName(memory, cpu);
    return ExecutionReturned(0x8eb63au);
}
