#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_SELECTED_GROUP = 5u,
    DP_ENCOUNTER_WIDTH = 0x11u,
    DP_GROUP_COUNT = 0x12u,
    DP_GROUPS = 0x13u,
    DP_REMAINING_ENEMIES = 0x22u,
    DP_ENEMY_COUNT = 0x23u,
    DP_ENEMIES = 0x24u,
    ENCOUNTER_TABLE = 0xbe93u,
    PARTY_IDS = 0x0a7bu,
    PARTY_ENCOUNTER_COUNTS = 0x859ea1u,
    FIELD_ENCOUNTER_GROUPS = WRAM_BATTLE_ENCOUNTER_WRITE_BASE,
    ENCOUNTER_WIDTH_LIMIT = 0x19u,
    ENEMY_COUNT = 6u
};

static uint32_t EncounterIndexedScratch(const Lufia2CpuState *cpu,
    uint16_t offset) {
    return (uint16_t)(cpu->direct_page + offset + cpu->x) | OP_DP_WRAP;
}

static Lufia2ExecutionResult EncounterChildUnwound(uint32_t site) {
    const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
    return result;
}

Lufia2ExecutionResult Lufia2BattleAddEncounterEnemy(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        !cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f0au || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x8184fcu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SELECTED_GROUP));
    OpTax(cpu);
    OpLda(memory, cpu, EncounterIndexedScratch(cpu, DP_GROUPS));
    OpRepWidths(cpu, 0x30u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x818504u, 0x81fbc6u, 3u, 0x81u))
        return EncounterChildUnwound(0x818504u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x818508u, 0x81fb8eu, 3u, 0x81u))
        return EncounterChildUnwound(0x818508u);
    OpSepWidths(cpu, 0x30u);
    cpu->carry = 1u;
    OpAdc(memory, cpu, OpDp(cpu, DP_ENCOUNTER_WIDTH));
    OpCmpValue(cpu, ENCOUNTER_WIDTH_LIMIT);
    if (!cpu->carry) {
        OpSta(memory, cpu, OpDp(cpu, DP_ENCOUNTER_WIDTH));
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SELECTED_GROUP)));
        OpStepMem(memory, cpu,
            EncounterIndexedScratch(cpu, DP_GROUPS + 1u), 1);
        OpStepMem(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT), 1);
    }
    return ExecutionReturned(0x81851du);
}

static uint32_t ShuffleEncounterBytes(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint8_t base, uint32_t site, uint16_t range) {
    do {
        OpLoadA(cpu, range);
        if (!CallChildWithFrame(memory, cpu, child, context,
                site, 0x808299u, 3u, 0x81u))
            return site;
        OpTax(cpu);
        OpLda(memory, cpu, EncounterIndexedScratch(cpu, base));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, EncounterIndexedScratch(cpu, base));
        OpDey(cpu);
    } while (!cpu->zero);
    return 0u;
}

static uint32_t SelectEncounterGroups(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x23u));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpRepWidths(cpu, 0x10u);
            return 0xffffffffu;
        }
    } else {
        OpLda(memory, cpu, OpDp(cpu, 0x23u));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpLoadA(cpu, 0xfeu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x8183e9u, 0x808299u, 3u, 0x81u))
                return 0x8183e9u;
            OpBitValue(cpu, 1u);
            if (!cpu->zero) {
                OpRepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpDp(cpu, 0x22u));
                ExchangeAccumulatorBytes(cpu);
                OpSta(memory, cpu, OpDp(cpu, 0x22u));
            }
        }
    }
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0x24u);
    uint32_t failed = ShuffleEncounterBytes(memory, cpu, child, context,
        0x23u, 0x8183ffu, 6u);
    if (failed)
        return failed;
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    for (;;) {
        OpLda(memory, cpu, EncounterIndexedScratch(cpu, 0x22u));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpSta(memory, cpu, OpAbsY(cpu, DP_GROUPS));
            LoadA16(cpu, cpu->direct_page);
            OpSta(memory, cpu, OpAbsY(cpu, DP_GROUPS + 1u));
            OpIny(cpu);
            OpIny(cpu);
            OpCpy(cpu, 6u);
            if (cpu->zero)
                break;
        }
        OpInx(cpu);
        OpCpx(cpu, 8u);
        if (cpu->zero)
            break;
    }
    OpTya(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_GROUP_COUNT));
    return 0u;
}

static uint32_t CountEncounterEnemies(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLdy(cpu, 3u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, PARTY_IDS));
        if (!cpu->negative)
            OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpLda(memory, cpu, OpLongX(cpu, PARTY_ENCOUNTER_COUNTS));
    PushAccumulator8(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81843fu, 0x808299u, 3u, 0x81u))
        return 0x81843fu;
    OpSta(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES));
    LoadA8(cpu, Pull8(memory, cpu));
    OpDecA(cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x818447u, 0x808299u, 3u, 0x81u))
        return 0x818447u;
    cpu->carry = 1u;
    OpAdc(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES));
    OpSta(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES));
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT));
    OpSta(memory, cpu, OpDp(cpu, DP_ENCOUNTER_WIDTH));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x818455u, 0x8184fcu, 2u, 0x81u))
        return 0x818455u;
    OpStepMem(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES), -1);
    while (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_GROUP_COUNT));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x81845eu, 0x808299u, 3u, 0x81u))
            return 0x81845eu;
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x818462u, 0x8184fcu, 2u, 0x81u))
            return 0x818462u;
        OpStepMem(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES), -1);
    }
    return 0u;
}

