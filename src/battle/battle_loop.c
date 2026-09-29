/* Battle loop. */

#include "battle/battle_internal.h"
#include "core/snes_registers.h"

typedef enum BattleFrameResult {
    BATTLE_FRAME_UNWOUND = 0,
    BATTLE_FRAME_FINISHED,
    BATTLE_FRAME_CONTINUE,
} BattleFrameResult;

static BattleFrameResult RunBattleFrame(Lufia2BattleChildCalls *calls) {
    const Lufia2Memory *memory = calls->memory;
    Lufia2CpuState *cpu = calls->cpu;

    if (!Lufia2BattleCallChild(calls, 0x8877u, 0x85ecf0u, 3u))
        return BATTLE_FRAME_UNWOUND;

    Lufia2BattleLoadControlFlags(memory, cpu);
    if (Lufia2BattleControlFlag(cpu, BATTLE_CONTROL_FINISHED)) {
        PushAccumulator8(memory, cpu);
        LoadA8(cpu, BATTLE_CONTROL_MODE_1 | BATTLE_CONTROL_MODE_2);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CONTROL_FLAGS), 0u);
        LoadA8(cpu, Pull8(memory, cpu));
    }

    Lufia2BattleLoadControlFlags(memory, cpu);
    if (Lufia2BattleControlFlag(cpu, BATTLE_CONTROL_MODE_1)) {
        if (!Lufia2BattleCallChild(calls, 0x8894u, 0x859236u, 3u))
            return BATTLE_FRAME_UNWOUND;
        if (!cpu->carry) {
            if (!Lufia2BattleCallChild(calls, 0x889au, 0x8596a2u, 3u))
                return BATTLE_FRAME_UNWOUND;
            if (!Lufia2BattleCallChild(calls, 0x889eu, 0x81c739u, 2u))
                return BATTLE_FRAME_UNWOUND;
            if (!Lufia2BattleCallChild(calls, 0x88a1u, 0x8596b0u, 3u))
                return BATTLE_FRAME_UNWOUND;
        }
        if (!Lufia2BattleCallChild(calls, 0x88a5u, 0x859275u, 3u))
            return BATTLE_FRAME_UNWOUND;
        if (!Lufia2BattleCallChild(calls, 0x88a9u, 0x81c294u, 2u))
            return BATTLE_FRAME_UNWOUND;
    }

    Lufia2BattleLoadControlFlags(memory, cpu);
    if (Lufia2BattleControlFlag(cpu, BATTLE_CONTROL_MODE_2) &&
        !Lufia2BattleCallChild(calls, 0x88b3u, 0x81c254u, 2u))
        return BATTLE_FRAME_UNWOUND;

    OpStz(memory, cpu, OpAbs(cpu, BATTLE_FRAME_STATE));
    if (!Lufia2BattleCallChild(calls, 0x88b9u, 0x85ab78u, 3u))
        return BATTLE_FRAME_UNWOUND;
    if (!Lufia2BattleCallChild(calls, 0x88bdu, 0x8589e5u, 3u))
        return BATTLE_FRAME_UNWOUND;
    if (!Lufia2BattleCallChild(calls, 0x88c1u, 0x81890au, 2u))
        return BATTLE_FRAME_UNWOUND;

    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_FRAME_STATE));

    Lufia2BattleLoadControlFlags(memory, cpu);
    if (Lufia2BattleControlFlag(cpu, BATTLE_CONTROL_FINISHED))
        return BATTLE_FRAME_FINISHED;

    if (!Lufia2BattleCallChild(calls, 0x88d0u, 0x81c240u, 2u))
        return BATTLE_FRAME_UNWOUND;

    return BATTLE_FRAME_CONTINUE;
}

static void ClearBattleTilemapPort(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x2800u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));

    OpLdx(cpu, 0x0280u);
    LoadA8(cpu, 0x21u);
    do {
        OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpDex(cpu);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2BattleMainLoop(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu,
                                           Lufia2PushedChildCall child,
                                           void *child_context) {
    Lufia2BattleChildCalls calls = {memory, cpu, child, child_context, 0x81u, 0u};

    OpLdx(cpu, cpu->stack);
    OpWriteX(memory, cpu, OpAbs(cpu, BATTLE_SAVED_LOOP_STACK), cpu->x);

    if (!Lufia2BattleCallChild(&calls, 0x8873u, 0x8593b7u, 3u))
        return Lufia2BattleChildUnwound(&calls);

    for (;;) {
        const BattleFrameResult frame = RunBattleFrame(&calls);

        if (frame == BATTLE_FRAME_UNWOUND)
            return Lufia2BattleChildUnwound(&calls);
        if (frame == BATTLE_FRAME_FINISHED)
            break;
    }

    ClearBattleTilemapPort(memory, cpu);

    OpLda(memory, cpu, 0x7ff8a2u);
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        if (!Lufia2BattleCallChild(&calls, 0x88fau, 0x85e7bcu, 3u))
            return Lufia2BattleChildUnwound(&calls);
        LoadA8(cpu, 29u);
        if (!Lufia2BattleCallChild(&calls, 0x8900u, 0x8093feu, 3u))
            return Lufia2BattleChildUnwound(&calls);
        if (!Lufia2BattleCallChild(&calls, 0x8904u, 0x81d9e1u, 2u))
            return Lufia2BattleChildUnwound(&calls);
    }

    return ExecutionReturned(0x818909u);
}
