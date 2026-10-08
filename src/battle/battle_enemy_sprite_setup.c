#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_SLOTS_LEFT = 0u,
    DP_SPRITE_SLOT = 1u,
    DP_ENEMY_SLOT = 2u,
    ENEMY_COUNT = 6u,
    GROUP_COUNT = 3u,
    ENEMY_RECORDS = 0x0a6eu,
    GROUP_IDS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_IDS & 0xffffu,
    GROUP_PALETTES = WRAM_BATTLE_ENEMY_SPRITE_GROUP_PALETTES & 0xffffu,
    GROUP_TILES = WRAM_BATTLE_ENEMY_SPRITE_GROUP_TILES & 0xffffu,
    GROUP_WIDTHS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_WIDTHS & 0xffffu,
    GROUP_HEIGHTS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_HEIGHTS & 0xffffu,
    GROUP_ATTRIBUTES = WRAM_BATTLE_ENEMY_SPRITE_GROUP_ATTRIBUTES & 0xffffu,
    ENEMY_GROUP_SLOTS = WRAM_BATTLE_ENEMY_SPRITE_RECORD_SLOTS & 0xffffu,
    ENEMY_X = WRAM_BATTLE_ENEMY_SPRITE_X_POSITIONS & 0xffffu,
    ENEMY_Y = WRAM_BATTLE_ENEMY_SPRITE_Y_POSITIONS & 0xffffu,
    ENEMY_GROUP_IDS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_KEYS & 0xffffu,
    SPRITE_RECORDS = WRAM_BATTLE_EFFECT_PARTY_POSITIONS,
    SPRITE_RECORD_BYTES = 15u,
    ENEMY_TIMERS = WRAM_BATTLE_ENEMY_SPRITE_TIMERS & 0xffffu
};

static Lufia2ExecutionResult SpriteChildUnwound(uint32_t site) {
    const Lufia2ExecutionResult result =
        {LUFIA2_EXECUTION_CHILD_UNWOUND, site};
    return result;
}

static void InitializeEnemyPosition(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + 1u));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + 2u));
    OpLda(memory, cpu, OpAbsX(cpu, ENEMY_X));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + 5u));
    OpLda(memory, cpu, OpAbsX(cpu, ENEMY_Y));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + 7u));
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + 6u));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + 8u));
    OpRepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + 9u));
    OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + 11u));
    OpSepWidths(cpu, 0x20u);
    OpTxa(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, ENEMY_TIMERS));
}

static void CopyEnemySpriteGroup(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t sources[] = {
        GROUP_ATTRIBUTES, GROUP_PALETTES, GROUP_WIDTHS,
        GROUP_HEIGHTS, GROUP_TILES
    };
    static const uint8_t offsets[] = {0u, 3u, 13u, 14u, 4u};
    for (unsigned field = 0u; field < 5u; ++field) {
        OpLda(memory, cpu, OpAbsX(cpu, sources[field]));
        OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + offsets[field]));
    }
}

Lufia2ExecutionResult Lufia2BattleFindEnemySpriteGroup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffbu)
        return ExecutionHandoff(cpu, 0x85df35u);
    OpLdx(cpu, 0u);
    do {
        OpCmp(memory, cpu, OpAbsX(cpu, GROUP_IDS));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpCpx(cpu, GROUP_COUNT);
    } while (!cpu->zero);
    return ExecutionReturned(0x85df43u);
}

Lufia2ExecutionResult Lufia2BattleInitializeEnemySpriteRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f06u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x85dedcu);
    InitializeEnemyPosition(memory, cpu);
    OpPushX(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_SPRITE_SLOT));
    OpSta(memory, cpu, OpAbsX(cpu, ENEMY_GROUP_SLOTS));
    OpLda(memory, cpu, OpAbsX(cpu, ENEMY_GROUP_IDS));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85df11u, 0x85df35u, 3u, 0x85u))
        return SpriteChildUnwound(0x85df11u);
    CopyEnemySpriteGroup(memory, cpu);
    OpPullX(memory, cpu);
    return ExecutionReturned(0x85df34u);
}

Lufia2ExecutionResult Lufia2BattleInitializeEnemySpriteRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f0au || cpu->stack > 0x1ffbu || !child)
        return ExecutionHandoff(cpu, 0x85de9du);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0u);
    OpLoadA(cpu, ENEMY_COUNT);
    OpSta(memory, cpu, OpDp(cpu, DP_SLOTS_LEFT));
    OpStz(memory, cpu, OpDp(cpu, DP_SPRITE_SLOT));
    OpLdx(cpu, 0u);
    OpLdy(cpu, 0u);
    do {
        OpWriteX(memory, cpu, OpDp(cpu, DP_ENEMY_SLOT), cpu->x);
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, ENEMY_RECORDS));
        if (cpu->zero) {
            LoadA16(cpu, cpu->direct_page);
            OpSta(memory, cpu, OpAbsY(cpu, SPRITE_RECORDS + 1u));
        } else {
            OpSepWidths(cpu, 0x20u);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ENEMY_SLOT)));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x85debeu, 0x85dedcu, 2u, 0x85u))
                return SpriteChildUnwound(0x85debeu);
        }
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, SPRITE_RECORD_BYTES);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_SPRITE_SLOT), 1);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ENEMY_SLOT)));
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_SLOTS_LEFT), -1);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85dedbu);
}
