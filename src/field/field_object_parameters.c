#include "lufia2/field.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    OBJECT_PARAMETER_TABLE = 0xc6fbu,
    OBJECT_PARAMETER_ROW_SIZE = 5u,
    DP_PARAMETER_KIND = 0x54u,
    DP_PARAMETER_FLAGS_LOW = 0x58u,
    DP_PARAMETER_FLAGS_HIGH = 0x59u,
    DP_PARAMETER_DESTINATION = 0x5du,
    DP_PARAMETER_SOURCE = 0x60u
};

static void ApplyObjectParameter(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->negative) {
        OpAndValue(cpu, 0x7fu);
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_PARAMETER_FLAGS_HIGH));
    } else {
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_PARAMETER_FLAGS_LOW));
    }
    OpBit(memory, cpu, OpAbsX(cpu, OBJECT_PARAMETER_TABLE + 1u));
    if (cpu->zero)
        return;
    ExchangeAccumulatorBytes(cpu);
    OpOra(memory, cpu, OpAbsY(cpu, WRAM_FIELD_RECOVERY_OBJECT_UPDATES));
    OpSta(memory, cpu, OpAbsY(cpu, WRAM_FIELD_RECOVERY_OBJECT_UPDATES));
    OpLda(memory, cpu, OpAbsX(cpu, OBJECT_PARAMETER_TABLE + 2u));
    OpSta(memory, cpu, OpDp(cpu, DP_PARAMETER_DESTINATION));
    OpLda(memory, cpu, OpAbsX(cpu, OBJECT_PARAMETER_TABLE + 3u));
    OpSta(memory, cpu, OpDp(cpu, DP_PARAMETER_DESTINATION + 1u));
    OpLda(memory, cpu, OpAbsX(cpu, OBJECT_PARAMETER_TABLE + 4u));
    OpSta(memory, cpu, OpDp(cpu, DP_PARAMETER_SOURCE));
    OpStz(memory, cpu, OpDp(cpu, DP_PARAMETER_SOURCE + 1u));
    OpLda(memory, cpu, OpAbs(cpu,
        Read16Direct(memory, cpu, DP_PARAMETER_SOURCE)));
    OpSta(memory, cpu, AbsoluteIndexedAddress(cpu,
        Read16Direct(memory, cpu, DP_PARAMETER_DESTINATION), cpu->y));
}

static void ApplyObjectParameterRows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, OBJECT_PARAMETER_TABLE));
        if (cpu->zero)
            break;
        ApplyObjectParameter(memory, cpu);
        OpLdx(cpu, (uint16_t)(cpu->x + OBJECT_PARAMETER_ROW_SIZE));
    }
}

Lufia2ExecutionResult Lufia2FieldApplyObjectParameters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83c6aau);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_UNK_7E1559));
        OpCmp(memory, cpu, OpDp(cpu, DP_PARAMETER_KIND));
        if (cpu->zero)
            ApplyObjectParameterRows(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, 32u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x83c6fau);
}
