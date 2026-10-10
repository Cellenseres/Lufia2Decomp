#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_ENEMY_COUNT = 0u,
    DP_REMAINING_WIDTHS = 1u,
    DP_ENEMY_FLAGS = 2u,
    FIELD_BATTLE_SOURCE = WRAM_FIELD_BATTLE_SOURCE,
    FIELD_BATTLE_RESOURCE = WRAM_FIELD_CONTACT_RESOURCE,
    FIELD_BATTLE_STYLE = WRAM_UNK_7FF8A0,
    FIELD_ENCOUNTERS = WRAM_FIELD_ENCOUNTER_RECORDS,
    SPECIAL_ENCOUNTERS = 0x97c53du,
    ENEMY_GROUPS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_KEYS & 0xffffu,
    ENEMY_WIDTHS = WRAM_BATTLE_ENEMY_ENCOUNTER_WIDTHS & 0xffffu,
    ENEMY_HEIGHTS = WRAM_BATTLE_ENEMY_ENCOUNTER_HEIGHTS & 0xffffu,
    ENEMY_X = WRAM_BATTLE_ENEMY_SPRITE_X_POSITIONS & 0xffffu,
    ENEMY_Y = WRAM_BATTLE_ENEMY_SPRITE_Y_POSITIONS & 0xffffu,
    ENEMY_FLAGS = WRAM_BATTLE_ENEMY_GROUP_FLAGS & 0xffffu,
    ENEMY_COUNT = 6u
};

static Lufia2ExecutionResult EncounterUnwound(uint32_t site) {
    const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
    return result;
}

Lufia2ExecutionResult Lufia2BattleClearEnemySpriteSlots(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->stack < 0x1f00u ||
        cpu->stack > 0x1ffbu)
        return ExecutionHandoff(cpu, 0x85de8eu);
    OpLdx(cpu, 0u);
    OpLoadA(cpu, 0xffu);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, (WRAM_BATTLE_ENEMY_SPRITE_RECORD_SLOTS & 0xffffu)));
        OpInx(cpu);
        OpCpx(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
    return ExecutionReturned(0x85de9cu);
}

static uint32_t LoadEnemyDimensions(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t sprite_site, uint32_t size_site) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            sprite_site, 0x81fbc6u, 3u, 0x81u))
        return sprite_site;
    if (!CallChildWithFrame(memory, cpu, child, context,
            size_site, 0x81fb8eu, 3u, 0x81u))
        return size_site;
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_WIDTHS));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_HEIGHTS));
    return 0u;
}

static void StoreEnemyCount(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_ENEMY_REMAINING_COUNT & 0xffffu)));
    OpSta(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_ENEMY_INITIAL_COUNT & 0xffffu)));
}

static void ArrangeEncounterEnemies(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT));
    OpSta(memory, cpu, OpDp(cpu, DP_REMAINING_WIDTHS));
    LoadA16(cpu, cpu->direct_page);
    do {
        OpIny(cpu);
        cpu->carry = 1u;
        OpAdc(memory, cpu, OpAbsY(cpu, ENEMY_WIDTHS));
        OpStepMem(memory, cpu, OpDp(cpu, DP_REMAINING_WIDTHS), -1);
    } while (!cpu->zero);
    OpDecA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_ENEMY_COMBINED_WIDTH & 0xffffu)));
    OpLoadA(cpu, 0x18u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_ENEMY_COMBINED_WIDTH & 0xffffu))));
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, ENEMY_X));
    OpLdy(cpu, 0u);
    do {
        cpu->carry = 1u;
        OpAdc(memory, cpu, OpAbsY(cpu, ENEMY_WIDTHS));
        OpSta(memory, cpu, OpAbsY(cpu, ENEMY_X + 1u));
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT - 1u);
    } while (!cpu->zero);
    OpLdx(cpu, 0u);
    do {
        OpLsrMem(memory, cpu, OpDp(cpu, DP_ENEMY_FLAGS));
        LoadA16(cpu, cpu->direct_page);
        if (cpu->accumulator_is_8_bit)
            RolA8(cpu);
        else
            RolA16(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, ENEMY_FLAGS));
        OpInx(cpu);
        OpCpx(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_X));
        for (unsigned shift = 0u; shift < 3u; ++shift)
            OpAslA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, ENEMY_X));
        OpLoadA(cpu, 0x0cu);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu,
            OpAbsY(cpu, ENEMY_HEIGHTS)));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbsY(cpu, ENEMY_FLAGS));
        for (unsigned shift = 0u; shift < 3u; ++shift)
            OpAslA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, ENEMY_Y));
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
    OpStz(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_ACTION_SPECIAL_RESULT & 0xffffu)));
}