static void ExpandEncounterGroups(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    do {
        OpLda(memory, cpu, EncounterIndexedScratch(cpu, DP_GROUPS + 1u));
        if (!cpu->zero) {
            OpLda(memory, cpu, EncounterIndexedScratch(cpu, DP_GROUPS));
            OpCmpValue(cpu, 0xffu);
            if (!cpu->zero) {
                do {
                    OpSta(memory, cpu, OpAbsY(cpu, DP_ENEMIES));
                    OpIny(cpu);
                    OpStepMem(memory, cpu,
                        EncounterIndexedScratch(cpu, DP_GROUPS + 1u), -1);
                } while (!cpu->zero);
            }
        }
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 6u);
    } while (!cpu->zero);
    OpLoadA(cpu, 0xffu);
    for (;;) {
        OpCpy(cpu, ENEMY_COUNT);
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpAbsY(cpu, DP_ENEMIES));
        OpIny(cpu);
    }
}

static uint32_t ShuffleEncounterEnemies(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT));
    OpCmpValue(cpu, 1u);
    if (cpu->zero)
        return 0u;
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0x19u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT), -1);
    do {
        OpLda(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT));
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x81849eu, 0x808299u, 3u, 0x81u))
            return 0x81849eu;
        OpAndValue(cpu, 0xffu);
        OpTax(cpu);
        OpLda(memory, cpu, EncounterIndexedScratch(cpu, DP_ENEMIES));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, EncounterIndexedScratch(cpu, DP_ENEMIES));
        OpDey(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return 0u;
}

static uint32_t SelectEncounterFlags(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, 5u);
    do {
        OpLda(memory, cpu, EncounterIndexedScratch(cpu, DP_ENEMIES));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x8184bbu, 0x81fb79u, 3u, 0x81u))
                return 0x8184bbu;
            OpSta(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES));
            OpLoadA(cpu, 0x64u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x8184c3u, 0x808299u, 3u, 0x81u))
                return 0x8184c3u;
            cpu->carry = 1u;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_REMAINING_ENEMIES)));
        }
        OpRolMem8(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT));
        OpDex(cpu);
    } while (!cpu->negative);
    return 0u;
}

static void StoreGeneratedEncounter(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    PullAccumulator16(memory, cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0100u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 5u);
    OpLda(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT));
    OpSta(memory, cpu, OpLongX(cpu, FIELD_ENCOUNTER_GROUPS + 1u));
    LoadA16(cpu, cpu->direct_page);
    for (unsigned byte = 2u; byte <= 4u; ++byte)
        OpSta(memory, cpu, OpLongX(cpu, FIELD_ENCOUNTER_GROUPS + byte));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, DP_ENEMIES));
        OpSta(memory, cpu, OpLongX(cpu, FIELD_ENCOUNTER_GROUPS));
        OpDex(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

Lufia2ExecutionResult Lufia2BattleGenerateEncounterRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->decimal || cpu->direct_page != 0u || cpu->stack < 0x1f10u ||
        cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x81839fu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x10u);
    OpPushX(memory, cpu);
    OpSepWidths(cpu, 0x30u);
    OpLdy(cpu, 0x97u);
    PushY(memory, cpu);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    OpRepWidths(cpu, 0x30u);
    OpAndValue(cpu, 0xffu);
    for (unsigned bit = 0u; bit < 3u; ++bit)
        OpAslA(cpu);
    OpTay(cpu);
    for (unsigned word = 0u; word < 4u; ++word) {
        OpLda(memory, cpu, OpAbsY(cpu, (uint16_t)(ENCOUNTER_TABLE + word * 2u)));
        OpSta(memory, cpu, OpDp(cpu, (uint8_t)(0x22u + word * 2u)));
    }
    for (unsigned word = 0u; word < 4u; ++word)
        OpStz(memory, cpu, OpDp(cpu, (uint8_t)(0x11u + word * 2u)));
    OpSepWidths(cpu, 0x30u);
    uint32_t failed = SelectEncounterGroups(memory, cpu, child, context);
    if (failed == 0xffffffffu) {
        StoreGeneratedEncounter(memory, cpu);
        return ExecutionReturned(0x8184fbu);
    }
    if (!failed)
        failed = CountEncounterEnemies(memory, cpu, child, context);
    if (!failed) {
        ExpandEncounterGroups(memory, cpu);
        failed = ShuffleEncounterEnemies(memory, cpu, child, context);
    }
    if (!failed)
        failed = SelectEncounterFlags(memory, cpu, child, context);
    if (failed)
        return EncounterChildUnwound(failed);
    StoreGeneratedEncounter(memory, cpu);
    return ExecutionReturned(0x8184fbu);
}
