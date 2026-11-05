#include "core/cpu_ops.h"
#include "lufia2/legacy_fragments.h"

enum {
    LEGACY_RECORD_X = 0x5bc1u,
    LEGACY_RECORD_Y = 0x5bc2u,
    LEGACY_RECORD_DIRECTION_MODE = 0x5be2u,
    LEGACY_SELECTED_DIRECTION_MODE = 0x5ff1u,
    LEGACY_POINT_X = 0x16u,
    LEGACY_POINT_Y = 0x17u,
    LEGACY_DIRECTION_VALUE = 0x18u,
    LEGACY_DIRECTION_INDEX = 0x1cu,
    ROM_LEGACY_DIRECTION_VALUES = 0x85fe24u
};

static void LegacyNegateDelta(Lufia2CpuState *cpu) {
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpIncA(cpu);
}

static void LegacyHorizontalDirection(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, bool alternate) {
    OpLda(memory, cpu, OpDp(cpu, 0u));
    const bool negative_x = cpu->negative;
    if (negative_x) {
        LegacyNegateDelta(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0u));
    }
    OpLdy(cpu, alternate ? 1u : 2u);
    OpLda(memory, cpu, OpDp(cpu, 1u));
    if (cpu->negative) {
        OpLdy(cpu, alternate ? 2u : 1u);
        LegacyNegateDelta(cpu);
    }
    OpCmp(memory, cpu, OpDp(cpu, 0u));
    if (cpu->negative)
        OpLdy(cpu, negative_x ? (alternate ? 0u : 3u) : (alternate ? 3u : 0u));
}

static void LegacyVerticalDirection(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, bool alternate) {
    OpLda(memory, cpu, OpDp(cpu, 1u));
    const bool negative_y = cpu->negative;
    if (negative_y) {
        LegacyNegateDelta(cpu);
        OpSta(memory, cpu, OpDp(cpu, 1u));
    }
    OpLdy(cpu, alternate ? 2u : 1u);
    OpLda(memory, cpu, OpDp(cpu, 0u));
    if (cpu->negative) {
        OpLdy(cpu, alternate ? 1u : 2u);
        LegacyNegateDelta(cpu);
    }
    OpCmp(memory, cpu, OpDp(cpu, 1u));
    if (cpu->negative)
        OpLdy(cpu, negative_y ? (alternate ? 0u : 3u) : (alternate ? 3u : 0u));
}

static void LegacyResolveDirection(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->zero) {
        LegacyHorizontalDirection(memory, cpu, false);
    } else {
        OpDecA(cpu);
        if (cpu->zero) {
            LegacyVerticalDirection(memory, cpu, false);
        } else {
            OpDecA(cpu);
            if (cpu->zero)
                LegacyVerticalDirection(memory, cpu, true);
            else
                LegacyHorizontalDirection(memory, cpu, true);
        }
    }
    OpWrite16(memory, OpDp(cpu, LEGACY_DIRECTION_INDEX), cpu->y);
    OpPushX(memory, cpu);
    OpTyx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_LEGACY_DIRECTION_VALUES));
    OpSta(memory, cpu, OpDp(cpu, LEGACY_DIRECTION_VALUE));
    OpStz(memory, cpu, OpDp(cpu, LEGACY_DIRECTION_VALUE + 1u));
    OpPullX(memory, cpu);
    OpPullY(memory, cpu);
}

Lufia2ExecutionResult Lufia2LegacyResolveRecordDirection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t entry) {
    if (cpu->program_bank != 0x85u || cpu->data_bank != 0x7fu ||
        cpu->accumulator_is_8_bit != 1u || cpu->index_is_8_bit != 0u ||
        cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f04u || cpu->stack > 0x1ff8u ||
        (entry != 0x85fcd2u && entry != 0x85fceau &&
         entry != 0x85fd00u && entry != 0x85fd16u))
        return ExecutionHandoff(cpu, entry);
    PushY(memory, cpu);
    for (unsigned axis = 0u; axis < 2u; ++axis) {
        const uint32_t record = OpAbsX(cpu, (uint16_t)(LEGACY_RECORD_X + axis));
        if (entry == 0x85fceau) {
            OpLda(memory, cpu, OpDp(cpu, (uint8_t)(LEGACY_POINT_X + axis)));
            cpu->carry = 1u;
            OpSbcValue(cpu, OpReadM(memory, cpu, record));
        } else {
            OpLda(memory, cpu, entry == 0x85fd00u ? record :
                OpAbsY(cpu, (uint16_t)(LEGACY_RECORD_X + axis)));
            cpu->carry = 1u;
            OpSbcValue(cpu, OpReadM(memory, cpu, entry == 0x85fcd2u ? record :
                OpDp(cpu, (uint8_t)(LEGACY_POINT_X + axis))));
        }
        OpSta(memory, cpu, OpDp(cpu, (uint8_t)axis));
    }
    OpLda(memory, cpu, entry == 0x85fd16u ?
        OpAbs(cpu, LEGACY_SELECTED_DIRECTION_MODE) :
        OpAbsX(cpu, LEGACY_RECORD_DIRECTION_MODE));
    LegacyResolveDirection(memory, cpu);
    return ExecutionReturned(0x85fe23u);
}
