#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    CHARACTER_STATUS = 0x0bbcu,
    CHARACTER_STRIDE = 0xbeu,
    CHARACTER_COUNT = 7u,
    CAPSULE_STATUS = 0x10eeu,
    SUBMENU_CHOICES = WRAM_BATTLE_SUBMENU_CHOICES & 0xffffu,
    SAVED_SUBMENU_CHOICES = WRAM_BATTLE_SAVED_SUBMENU_CHOICES,
    SUBMENU_RECORD_BYTES = 7u,
    SAVED_RECORD_BYTES = 4u,
    DP_CHOICE_OFFSET = 0u,
    ENEMY_POINTERS = 0x0a6eu,
    ENEMY_STATUS = 0x0fu,
    ENEMY_GROUP = 0x50u,
    REMAINING_GROUPS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_KEYS & 0xffffu,
    GROUP_FLAGS = WRAM_BATTLE_ENEMY_GROUP_FLAGS & 0xffffu,
    ENEMY_COUNT = 6u,
    DP_ENCOUNTER_FLAGS = 0u,
    FIELD_BATTLE_SOURCE = WRAM_FIELD_BATTLE_SOURCE,
    FIELD_CONTACT_RESOURCE = WRAM_FIELD_CONTACT_RESOURCE,
    FIELD_ENCOUNTERS = WRAM_FIELD_ENCOUNTER_RECORDS,
    FIELD_RECORD_BYTES = 10u
};

static uint8_t EncounterContext(const Lufia2CpuState *cpu,
    uint16_t minimum_stack) {
    return cpu->program_bank == 0x85u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= minimum_stack && cpu->stack <= 0x1ffbu;
}

Lufia2ExecutionResult Lufia2BattleRetainPersistentStatuses(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EncounterContext(cpu, 0x1f00u))
        return ExecutionHandoff(cpu, 0x85edbbu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xfffau);
    for (unsigned member = 0u; member < CHARACTER_COUNT; ++member)
        OpTestBits(memory, cpu,
            OpAbs(cpu, (uint16_t)(CHARACTER_STATUS + CHARACTER_STRIDE * member)), 0u);
    OpStz(memory, cpu, OpAbs(cpu, CAPSULE_STATUS));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x85eddau);
}

static void AdvanceSubmenuRecord(Lufia2CpuState *cpu) {
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, SUBMENU_RECORD_BYTES);
    OpTay(cpu);
    for (unsigned byte = 0u; byte < SAVED_RECORD_BYTES; ++byte)
        OpInx(cpu);
    OpCpx(cpu, CHARACTER_COUNT * SAVED_RECORD_BYTES);
}

Lufia2ExecutionResult Lufia2BattleLoadSavedSubmenuChoices(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EncounterContext(cpu, 0x1f00u))
        return ExecutionHandoff(cpu, 0x85edf1u);
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        OpLda(memory, cpu, OpLongX(cpu, SAVED_SUBMENU_CHOICES + 1u));
        OpSta(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES));
        OpLda(memory, cpu, OpLongX(cpu, SAVED_SUBMENU_CHOICES + 2u));
        OpSta(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 2u));
        OpLda(memory, cpu, OpLongX(cpu, SAVED_SUBMENU_CHOICES + 3u));
        OpSta(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 5u));
        OpLda(memory, cpu, OpLongX(cpu, SAVED_SUBMENU_CHOICES));
        OpAndValue(cpu, 0x0fu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 2u));
        OpSta(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 3u));
        OpLda(memory, cpu, OpLongX(cpu, SAVED_SUBMENU_CHOICES));
        for (unsigned bit = 0u; bit < 4u; ++bit)
            OpLsrA(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES));
        OpSta(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 1u));
        LoadA16(cpu, cpu->direct_page);
        OpSta(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 4u));
        OpSta(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 6u));
        AdvanceSubmenuRecord(cpu);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x85ee3du);
}

