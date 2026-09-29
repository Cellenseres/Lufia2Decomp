#ifndef LUFIA2_BATTLE_BATTLE_INTERNAL_H
#define LUFIA2_BATTLE_BATTLE_INTERNAL_H

#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    BATTLE_WRAM_SCRIPT_CONTEXT = 0x0a13u,
    BATTLE_WRAM_BACKGROUND_ID = 0x11e1u,
    BATTLE_WRAM_CONTROL_FLAGS = 0x11e7u,
    BATTLE_WRAM_STARTUP_COUNTER = 0x1264u,
    BATTLE_WRAM_FRAME_STATE = 0x129au,
    BATTLE_WRAM_SAVED_ENTRY_STACK = 0x1395u,
    BATTLE_WRAM_SAVED_LOOP_STACK = 0x1397u,
};

enum {
    BATTLE_CONTROL_MODE_1 = 0x01u,
    BATTLE_CONTROL_MODE_2 = 0x02u,
    BATTLE_CONTROL_MODE_6 = 0x40u,
    BATTLE_CONTROL_FINISHED = 0x80u,
    BATTLE_BACKGROUND_BLANK = 0x18u,
};

typedef struct Lufia2BattleChildCalls {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    uint8_t return_bank;
    uint32_t unwind_site;
} Lufia2BattleChildCalls;

static inline uint8_t Lufia2BattleCallChild(
    Lufia2BattleChildCalls *calls,
    uint16_t site,
    uint32_t target,
    uint8_t frame_size) {
    const uint32_t full_site = ((uint32_t)calls->return_bank << 16) | site;

    if (frame_size == 2u)
        SimulateJsrFrame(calls->memory, calls->cpu, (uint16_t)(site + 2u));
    else
        SimulateJslFrame(
            calls->memory, calls->cpu, calls->return_bank, (uint16_t)(site + 3u));

    if (calls->child(
            calls->context, calls->cpu, target, full_site, frame_size))
        return 1;

    calls->unwind_site = full_site;
    return 0;
}

static inline Lufia2ExecutionResult Lufia2BattleChildUnwound(
    const Lufia2BattleChildCalls *calls) {
    Lufia2ExecutionResult result = ExecutionReturned(calls->unwind_site);

    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static inline void Lufia2BattleLoadControlFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, BATTLE_WRAM_CONTROL_FLAGS));
}

static inline uint8_t Lufia2BattleControlFlag(
    Lufia2CpuState *cpu, uint8_t mask) {
    OpBitValue(cpu, mask);
    return !cpu->zero;
}

#endif
