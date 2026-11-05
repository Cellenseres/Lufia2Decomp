#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/legacy_fragments.h"

enum {
    RECORD_POSITION = 0x5bc1u,
    RECORD_SCALE = 0x5bcau,
    RECORD_EXTENT = 0x5bcbu,
    RECORD_KIND = 0x5be3u,
    RECORD_LIMIT = 0x5be8u,
    RECORD_REMAINING = 0x5be9u,
    RECORD_COMPONENTS = 0x5bebu,
    RECORD_SEPARATION = 0x5bedu,
    RECORD_VALUE_TABLE = 0x6079u,
    RECORD_COMPONENT_BASE = 0x6179u,
    RECORD_COMPONENT_OUTPUT = 0xb986u,
    POINT_INPUT = 0x16u,
    RECORD_VALUE = 0x1au,
    RECORD_DELTA = 0x1cu,
    RECORD_OFFSET = 0xc0u,
    SNES_MULTIPLICAND = 0x4202u,
    SNES_MULTIPLIER = 0x4203u,
    SNES_PRODUCT = 0x4216u
};

static bool LegacyRecordMathSupported(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x85u && cpu->data_bank == 0x7fu &&
        cpu->accumulator_is_8_bit == 1u && cpu->index_is_8_bit == 0u &&
        !cpu->direct_page && !cpu->decimal;
}

static bool LegacyRecordStackSupported(const Lufia2CpuState *cpu) {
    return LegacyRecordMathSupported(cpu) &&
        cpu->stack >= 0x1f04u && cpu->stack <= 0x1ff8u;
}

static void LegacyLoadRecordValue(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, RECORD_OFFSET));
    for (unsigned bit = 0u; bit < 4u; ++bit)
        OpLsrA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, RECORD_VALUE_TABLE));
    OpAndValue(cpu, 0xffu);
}

Lufia2ExecutionResult Lufia2LegacyLoadRecordValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!LegacyRecordMathSupported(cpu))
        return ExecutionHandoff(cpu, 0x85fc84u);
    OpRepWidths(cpu, 0x20u);
    LegacyLoadRecordValue(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, RECORD_VALUE));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x85fc97u);
}

Lufia2ExecutionResult Lufia2LegacyAccumulateRecordValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!LegacyRecordMathSupported(cpu))
        return ExecutionHandoff(cpu, 0x85fc61u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x1fu));
    OpAndValue(cpu, 0xffu);
    if (cpu->zero) {
        OpLoadA(cpu, 60u);
        OpSta(memory, cpu, OpDp(cpu, RECORD_VALUE));
    }
    LegacyLoadRecordValue(memory, cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, RECORD_VALUE));
    OpSta(memory, cpu, OpDp(cpu, RECORD_VALUE));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x85fc83u);
}

Lufia2ExecutionResult Lufia2LegacyClampRecordDelta(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!LegacyRecordMathSupported(cpu))
        return ExecutionHandoff(cpu, 0x85fc40u);
    OpLda(memory, cpu, OpAbsX(cpu, RECORD_LIMIT));
    OpSta(memory, cpu, OpDp(cpu, 3u));
    OpLda(memory, cpu, OpAbsX(cpu, RECORD_SCALE));
    OpSta(memory, cpu, OpDp(cpu, 7u));
    OpLda(memory, cpu, OpDp(cpu, 0x1fu));
    bool use_limit = !cpu->negative;
    if (!use_limit) {
        OpLda(memory, cpu, OpDp(cpu, 3u));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, RECORD_DELTA + 1u));
        OpCmp(memory, cpu, OpDp(cpu, 7u));
        use_limit = cpu->carry;
    }
    if (use_limit)
        OpLda(memory, cpu, OpDp(cpu, 7u));
    OpSta(memory, cpu, OpDp(cpu, 1u));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 3u)));
    OpSta(memory, cpu, OpDp(cpu, RECORD_DELTA + 1u));
    return ExecutionReturned(0x85fc60u);
}

static bool LegacyRecordSeparated(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t field) {
    OpLda(memory, cpu, OpAbsY(cpu, field));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, field)));
    if (cpu->negative)
        return false;
    OpCmpValue(cpu, 60u);
    return cpu->carry;
}

