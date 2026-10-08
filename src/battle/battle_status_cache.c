#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    PARTY_RECORDS = WRAM_BATTLE_PARTY_RECORDS & 0xffffu,
    ENEMY_RECORDS = WRAM_BATTLE_ENEMY_RECORDS & 0xffffu,
    RECORD_STATUS = 0x0fu,
    ENEMY_STATUS_BASE = 0x162au,
    ENEMY_RECORD_SIZE = 0xbeu,
    STATUS_CACHE = WRAM_BATTLE_STATUS_CACHE & 0xffffu,
    ENEMY_SPRITE_SLOTS = WRAM_BATTLE_ENEMY_SPRITE_RECORD_SLOTS & 0xffffu,
    ENEMY_GRAPHICS_GROUPS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_IDS & 0xffffu,
    ENEMY_GROUP_KEYS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_KEYS & 0xffffu
};

Lufia2ExecutionResult Lufia2BattleSnapshotStatuses(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->stack < 0x1f00u ||
        cpu->stack > 0x1ffbu)
        return ExecutionHandoff(cpu, 0x85ed51u);
    for (unsigned member = 0u; member < 5u; ++member) {
        LoadA16(cpu, cpu->direct_page);
        OpLdx(cpu, OpReadX(memory, cpu,
            OpAbs(cpu, (uint16_t)(PARTY_RECORDS + member * 2u))));
        if (!cpu->zero)
            OpLda(memory, cpu, OpAbsX(cpu, RECORD_STATUS));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(STATUS_CACHE + member)));
    }
    for (unsigned enemy = 0u; enemy < 6u; ++enemy) {
        OpLda(memory, cpu, OpAbs(cpu,
            (uint16_t)(ENEMY_STATUS_BASE + enemy * ENEMY_RECORD_SIZE)));
        OpSta(memory, cpu, OpAbs(cpu,
            (uint16_t)(STATUS_CACHE + 5u + enemy)));
    }
    return ExecutionReturned(0x85edb1u);
}

static void ClearDefeatedEnemyRecord(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_RECORDS));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, ENEMY_RECORD_SIZE);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, 0u));
        OpInx(cpu);
        OpDecA(cpu);
    } while (!cpu->zero);
    OpTya(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x4216u)));
    OpStz(memory, cpu, OpAbsX(cpu, 6u));
    OpRepWidths(cpu, 0x20u);
}

static void InvalidateEnemySprite(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpTya(cpu);
    OpLsrA(cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, ENEMY_SPRITE_SLOTS));
    OpRepWidths(cpu, 0x20u);
}

static void ReleaseUnusedGraphicsGroups(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLdy(cpu, 2u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_GRAPHICS_GROUPS));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpLdx(cpu, 5u);
            bool retained = false;
            do {
                OpLda(memory, cpu, OpAbsX(cpu, ENEMY_SPRITE_SLOTS));
                OpCmpValue(cpu, 0xffu);
                if (!cpu->zero) {
                    OpLda(memory, cpu, OpAbsY(cpu, ENEMY_GRAPHICS_GROUPS));
                    OpCmpValue(cpu, OpReadM(memory, cpu,
                        OpAbsX(cpu, ENEMY_GROUP_KEYS)));
                    if (cpu->zero) {
                        retained = true;
                        break;
                    }
                }
                OpDex(cpu);
            } while (!cpu->negative);
            if (!retained) {
                OpLoadA(cpu, 0xffu);
                OpSta(memory, cpu, OpAbsY(cpu, ENEMY_GRAPHICS_GROUPS));
            }
        }
        OpDey(cpu);
    } while (!cpu->negative);
}

Lufia2ExecutionResult Lufia2BattleReleaseDefeatedEnemies(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->stack < 0x1f02u ||
        cpu->stack > 0x1ffbu)
        return ExecutionHandoff(cpu, 0x85dfb9u);
    Push8(memory, cpu, cpu->data_bank);
    Push8(memory, cpu, cpu->program_bank);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 10u);
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, ENEMY_RECORDS)));
        if (cpu->zero) {
            InvalidateEnemySprite(memory, cpu);
        } else {
            OpLda(memory, cpu, OpAbsX(cpu, RECORD_STATUS));
            OpAndValue(cpu, 4u);
            if (!cpu->zero) {
                ClearDefeatedEnemyRecord(memory, cpu);
                InvalidateEnemySprite(memory, cpu);
            }
        }
        OpDey(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    ReleaseUnusedGraphicsGroups(memory, cpu);
    cpu->data_bank = Pull8(memory, cpu);
    SetNz8(cpu, cpu->data_bank);
    return ExecutionReturned(0x85e02cu);
}
