#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_TILE_ROW = 0u,
    DP_PALETTE_ID = 0u,
    DP_PALETTE_BYTES = 1u,
    DP_TILE_FACTORS = 2u,
    DP_GRAPHICS_ID = 4u,
    DP_TILE_SOURCE = 8u,
    DP_TILE_TARGET = 11u,
    DP_RESOURCE_ID = 0x54u,
    DP_RESOURCE_TARGET = 0x60u,
    DP_RESOURCE_BANK = 0x62u,
    GROUP_IDS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_IDS & 0xffffu,
    GROUP_GRAPHICS = WRAM_BATTLE_ENEMY_GROUP_GRAPHICS & 0xffffu,
    GROUP_PALETTES = WRAM_BATTLE_ENEMY_GROUP_PALETTE_IDS & 0xffffu,
    GROUP_PALETTE_SLOTS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_PALETTES & 0xffffu,
    GROUP_TILE_ROWS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_TILES & 0xffffu,
    PALETTES_USED = WRAM_BATTLE_ENEMY_PALETTES_USED & 0xffffu,
    CACHED_TILE_SETS = WRAM_BATTLE_ENEMY_CACHED_TILE_SETS & 0xffffu,
    TILE_ROWS_USED = WRAM_BATTLE_ENEMY_TILE_ROWS_USED & 0xffffu,
    GROUP_COUNT = 3u,
    GRAPHICS_RESOURCES = 0x1b7u,
    GRAPHICS_SOURCE = 0xdf00u,
    GRAPHICS_TARGET = 0x6000u,
    PALETTE_TARGET = 0x4820u,
    PALETTE_BYTES = 32u,
    MULTIPLIER_A = 0x4202u,
    MULTIPLIER_B = 0x4203u,
    MULTIPLIER_RESULT = 0x4216u
};

static Lufia2ExecutionResult GraphicsChildUnwound(uint32_t site) {
    const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
    return result;
}

static uint8_t FindCachedEnemyTiles(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    for (;;) {
        OpCpx(cpu, OpReadX(memory, cpu, OpAbs(cpu, CACHED_TILE_SETS)));
        if (cpu->zero)
            return 0u;
        OpLda(memory, cpu, OpAbsY(cpu, GROUP_IDS + 1u));
        OpCmp(memory, cpu, OpDp(cpu, DP_GRAPHICS_ID));
        if (cpu->zero)
            return 1u;
        for (unsigned byte = 0u; byte < 4u; ++byte)
            OpIny(cpu);
        OpInx(cpu);
    }
}

static void PrepareEnemyTileBlit(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0101u);
    OpLsrA(cpu);
    OpAndValue(cpu, 0x7f7fu);
    OpSta(memory, cpu, OpDp(cpu, DP_TILE_FACTORS));
    OpLdy(cpu, 0u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, PALETTE_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLIER_A));
    OpLda(memory, cpu, 0x7edf00u);
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLIER_B));
    OpLdx(cpu, GRAPHICS_TARGET);
    OpWriteX(memory, cpu, OpDp(cpu, DP_TILE_TARGET), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, GRAPHICS_SOURCE + 1u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, MULTIPLIER_RESULT));
    OpSta(memory, cpu, OpDp(cpu, DP_TILE_SOURCE));
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2BattleAllocateEnemyTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f08u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x85ebe0u);
    OpPushX(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_GRAPHICS_ID));
    if (FindCachedEnemyTiles(memory, cpu)) {
        OpLda(memory, cpu, OpAbsY(cpu, GROUP_GRAPHICS));
        OpRepWidths(cpu, 0x10u);
        OpPullX(memory, cpu);
        return ExecutionReturned(0x85ec4cu);
    }
    OpRepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpAbs(cpu, TILE_ROWS_USED));
    PushAccumulator8(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_TILE_ROW));
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpDp(cpu, DP_GRAPHICS_ID));
    OpRepWidths(cpu, 0x20u);
    OpIncA(cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85ec09u, 0x81fb8eu, 3u, 0x85u))
        return GraphicsChildUnwound(0x85ec09u);
    PrepareEnemyTileBlit(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85ec3au, 0x81bcccu, 3u, 0x85u))
        return GraphicsChildUnwound(0x85ec3au);
    OpLda(memory, cpu, OpDp(cpu, DP_TILE_ROW));
    OpSta(memory, cpu, OpAbs(cpu, TILE_ROWS_USED));
    LoadA8(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    return ExecutionReturned(0x85ec45u);
}

Lufia2ExecutionResult Lufia2BattleAllocateEnemyPalette(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f06u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x85ec4du);
    OpPushX(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_ID));
    OpLoadA(cpu, PALETTE_BYTES);
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLIER_A));
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_BYTES));
    OpLda(memory, cpu, OpAbs(cpu, PALETTES_USED));
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLIER_B));
    PushAccumulator8(memory, cpu);
    PushDataBank(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, MULTIPLIER_RESULT)));
    OpLda(memory, cpu, OpDp(cpu, DP_PALETTE_ID));
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLIER_B));
    OpStepMem(memory, cpu, OpAbs(cpu, PALETTES_USED), 1);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, MULTIPLIER_RESULT)));
    OpSetDataBank(memory, cpu, 0x7eu);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, GRAPHICS_SOURCE + 1u));
        OpSta(memory, cpu, OpAbsX(cpu, PALETTE_TARGET));
        OpIny(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_PALETTE_BYTES), -1);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    return ExecutionReturned(0x85ec80u);
}

static void PrepareEnemyGraphicsResource(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0u;
    OpAdcValue(cpu, GRAPHICS_RESOURCES);
    OpSta(memory, cpu, OpDp(cpu, DP_RESOURCE_ID));
    OpLdx(cpu, GRAPHICS_SOURCE);
    OpWriteX(memory, cpu, OpDp(cpu, DP_RESOURCE_TARGET), cpu->x);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DP_RESOURCE_BANK));
}

Lufia2ExecutionResult Lufia2BattleLoadEnemyGraphicsGroups(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffbu || !child)
        return ExecutionHandoff(cpu, 0x85eb91u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, PALETTES_USED));
    OpStz(memory, cpu, OpAbs(cpu, CACHED_TILE_SETS));
    OpStz(memory, cpu, OpAbs(cpu, TILE_ROWS_USED));
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, GROUP_IDS));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, GROUP_GRAPHICS));
            OpPushX(memory, cpu);
            PrepareEnemyGraphicsResource(memory, cpu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x85ebc1u, 0x808e9du, 3u, 0x85u))
                return GraphicsChildUnwound(0x85ebc1u);
            OpPullX(memory, cpu);
            OpLda(memory, cpu, OpAbsX(cpu, GROUP_GRAPHICS));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x85ebc9u, 0x85ebe0u, 2u, 0x85u))
                return GraphicsChildUnwound(0x85ebc9u);
            OpSta(memory, cpu, OpAbsX(cpu, GROUP_TILE_ROWS));
            OpLda(memory, cpu, OpAbsX(cpu, GROUP_PALETTES));
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x85ebd2u, 0x85ec4du, 2u, 0x85u))
                return GraphicsChildUnwound(0x85ebd2u);
            OpSta(memory, cpu, OpAbsX(cpu, GROUP_PALETTE_SLOTS));
        }
        OpInx(cpu);
        OpCpx(cpu, GROUP_COUNT);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85ebdfu);
}