Lufia2ExecutionResult Lufia2BattleStoreSavedSubmenuChoices(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EncounterContext(cpu, 0x1f00u))
        return ExecutionHandoff(cpu, 0x85ee3eu);
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 1u));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES)));
        for (unsigned bit = 0u; bit < 4u; ++bit)
            OpAslA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_CHOICE_OFFSET));
        OpLda(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 3u));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 2u)));
        OpOra(memory, cpu, OpDp(cpu, DP_CHOICE_OFFSET));
        OpSta(memory, cpu, OpLongX(cpu, SAVED_SUBMENU_CHOICES));
        OpLda(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES));
        OpSta(memory, cpu, OpLongX(cpu, SAVED_SUBMENU_CHOICES + 1u));
        OpLda(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 2u));
        OpSta(memory, cpu, OpLongX(cpu, SAVED_SUBMENU_CHOICES + 2u));
        OpLda(memory, cpu, OpAbsY(cpu, SUBMENU_CHOICES + 5u));
        OpSta(memory, cpu, OpLongX(cpu, SAVED_SUBMENU_CHOICES + 3u));
        AdvanceSubmenuRecord(cpu);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x85ee81u);
}

static void RotateEncounterFlag(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint32_t address = DirectAddress(cpu, DP_ENCOUNTER_FLAGS);
    const uint8_t previous = Read8(memory, address);
    const uint8_t flags = (uint8_t)((previous >> 1) |
        (cpu->carry ? 0x80u : 0u));
    cpu->carry = previous & 1u;
    Write8(memory, address, flags);
    SetNz8(cpu, flags);
}

static void CollectRemainingEnemies(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, DP_ENCOUNTER_FLAGS));
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    do {
        PushY(memory, cpu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_POINTERS));
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, ENEMY_STATUS));
            OpAndValue(cpu, 4u);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsY(cpu, ENEMY_GROUP));
                OpSta(memory, cpu, OpAbsX(cpu, REMAINING_GROUPS));
                OpRepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpStack(cpu, 1u));
                OpLsrA(cpu);
                OpTay(cpu);
                OpSepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpAbsY(cpu, GROUP_FLAGS));
                OpLsrA(cpu);
                RotateEncounterFlag(memory, cpu);
                OpInx(cpu);
            }
        }
        OpPullY(memory, cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT * 2u);
    } while (!cpu->zero);
    OpTxy(cpu);
    OpLoadA(cpu, 0xffu);
    for (;;) {
        OpCpx(cpu, ENEMY_COUNT);
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpAbsX(cpu, REMAINING_GROUPS));
        OpLsrMem(memory, cpu, OpDp(cpu, DP_ENCOUNTER_FLAGS));
        OpInx(cpu);
    }
    OpLsrMem(memory, cpu, OpDp(cpu, DP_ENCOUNTER_FLAGS));
    OpLsrMem(memory, cpu, OpDp(cpu, DP_ENCOUNTER_FLAGS));
}

static void StoreFieldEncounter(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, FIELD_CONTACT_RESOURCE);
    OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
    OpLoadA(cpu, FIELD_RECORD_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
    OpLda(memory, cpu, OpDp(cpu, DP_ENCOUNTER_FLAGS));
    OpSta(memory, cpu, OpAbsX(cpu, REMAINING_GROUPS));
    OpLdy(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x4216u)));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, REMAINING_GROUPS));
        OpSta(memory, cpu, OpLongX(cpu, FIELD_ENCOUNTERS));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT + 1u);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2BattleStoreRemainingEncounter(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EncounterContext(cpu, 0x1f04u))
        return ExecutionHandoff(cpu, 0x85eea1u);
    Push8(memory, cpu, cpu->data_bank);
    Push8(memory, cpu, cpu->program_bank);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    OpLda(memory, cpu, FIELD_BATTLE_SOURCE);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero) {
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            CollectRemainingEnemies(memory, cpu);
            StoreFieldEncounter(memory, cpu);
        }
    }
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    return ExecutionReturned(0x85ef1fu);
}
