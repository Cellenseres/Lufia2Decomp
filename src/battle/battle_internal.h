#ifndef LUFIA2_BATTLE_BATTLE_INTERNAL_H
#define LUFIA2_BATTLE_BATTLE_INTERNAL_H

#include <stdbool.h>

#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"

/* ROM routines reached by battle child calls. */
enum {
    BATTLE_ROUTINE_DECOMPRESS_RESOURCE = 0x808e9du,
    BATTLE_ROUTINE_BUILD_SPRITES = 0x81b5c4u,
    BATTLE_ROUTINE_LOAD_PALETTE = 0x81b974u,
    BATTLE_ROUTINE_COMMIT_PALETTES = 0x81b9afu,
    BATTLE_ROUTINE_TILE_BLOCK = 0x81be58u,
    BATTLE_ROUTINE_RESET_PARTY_TILEMAP = 0x81c2e3u,
    BATTLE_ROUTINE_CLEAR_WINDOW_TILEMAP = 0x81c2fbu,
    BATTLE_ROUTINE_CHOOSE_COMMAND = 0x81cb77u,
    BATTLE_ROUTINE_CHOOSE_PARTY_ACTION = 0x81cc2eu,
    BATTLE_ROUTINE_CHOOSE_TARGETS = 0x81d4e0u,
    BATTLE_ROUTINE_PARTY_WINDOWS = 0x81df0au,
    BATTLE_ROUTINE_SPRITES = 0x858a2fu,
    BATTLE_ROUTINE_CLEAR_SPRITE_OFFSETS = 0x8589e5u,
    BATTLE_ROUTINE_SYNC_STATUS_MARKERS = 0x8591a1u,
    BATTLE_ROUTINE_QUEUE_STATUS_SPRITES = 0x859bdau,
    BATTLE_ROUTINE_RANDOM_FRACTION = 0x85dceau,
    BATTLE_ROUTINE_FRAME_INPUT = 0x85ec81u,
};

/* Set $FF to rebuild battle sprites next frame. */
enum {
    BATTLE_SPRITE_REBUILD_REQUEST = 0x0012f3u,
};

/* Sprite build mode and tile grid hold switches. */
enum {
    BATTLE_SPRITE_MODE = 0x15abu,
    BATTLE_TILE_GRID_HOLD = 0x125fu, /* non-zero: leave the tile grid alone */
};

enum {
    BATTLE_FRAME_STATE = 0x129au,
    BATTLE_SAVED_ENTRY_STACK = 0x1395u,
    BATTLE_SAVED_LOOP_STACK = 0x1397u,
};

/* Battle pad word; high byte holds the directions. */
enum {
    BATTLE_DP_PAD_FILTERED = 0xddu,
    BATTLE_DP_PAD_FILTERED_HIGH = 0xdeu,
};

enum {
    BATTLE_CONTROL_MODE_1 = 0x01u,
    BATTLE_CONTROL_MODE_2 = 0x02u,
    BATTLE_CONTROL_FINISHED = 0x80u,
    BATTLE_BACKGROUND_BLANK = 0x18u,
};