static uint32_t LoadFieldEncounter(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
    OpLdy(cpu, 0u);
    OpStz(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x4216u)));
    do {
        OpLda(memory, cpu, OpLongX(cpu, FIELD_ENCOUNTERS));
        OpSta(memory, cpu, OpAbsY(cpu, ENEMY_GROUPS));
        OpIncA(cpu);
        if (!cpu->zero)
            OpStepMem(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT), 1);
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpLongX(cpu, FIELD_ENCOUNTERS));
    OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_FLAGS));
    StoreEnemyCount(memory, cpu);
    OpLdy(cpu, ENEMY_COUNT - 1u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_GROUPS));
        const uint32_t unwind = LoadEnemyDimensions(memory, cpu,
            child, context, 0x818246u, 0x81824au);
        if (unwind)
            return unwind;
        OpDey(cpu);
    } while (!cpu->negative);
    ArrangeEncounterEnemies(memory, cpu);
    return 0u;
}

static uint32_t LoadSpecialEncounter(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, FIELD_BATTLE_RESOURCE);
    OpRepWidths(cpu, 0x20u);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, SPECIAL_ENCOUNTERS));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpStz(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT));
    do {
        OpLda(memory, cpu, OpLongX(cpu, SPECIAL_ENCOUNTERS));
        OpSta(memory, cpu, OpAbsY(cpu, ENEMY_GROUPS));
        OpIncA(cpu);
        if (!cpu->zero) {
            PushIndex(memory, cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_ENEMY_COUNT), 1);
            OpDecA(cpu);
            const uint32_t unwind = LoadEnemyDimensions(memory, cpu,
                child, context, 0x8182e0u, 0x8182e4u);
            if (unwind)
                return unwind;
            OpPullX(memory, cpu);
        }
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
    StoreEnemyCount(memory, cpu);
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, SPECIAL_ENCOUNTERS));
        OpSta(memory, cpu, OpAbsY(cpu, ENEMY_X));
        OpInx(cpu);
        OpLda(memory, cpu, OpLongX(cpu, SPECIAL_ENCOUNTERS));
        OpSta(memory, cpu, OpAbsY(cpu, ENEMY_Y));
        LoadA16(cpu, cpu->direct_page);
        OpSta(memory, cpu, OpAbsY(cpu, ENEMY_FLAGS));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
    OpInx(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, SPECIAL_ENCOUNTERS));
    OpSta(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_EXPERIENCE_REWARD & 0xffffu)));
    OpInx(cpu);
    OpInx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, SPECIAL_ENCOUNTERS));
    OpSta(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_GOLD_REWARD & 0xffffu)));
    OpInx(cpu);
    OpInx(cpu);
    OpSepWidths(cpu, 0x20u);
    const uint16_t destinations[] = {(WRAM_BATTLE_EFFECT_SPECIAL_PARTY & 0xffffu), (WRAM_BATTLE_SPECIAL_ENCOUNTER_FLAGS & 0xffffu), (WRAM_BATTLE_CONTROL_FLAGS & 0xffffu)};
    for (unsigned field = 0u; field < 3u; ++field) {
        OpLda(memory, cpu, OpLongX(cpu, SPECIAL_ENCOUNTERS));
        OpSta(memory, cpu, OpAbs(cpu, destinations[field]));
        OpInx(cpu);
    }
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_ACTION_SPECIAL_RESULT & 0xffffu)));
    return 0u;
}

Lufia2ExecutionResult Lufia2BattleLoadEncounter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f16u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x8181e6u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8181e6u, 0x85de8eu, 3u, 0x81u))
        return EncounterUnwound(0x8181e6u);
    OpLda(memory, cpu, FIELD_BATTLE_STYLE);
    OpSta(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_BACKGROUND_ID & 0xffffu)));
    OpLda(memory, cpu, FIELD_BATTLE_SOURCE);
    if (!cpu->zero) {
        OpIncA(cpu);
        if (cpu->zero) {
            const uint32_t unwind = LoadSpecialEncounter(memory, cpu,
                child, context);
            return unwind ? EncounterUnwound(unwind) :
                ExecutionReturned(0x818350u);
        }
        OpLda(memory, cpu, FIELD_BATTLE_RESOURCE);
        OpLdx(cpu, 0u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x818204u, 0x81839fu, 2u, 0x81u))
            return EncounterUnwound(0x818204u);
        OpLoadA(cpu, 0u);
    } else {
        OpLda(memory, cpu, FIELD_BATTLE_RESOURCE);
    }
    const uint32_t unwind = LoadFieldEncounter(memory, cpu, child, context);
    return unwind ? EncounterUnwound(unwind) : ExecutionReturned(0x8182bcu);
}
