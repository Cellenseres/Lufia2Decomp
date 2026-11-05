#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/legacy_fragments.h"

enum {
    LEGACY_STATE = 0x1360u,
    LEGACY_SOURCE_BYTE = 0x7f0a52u,
    LEGACY_OUTPUT_BYTE = 0x7f6148u,
    LEGACY_POINTER_BANK = 0x5fu
};

static bool LegacyStackSupported(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u &&
        cpu->stack >= 0x1f04u && cpu->stack <= 0x1ff8u;
}

Lufia2ExecutionResult Lufia2LegacyPreserveRegisters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!LegacyStackSupported(cpu))
        return ExecutionHandoff(cpu, 0x83ff31u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83ff3du);
}

static void PrepareLegacyState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, LEGACY_STATE + 0x15u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, LEGACY_STATE + 0x23u);
    for (unsigned field = 0x0du; field <= 0x0fu; ++field)
        OpStz(memory, cpu, OpAbs(cpu, (uint16_t)(LEGACY_STATE + field)));
    OpLdx(cpu, 0u);
    static const uint8_t words[] = {0x07u, 0x09u, 0x0bu, 0x12u};
    for (unsigned field = 0u; field < sizeof words; ++field)
        OpWriteX(memory, cpu, OpAbs(cpu, LEGACY_STATE + words[field]), cpu->x);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, LEGACY_STATE + 0x25u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, LEGACY_STATE + 0x20u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, LEGACY_POINTER_BANK));
}

static void FinishLegacyState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, LEGACY_SOURCE_BYTE);
    OpSta(memory, cpu, OpAbs(cpu, LEGACY_STATE + 0x21u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, LEGACY_STATE));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, LEGACY_STATE + 0x13u));
    OpStz(memory, cpu, OpAbs(cpu, LEGACY_STATE + 0x14u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, LEGACY_STATE + 0x06u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, LEGACY_STATE + 0x02u));
    OpLdx(cpu, 0u);
    static const uint8_t words[] = {0x16u, 0x18u, 0x19u};
    for (unsigned field = 0u; field < sizeof words; ++field)
        OpWriteX(memory, cpu, OpAbs(cpu, LEGACY_STATE + words[field]), cpu->x);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, LEGACY_STATE + 0x24u);
    OpSta(memory, cpu, LEGACY_OUTPUT_BYTE);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, LEGACY_STATE + 0x05u));
}

Lufia2ExecutionResult Lufia2LegacyInitializeState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!LegacyStackSupported(cpu) || !child || cpu->direct_page)
        return ExecutionHandoff(cpu, 0x83ff3eu);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    PrepareLegacyState(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83ff7au, 0x84b51du, 3u, 0x83u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x83ff7au, 0u};
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xff7eu);
    FinishLegacyState(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83ffb9u);
}