enum {
    BATTLE_PARTY_SIZE = WRAM_BATTLE_PARTY_RECORDS_COUNT,
    BATTLE_ENEMY_COUNT = WRAM_BATTLE_ENEMY_RECORDS_COUNT,
    BATTLE_PARTY_TARGET_COUNT = WRAM_BATTLE_PARTY_TARGETS_COUNT,
    BATTLE_POINTER_SIZE = 2u,
    BATTLE_TURN_ENTRY_SIZE = 3u,
    BATTLE_TARGET_RECORD_SIZE = 4u,
    BATTLE_SPRITE_SOURCE_SIZE = 5u,
    BATTLE_OAM_ENTRY_SIZE = 4u,
    BATTLE_STATUS_ICON_RECORD_SIZE = 4u,
    BATTLE_STATUS_SPRITE_RECORD_SIZE = 13u,
    BATTLE_BATTLER_STATUS = 0x0fu,
    BATTLE_BATTLER_BASE_PRIORITY = 0x2fu,
    BATTLE_BATTLER_PRIORITY_BONUS = 0x3du,
    BATTLE_STATUS_DOWNED = 0x04u,
    BATTLE_STATUS_TURN_SELECTED = 0x10u,
    BATTLE_STATUS_NO_TURN_MASK = 0x2cu,
    BATTLE_STATUS_NO_COMMAND_MASK = 0x3cu,
    BATTLE_ACTOR_ENEMY_SIDE = 0x80u,
    BATTLE_ACTOR_CAPSULE = 0x10u,
    BATTLE_ACTION_TARGET_MASK = WRAM_BATTLE_STAGED_ACTION + 2u,
    BATTLE_ACTION_TYPE = WRAM_BATTLE_STAGED_ACTION + 4u,
    BATTLE_ACTION_PARAMETER = WRAM_BATTLE_STAGED_ACTION + 6u,
    BATTLE_ACTION_PRIORITY = WRAM_BATTLE_STAGED_ACTION + 8u,
    BATTLE_ICON_RECORD_STATUS = WRAM_BATTLE_STATUS_ICON_RECORDS + 1u,
    BATTLE_ICON_RECORD_TIMER = WRAM_BATTLE_STATUS_ICON_RECORDS + 2u,
    BATTLE_ICON_TIMER = WRAM_BATTLE_ICON_STATE,
    BATTLE_ICON_CYCLE_INDEX = WRAM_BATTLE_ICON_STATE + 1u,
    BATTLE_RESULT_WINDOW_Y = WRAM_BATTLE_RESULT_WINDOW + 1u,
    BATTLE_RESULT_WINDOW_WIDTH = WRAM_BATTLE_RESULT_WINDOW + 2u,
    BATTLE_RESULT_WINDOW_HEIGHT = WRAM_BATTLE_RESULT_WINDOW + 3u,
    BATTLE_RESULT_COLUMN = WRAM_BATTLE_RESULT_WINDOW + 4u,
    BATTLE_RESULT_ROW = WRAM_BATTLE_RESULT_WINDOW + 5u,
    BATTLE_RESULT_WINDOW_BASE = WRAM_BATTLE_RESULT_WINDOW + 6u,
    BATTLE_RESULT_WRITE_POSITION = WRAM_BATTLE_RESULT_WINDOW + 8u,
    BATTLE_RESULT_TILE_ATTRIBUTES = WRAM_BATTLE_RESULT_WINDOW + 10u,
};

enum {
    BATTLE_PARTY_COMMAND_SPELL = 0u,
    BATTLE_PARTY_COMMAND_ATTACK = 3u,
    BATTLE_PARTY_COMMAND_IP = 6u,
    BATTLE_PARTY_COMMAND_ITEM = 9u,
    BATTLE_PARTY_COMMAND_DEFEND = 12u,
    BATTLE_ACTION_NONE = 0u,
    BATTLE_ACTION_ATTACK = 1u,
    BATTLE_ACTION_SPELL = 2u,
    BATTLE_ACTION_ITEM = 3u,
    BATTLE_ACTION_DEFEND = 4u,
    BATTLE_ACTION_COLLECTIVE = 6u,
    BATTLE_ACTION_IP = 8u,
    BATTLE_ACTION_SELECTED_TURN = 0x0fu,
};

typedef struct BattleContext {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *child_context;
    uint8_t return_bank;
    uint32_t unwind_site;
} BattleContext;

void BattleCallRandomFraction(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              uint16_t return_address);

static inline BattleContext BattleContextCreate(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *child_context,
                                                uint8_t return_bank) {
    return (BattleContext){memory, cpu, child, child_context, return_bank, 0u};
}

