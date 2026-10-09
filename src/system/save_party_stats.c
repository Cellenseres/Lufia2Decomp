#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    SAVE_PARTY_MEMBER_INDEX = WRAM_PARTY_STAT_MEMBER_INDEX,
    SAVE_PARTY_LEVEL = WRAM_PARTY_STAT_LEVEL,
    SAVE_PARTY_BASE_STATS = WRAM_PARTY_BASE_STAT_WORK,
    SAVE_PARTY_LEVEL_EXPERIENCE = WRAM_PARTY_LEVEL_EXPERIENCE_WORK,
    SAVE_PARTY_RECORD_LEVEL = 0x0eu,
    SAVE_PARTY_RECORD_BASE_STATS = 0x51u,
    SAVE_PARTY_RECORD_THRESHOLD = 0x62u,
    SAVE_PARTY_STAT_COUNT = 7u,
    SAVE_PARTY_MEMBER_COUNT = 7u,
    ROM_SAVE_PARTY_RECORDS = 0x859ebau
};

static void SelectSavedPartyLevel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, SAVE_PARTY_RECORD_LEVEL));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SAVE_PARTY_LEVEL));
    OpSepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x97u);
}

static Lufia2ExecutionResult SaveStatsUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SaveStatsChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target) {
    return CallChildWithFrame(
        memory, cpu, child, context, site, target, 3u, cpu->program_bank);
}

Lufia2ExecutionResult Lufia2SaveDerivePartyStatBonuses(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85cc69u);
    OpRepWidths(cpu, 0x20u);
    SelectSavedPartyLevel(memory, cpu);
    if (!SaveStatsChild(memory, cpu, child, context, 0x85cc7bu, 0x81f87fu))
        return SaveStatsUnwound(0x85cc7bu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    for (unsigned stat = 0u; stat < SAVE_PARTY_STAT_COUNT; ++stat) {
        const uint16_t offset = (uint16_t)(2u * stat);
        OpLda(memory, cpu, OpAbsY(cpu, (uint16_t)(SAVE_PARTY_RECORD_BASE_STATS + offset)));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, (uint16_t)(SAVE_PARTY_BASE_STATS + offset))));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(SAVE_PARTY_BASE_STATS + offset)));
    }
    OpStepMem(memory, cpu, OpAbs(cpu, SAVE_PARTY_MEMBER_INDEX), 1);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x85cccdu);
}

Lufia2ExecutionResult Lufia2SaveRestorePartyBaseStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85cbdcu);
    OpLdx(cpu, 0u);
    OpRepWidths(cpu, 0x20u);
    do {
        OpLda(memory, cpu, LongIndexedAddress(ROM_SAVE_PARTY_RECORDS, cpu->x));
        OpTay(cpu);
        OpTxa(cpu);
        OpLsrA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, SAVE_PARTY_MEMBER_INDEX));
        SelectSavedPartyLevel(memory, cpu);
        if (!SaveStatsChild(memory, cpu, child, context, 0x85cbfbu, 0x81f87fu))
            return SaveStatsUnwound(0x85cbfbu);
        PullDataBank(memory, cpu);
        PushIndex(memory, cpu);
        OpTyx(cpu);
        if (!SaveStatsChild(memory, cpu, child, context, 0x85cc02u, 0x81f9e9u))
            return SaveStatsUnwound(0x85cc02u);
        OpTxy(cpu);
        OpPullX(memory, cpu);
        OpLda(memory, cpu, OpAbs(cpu, SAVE_PARTY_LEVEL_EXPERIENCE + 2u));
        OpSta(memory, cpu, OpAbsY(cpu, SAVE_PARTY_RECORD_THRESHOLD + 2u));
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, SAVE_PARTY_LEVEL_EXPERIENCE));
        OpSta(memory, cpu, OpAbsY(cpu, SAVE_PARTY_RECORD_THRESHOLD));
        for (unsigned stat = 0u; stat < SAVE_PARTY_STAT_COUNT; ++stat) {
            const uint16_t offset = (uint16_t)(2u * stat);
            OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(SAVE_PARTY_BASE_STATS + offset)));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpAbsY(cpu, (uint16_t)(SAVE_PARTY_RECORD_BASE_STATS + offset)));
            OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(SAVE_PARTY_RECORD_BASE_STATS + offset)));
        }
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 2u * SAVE_PARTY_MEMBER_COUNT);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x85cc68u);
}
