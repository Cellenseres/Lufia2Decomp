#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    PARTY_RECORDS = WRAM_MENU_PARTY_CHARACTER_OFFSET & 0xffffu,
    ENEMY_RECORDS = WRAM_BATTLE_ENEMY_RECORDS & 0xffffu,
    ACTIVE_PARTY_RECORDS = WRAM_BATTLE_PARTY_RECORDS & 0xffffu,
    FIRST_MODIFIER = 0x37u,
    MODIFIER_COUNT = 7u,
    RECORD_STATUS = 0x0fu
};

static bool RecordResetContext(const Lufia2CpuState *cpu,
    uint16_t minimum_stack, uint16_t maximum_stack) {
    return cpu->program_bank == 0x85u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal &&
        cpu->stack >= minimum_stack && cpu->stack <= maximum_stack;
}

static void ClearRecordModifiers(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t records, uint16_t final_offset) {
    Push8(memory, cpu, cpu->data_bank);
    Push8(memory, cpu, cpu->program_bank);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    OpRepWidths(cpu, 0x20u);
    LoadA16(cpu, cpu->direct_page);
    OpLdx(cpu, final_offset);
    do {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, records)));
        if (!cpu->zero) {
            for (unsigned modifier = 0u; modifier < MODIFIER_COUNT; ++modifier)
                OpSta(memory, cpu, OpAbsY(cpu,
                    (uint16_t)(FIRST_MODIFIER + modifier * 2u)));
        }
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
}

Lufia2ExecutionResult Lufia2BattleClearPartyModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!RecordResetContext(cpu, 0x1f02u, 0x1ffbu))
        return ExecutionHandoff(cpu, 0x85da71u);
    ClearRecordModifiers(memory, cpu, PARTY_RECORDS, 8u);
    return ExecutionReturned(0x85da9bu);
}

Lufia2ExecutionResult Lufia2BattleClearEnemyModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!RecordResetContext(cpu, 0x1f02u, 0x1ffbu))
        return ExecutionHandoff(cpu, 0x85da9cu);
    ClearRecordModifiers(memory, cpu, ENEMY_RECORDS, 10u);
    return ExecutionReturned(0x85dac6u);
}

Lufia2ExecutionResult Lufia2BattleClearSelectedPartyStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!RecordResetContext(cpu, 0x1f00u, 0x1ffcu))
        return ExecutionHandoff(cpu, 0x85ee9bu);
    if (!cpu->zero)
        OpStz(memory, cpu, OpAbsX(cpu, RECORD_STATUS));
    return ExecutionReturned(0x85eea0u);
}

static Lufia2ExecutionResult RecordResetUnwound(uint32_t site) {
    const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
    return result;
}

Lufia2ExecutionResult Lufia2BattleClearAllModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!RecordResetContext(cpu, 0x1f05u, 0x1ffbu) || !child)
        return ExecutionHandoff(cpu, 0x85edb2u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85edb2u, 0x85da71u, 3u, 0x85u))
        return RecordResetUnwound(0x85edb2u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85edb6u, 0x85da9cu, 3u, 0x85u))
        return RecordResetUnwound(0x85edb6u);
    return ExecutionReturned(0x85edbau);
}

Lufia2ExecutionResult Lufia2BattleClearPartyStatuses(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!RecordResetContext(cpu, 0x1f02u, 0x1ffbu) || !child)
        return ExecutionHandoff(cpu, 0x85ee82u);
    const uint32_t sites[] = {0x85ee85u, 0x85ee8bu, 0x85ee91u, 0x85ee97u};
    for (unsigned member = 0u; member < 4u; ++member) {
        OpLdx(cpu, OpReadX(memory, cpu,
            OpAbs(cpu, (uint16_t)(ACTIVE_PARTY_RECORDS + member * 2u))));
        if (!CallChildWithFrame(memory, cpu, child, context,
                sites[member], 0x85ee9bu, 2u, 0x85u))
            return RecordResetUnwound(sites[member]);
    }
    return ExecutionReturned(0x85ee9au);
}