static inline bool BattleCall(BattleContext *battle, uint16_t site, uint32_t target,
                              uint8_t frame_size) {
    const uint32_t full_site = ((uint32_t)battle->return_bank << 16) | site;

    if (frame_size == 2u)
        SimulateJsrFrame(battle->memory, battle->cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(battle->memory, battle->cpu, battle->return_bank,
                         (uint16_t)(site + 3u));

    if (battle->child(battle->child_context, battle->cpu, target, full_site,
                      frame_size))
        return true;

    battle->unwind_site = full_site;
    return false;
}

static inline Lufia2ExecutionResult BattleChildUnwound(const BattleContext *battle) {
    Lufia2ExecutionResult result = ExecutionReturned(battle->unwind_site);

    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static inline bool BattleControlHas(BattleContext *battle, uint8_t mask) {
    OpLda(battle->memory, battle->cpu, OpAbs(battle->cpu, WRAM_BATTLE_CONTROL_FLAGS));
    OpBitValue(battle->cpu, mask);
    return !battle->cpu->zero;
}

static inline bool BattleRunSetup(BattleContext *battle) {
    return BattleCall(battle, 0x884fu, 0x818000u, 2u);
}

static inline bool BattleRunMainLoop(BattleContext *battle) {
    return BattleCall(battle, 0x8852u, 0x81886fu, 2u);
}

static inline bool BattleRunExit(BattleContext *battle) {
    return BattleCall(battle, 0x8861u, 0x81876bu, 2u);
}

static inline bool BattleRunDisplaySetup(BattleContext *battle) {
    return BattleCall(battle, 0x81b9u, 0x81851eu, 2u);
}

static inline bool BattlePrepareBackground(BattleContext *battle) {
    return BattleCall(battle, 0x86c0u, 0x81b9c7u, 3u);
}

static inline bool BattleRunFrameUpkeep(BattleContext *battle) {
    return BattleCall(battle, 0x8877u, 0x85ecf0u, 3u);
}

static inline bool BattleLoadPortraits(BattleContext *battle) {
    return BattleCall(battle, 0x8661u, 0x81bae8u, 3u);
}

static inline bool BattleLoadPalette(BattleContext *battle, uint16_t site) {
    return BattleCall(battle, site, BATTLE_ROUTINE_LOAD_PALETTE, 2u);
}

static inline bool BattleCommitPalettes(BattleContext *battle) {
    return BattleCall(battle, 0x86e4u, BATTLE_ROUTINE_COMMIT_PALETTES, 3u);
}

static inline bool BattleLoadDisplayDefaults(BattleContext *battle) {
    return BattleCall(battle, 0x8624u, 0x81c2c0u, 2u);
}

static inline bool BattleClearBackgroundTilemap(BattleContext *battle) {
    return BattleCall(battle, 0x8541u, 0x81c2d0u, 2u);
}

static inline bool BattleResetPartyTilemap(BattleContext *battle) {
    return BattleCall(battle, 0x8544u, BATTLE_ROUTINE_RESET_PARTY_TILEMAP, 2u);
}

static inline bool BattleClearWindowTilemapForSetup(BattleContext *battle) {
    return BattleCall(battle, 0x8547u, BATTLE_ROUTINE_CLEAR_WINDOW_TILEMAP, 2u);
}

static inline bool BattleClearTilemap3800ForSetup(BattleContext *battle) {
    return BattleCall(battle, 0x854au, 0x81c30eu, 2u);
}

static inline bool BattleClearWindowTilemapForExit(BattleContext *battle) {
    return BattleCall(battle, 0x8796u, BATTLE_ROUTINE_CLEAR_WINDOW_TILEMAP, 2u);
}

static inline bool BattleClearTilemap3800ForExit(BattleContext *battle) {
    return BattleCall(battle, 0x8799u, 0x81c30eu, 2u);
}

static inline bool BattleDecompressResource(BattleContext *battle, uint16_t site) {
    return BattleCall(battle, site, BATTLE_ROUTINE_DECOMPRESS_RESOURCE, 3u);
}

static inline bool BattleCreatePartyRecord(BattleContext *battle) {
    return BattleCall(battle, 0x80fau, 0x81fc0bu, 3u);
}

static inline bool BattleBuildSprites(BattleContext *battle) {
    return BattleCall(battle, 0x81c0u, BATTLE_ROUTINE_SPRITES, 3u);
}

static inline bool BattleRandomScale(BattleContext *battle, uint16_t site) {
    return BattleCall(battle, site, 0x808299u, 3u);
}

#endif