Lufia2ExecutionResult Lufia2LegacyCheckRecordSeparation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!LegacyRecordStackSupported(cpu))
        return ExecutionHandoff(cpu, 0x85fc98u);
    PushY(memory, cpu);
    OpLdy(cpu, Read16Direct(memory, cpu, RECORD_OFFSET));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0x19u));
    if (LegacyRecordSeparated(memory, cpu, RECORD_SEPARATION) ||
        LegacyRecordSeparated(memory, cpu, RECORD_SEPARATION + 1u) ||
        LegacyRecordSeparated(memory, cpu, RECORD_SEPARATION + 1u))
        OpStepMem(memory, cpu, OpDp(cpu, 0x19u), 1);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x85fcd1u);
}

Lufia2ExecutionResult Lufia2LegacyCopyRecordDeltas(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!LegacyRecordMathSupported(cpu))
        return ExecutionHandoff(cpu, 0x85fe7fu);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    for (unsigned bit = 0u; bit < 3u; ++bit)
        OpLsrA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    for (unsigned component = 0u; component < 4u; ++component) {
        OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(RECORD_COMPONENTS + component)));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu,
            OpAbsY(cpu, (uint16_t)(RECORD_COMPONENT_BASE + component))));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(RECORD_COMPONENT_OUTPUT + component)));
    }
    return ExecutionReturned(0x85feb0u);
}

Lufia2ExecutionResult Lufia2LegacyPrepareScaledRecordDelta(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!LegacyRecordStackSupported(cpu) || !child)
        return ExecutionHandoff(cpu, 0x85fbe3u);
    static const uint16_t fields[] = {
        RECORD_REMAINING, RECORD_LIMIT, RECORD_EXTENT, RECORD_SCALE};
    static const uint8_t scratch[] = {2u, 3u, 6u, 7u};
    for (unsigned field = 0u; field < 4u; ++field) {
        OpLda(memory, cpu, OpAbsX(cpu, fields[field]));
        OpSta(memory, cpu, OpDp(cpu, scratch[field]));
    }
    OpLoadA(cpu, 0u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, 6u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_MULTIPLICAND));
    OpLda(memory, cpu, OpDp(cpu, 3u));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, RECORD_DELTA + 1u)));
    if (cpu->negative)
        OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MULTIPLIER));
    OpSta(memory, cpu, OpDp(cpu, 3u));
    EmitExecutionCheckpoint(memory, cpu, 0x85fc0eu);
    EmitExecutionCheckpoint(memory, cpu, 0x85fc0fu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SNES_PRODUCT));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, 7u));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85fc1du, 0x80e32bu, 3u, 0x85u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x85fc1du, 0u};
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit > 1u ||
        cpu->index_is_8_bit > 1u)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xfc21u);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpCmp(memory, cpu, OpDp(cpu, 2u));
    if (!cpu->carry)
        OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, 3u));
    OpSta(memory, cpu, OpDp(cpu, 1u));
    OpLda(memory, cpu, OpAbsX(cpu, RECORD_REMAINING));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 2u)));
    OpSta(memory, cpu, OpDp(cpu, RECORD_DELTA));
    return ExecutionReturned(0x85fc3fu);
}

Lufia2ExecutionResult Lufia2LegacyProbeNearbyRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!LegacyRecordStackSupported(cpu) || !child)
        return ExecutionHandoff(cpu, 0x85fe28u);
    OpPushX(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, POINT_INPUT));
    PushAccumulator8(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, 0x20u));
    OpStz(memory, cpu, OpDp(cpu, 0x21u));
    OpLda(memory, cpu, OpAbsY(cpu, RECORD_KIND));
    OpSta(memory, cpu, OpDp(cpu, POINT_INPUT));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85fe35u, 0x82e2bdu, 3u, 0x85u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x85fe35u, 0u};
    if (cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit != 0u)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xfe39u);
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    if (!cpu->negative) {
        for (unsigned axis = 0u; axis < 3u; ++axis) {
            const uint16_t field = (uint16_t)(RECORD_POSITION + axis);
            OpLda(memory, cpu, OpAbsX(cpu, field));
            cpu->carry = 1u;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, field)));
            if (cpu->negative) {
                OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
                OpIncA(cpu);
            }
            if (axis < 2u)
                OpSta(memory, cpu, OpDp(cpu, (uint8_t)axis));
        }
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, 0u));
        OpAdc(memory, cpu, OpDp(cpu, 1u));
        OpCmpValue(cpu, 4u);
        if (!cpu->carry) {
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsX(cpu, RECORD_SEPARATION + 1u));
            OpAndValue(cpu, 0xffu);
            OpSta(memory, cpu, OpDp(cpu, 0x20u));
            OpSepWidths(cpu, 0x20u);
        }
    }
    OpLoadA(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, POINT_INPUT));
    OpPullX(memory, cpu);
    return ExecutionReturned(0x85fe7eu);
}
