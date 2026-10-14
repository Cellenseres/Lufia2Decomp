#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
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

enum {
    DP_ENEMY_GRAPHICS_ID = 0x22u,
    ENEMY_GRAPHICS_RECORD_ID = 0x50u,
    ENEMY_GRAPHICS_GROUP_EMPTY = 0xffu
};

static uint8_t EnemyGroupContextReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x85u && cpu->data_bank == 0x85u &&
        cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->direct_page && !cpu->decimal;
}

static Lufia2ExecutionResult EnemyGroupChildUnwound(uint32_t site) {
    const Lufia2ExecutionResult result = {
        LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
    return result;
}

static uint8_t FindEnemyGraphicsGroup(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_SPRITE_GROUP_IDS));
        OpCmpValue(cpu, ENEMY_GRAPHICS_GROUP_EMPTY);
        if (cpu->zero)
            return 1u;
        OpCmp(memory, cpu, OpDp(cpu, DP_ENEMY_GRAPHICS_ID));
        if (cpu->zero)
            return 1u;
        OpInx(cpu);
        OpCpx(cpu, WRAM_BATTLE_ENEMY_SPRITE_GROUP_IDS_COUNT);
        if (cpu->zero)
            return 0u;
    }
}

static void StoreEnemyGroupDimensions(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0101u);
    OpLsrA(cpu);
    OpAndValue(cpu, 0x7f7fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_SPRITE_GROUP_WIDTHS));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_SPRITE_GROUP_HEIGHTS));
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_GROUP_TILE_COUNTS));
}

Lufia2ExecutionResult Lufia2BattleBuildEnemyGraphicsGroups(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x85df44u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, ENEMY_GRAPHICS_GROUP_EMPTY);
    for (uint16_t group = 0u; group < WRAM_BATTLE_ENEMY_SPRITE_GROUP_IDS_COUNT; ++group)
        OpSta(memory, cpu, OpAbs(cpu,
            (uint16_t)(WRAM_BATTLE_ENEMY_SPRITE_GROUP_IDS + group)));
    OpLdy(cpu, 0u);
    unsigned visits = 0u;
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_RECORDS)));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, ENEMY_GRAPHICS_RECORD_ID));
            OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_GRAPHICS_ID));
            if (!FindEnemyGraphicsGroup(memory, cpu))
                return ExecutionHandoff(cpu, 0x85df73u);
            OpLda(memory, cpu, OpDp(cpu, DP_ENEMY_GRAPHICS_ID));
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_SPRITE_GROUP_IDS));
            if (!CallChildWithFrame(memory, cpu, child, context,
                                   0x85df79u, 0x81fbc6u, 3u, 0x85u))
                return EnemyGroupChildUnwound(0x85df79u);
            if (!EnemyGroupContextReady(cpu))
                return ExecutionHandoff(cpu, 0x85df7du);
            OpDecA(cpu);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_GROUP_GRAPHICS));
            ExchangeAccumulatorBytes(cpu);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_GROUP_PALETTE_IDS));
            ExchangeAccumulatorBytes(cpu);
            OpIncA(cpu);
            PushAccumulator8(memory, cpu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                                   0x85df88u, 0x81fb8eu, 3u, 0x85u))
                return EnemyGroupChildUnwound(0x85df88u);
            if (!EnemyGroupContextReady(cpu))
                return ExecutionHandoff(cpu, 0x85df8cu);
            StoreEnemyGroupDimensions(memory, cpu);
            LoadA8(cpu, Pull8(memory, cpu));
            if (!CallChildWithFrame(memory, cpu, child, context,
                                   0x85dfa9u, 0x81fba2u, 3u, 0x85u))
                return EnemyGroupChildUnwound(0x85dfa9u);
            if (!EnemyGroupContextReady(cpu))
                return ExecutionHandoff(cpu, 0x85dfadu);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_SPRITE_GROUP_ATTRIBUTES));
        }
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, WRAM_BATTLE_ENEMY_RECORDS_COUNT * 2u);
        if (!cpu->zero && ++visits == 4096u)
            return ExecutionHandoff(cpu, 0x85df55u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85dfb8u);
}
