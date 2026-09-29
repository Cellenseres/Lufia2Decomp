#ifndef LUFIA2_BATTLE_BATTLE_INTERNAL_H
#define LUFIA2_BATTLE_BATTLE_INTERNAL_H

#include <stdbool.h>

#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    BATTLE_FRAME_STATE = 0x129au,
    BATTLE_SAVED_ENTRY_STACK = 0x1395u,
    BATTLE_SAVED_LOOP_STACK = 0x1397u,
};

enum {
    BATTLE_CONTROL_MODE_1 = 0x01u,
    BATTLE_CONTROL_MODE_2 = 0x02u,
    BATTLE_CONTROL_FINISHED = 0x80u,
    BATTLE_BACKGROUND_BLANK = 0x18u,
};

typedef struct BattleContext {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *child_context;
    uint8_t return_bank;
    uint32_t unwind_site;
} BattleContext;

static inline BattleContext BattleContextCreate(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *child_context,
                                                uint8_t return_bank) {
    return (BattleContext){memory, cpu, child, child_context, return_bank, 0u};
}

static inline bool BattleCall(BattleContext *battle, uint16_t site,
                              uint32_t target, uint8_t frame_size) {
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
    OpLda(battle->memory, battle->cpu,
          OpAbs(battle->cpu, WRAM_BATTLE_CONTROL_FLAGS));
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
    return BattleCall(battle, site, 0x81b974u, 2u);
}

static inline bool BattleCommitPalettes(BattleContext *battle) {
    return BattleCall(battle, 0x86e4u, 0x81b9afu, 3u);
}

static inline bool BattleClear2000(BattleContext *battle) {
    return BattleCall(battle, 0x8541u, 0x81c2d0u, 2u);
}

static inline bool BattleFill2800(BattleContext *battle) {
    return BattleCall(battle, 0x8544u, 0x81c2e3u, 2u);
}

static inline bool BattleClear3000(BattleContext *battle, uint16_t site) {
    return BattleCall(battle, site, 0x81c2fbu, 2u);
}

static inline bool BattleClear3800(BattleContext *battle, uint16_t site) {
    return BattleCall(battle, site, 0x81c30eu, 2u);
}

#endif
