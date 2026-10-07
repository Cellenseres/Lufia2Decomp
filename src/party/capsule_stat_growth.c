#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/party.h"
#include "system/wram.h"

enum {
    DP_CAPSULE_GROWTH_SUM = 0x11u,
    DP_CAPSULE_GROWTH_FORM = 0x13u,
    DP_CAPSULE_GROWTH_COUNTER = 0x15u,
    DP_CAPSULE_GROWTH_ROW = 0x17u,
    DP_CAPSULE_GROWTH_RATE = 0x1cu,
    ROM_CAPSULE_GROWTH_RATES = 0xa6f420u,
    CAPSULE_RECORD_BANK = 0x97u
};

static void ReadCapsuleLevel(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_WORK_LEVEL));
    OpAndValue(cpu, 0xffu);
}

static void AccumulateCapsuleGrowth(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_COUNTER));
    for (unsigned shift = 0; shift < 4u; ++shift)
        OpLsrA(cpu);
    PushAccumulator16(memory, cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_ROW));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_CAPSULE_GROWTH_RATES));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_RATE));
    PullAccumulator16(memory, cpu);
    OpCmp(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_FORM));
    if (cpu->carry) {
        if (!cpu->zero)
            OpLsrMem(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_RATE));
        OpLsrMem(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_RATE));
    }
    OpLda(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_RATE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_SUM));
    OpSta(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_SUM));
}

Lufia2ExecutionResult Lufia2CapsuleAccumulateStatGrowth(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x200u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82d31cu);
    OpAndValue(cpu, 0xffu);
    for (unsigned shift = 0; shift < 3u; ++shift)
        OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_ROW));
    OpStz(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_SUM));
    ReadCapsuleLevel(memory, cpu);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_FORM));
        OpSta(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_FORM));
        OpStz(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_FORM + 1u));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_COUNTER));
        do {
            AccumulateCapsuleGrowth(memory, cpu);
            ReadCapsuleLevel(memory, cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_COUNTER), 1);
            OpCmp(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_COUNTER));
        } while (!cpu->zero);
    }
    for (unsigned shift = 0; shift < 4u; ++shift)
        OpLsrMem(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_SUM));
    return ExecutionReturned(0x82d377u);
}

static Lufia2ExecutionResult CapsuleStatUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2CapsuleBuildStatValues(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    static const uint8_t growth_offsets[] = {0x1du, 0x1eu, 0x1fu, 0x20u, 0x21u, 0x22u};
    static const uint8_t base_offsets[] = {0x15u, 0x18u, 0x19u, 0x1au, 0x1bu, 0x1cu};
    static const uint16_t destinations[] = {WRAM_CAPSULE_BASE_STAT_VALUES, WRAM_CAPSULE_BASE_STAT_VALUES + 4u,
        WRAM_CAPSULE_BASE_STAT_VALUES + 6u, WRAM_CAPSULE_BASE_STAT_VALUES + 8u,
        WRAM_CAPSULE_BASE_STAT_VALUES + 10u, WRAM_CAPSULE_BASE_STAT_VALUES + 12u};
    static const uint32_t sites[] = {0x82d2a5u, 0x82d2bau, 0x82d2ccu, 0x82d2deu, 0x82d2f0u, 0x82d302u};
    if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x82d283u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82d283u, 0x82c38bu, 2u, 0x82u))
        return CapsuleStatUnwound(0x82d283u);
    OpLdy(cpu, OpRead16(memory, OpAbs(cpu, WRAM_CAPSULE_RECORD_POINTER)));
    PushDataBank(memory, cpu);
    OpLoadA(cpu, CAPSULE_RECORD_BANK);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, 0x16u));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_STAT_COPY_A));
    OpLda(memory, cpu, OpAbsY(cpu, 0x17u));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_STAT_COPY_B));
    for (unsigned stat = 0; stat < 6u; ++stat) {
        OpLda(memory, cpu, OpAbsY(cpu, growth_offsets[stat]));
        if (!CallChildWithFrame(memory, cpu, child, context,
                sites[stat], 0x82d31cu, 2u, 0x82u))
            return CapsuleStatUnwound(sites[stat]);
        OpLda(memory, cpu, OpAbsY(cpu, base_offsets[stat]));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, DP_CAPSULE_GROWTH_SUM));
        OpSta(memory, cpu, OpAbs(cpu, destinations[stat]));
        if (stat == 0u)
            OpSta(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_CURRENT_HP));
    }
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    LoadX16(cpu, WRAM_CAPSULE_WORK_STATS);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x82d317u, 0x81f4d5u, 3u, 0x82u))
        return CapsuleStatUnwound(0x82d317u);
    return ExecutionReturned(0x82d31bu);
}
