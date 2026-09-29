/* Battle loop. */

#include "battle/battle_internal.h"
#include "core/snes_registers.h"

typedef enum BattleFrameResult {
    BATTLE_FRAME_UNWOUND = 0,
    BATTLE_FRAME_FINISHED,
    BATTLE_FRAME_CONTINUE,
} BattleFrameResult;

static BattleFrameResult RunBattleFrame(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleRunFrameUpkeep(battle))
        return BATTLE_FRAME_UNWOUND;

    if (BattleControlHas(battle, BATTLE_CONTROL_FINISHED)) {
        PushAccumulator8(memory, cpu);
        LoadA8(cpu, BATTLE_CONTROL_MODE_1 | BATTLE_CONTROL_MODE_2);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CONTROL_FLAGS), 0u);
        LoadA8(cpu, Pull8(memory, cpu));
    }

    if (BattleControlHas(battle, BATTLE_CONTROL_MODE_1)) {
        if (!BattleCall(battle, 0x8894u, 0x859236u, 3u))
            return BATTLE_FRAME_UNWOUND;
        if (!cpu->carry) {
            if (!BattleCall(battle, 0x889au, 0x8596a2u, 3u))
                return BATTLE_FRAME_UNWOUND;
            if (!BattleCall(battle, 0x889eu, 0x81c739u, 2u))
                return BATTLE_FRAME_UNWOUND;
            if (!BattleCall(battle, 0x88a1u, 0x8596b0u, 3u))
                return BATTLE_FRAME_UNWOUND;
        }
        if (!BattleCall(battle, 0x88a5u, 0x859275u, 3u))
            return BATTLE_FRAME_UNWOUND;
        if (!BattleCall(battle, 0x88a9u, 0x81c294u, 2u))
            return BATTLE_FRAME_UNWOUND;
    }

    if (BattleControlHas(battle, BATTLE_CONTROL_MODE_2) &&
        !BattleCall(battle, 0x88b3u, 0x81c254u, 2u))
        return BATTLE_FRAME_UNWOUND;

    OpStz(memory, cpu, OpAbs(cpu, BATTLE_FRAME_STATE));
    if (!BattleCall(battle, 0x88b9u, 0x85ab78u, 3u))
        return BATTLE_FRAME_UNWOUND;
    if (!BattleCall(battle, 0x88bdu, 0x8589e5u, 3u))
        return BATTLE_FRAME_UNWOUND;
    if (!BattleCall(battle, 0x88c1u, 0x81890au, 2u))
        return BATTLE_FRAME_UNWOUND;

    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_FRAME_STATE));

    if (BattleControlHas(battle, BATTLE_CONTROL_FINISHED))
        return BATTLE_FRAME_FINISHED;

    if (!BattleCall(battle, 0x88d0u, 0x81c240u, 2u))
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
    BattleContext battle = {memory, cpu, child, child_context, 0x81u, 0u};

    OpLdx(cpu, cpu->stack);
    OpWriteX(memory, cpu, OpAbs(cpu, BATTLE_SAVED_LOOP_STACK), cpu->x);

    if (!BattleCall(&battle, 0x8873u, 0x8593b7u, 3u))
        return BattleChildUnwound(&battle);

    for (;;) {
        const BattleFrameResult frame = RunBattleFrame(&battle);

        if (frame == BATTLE_FRAME_UNWOUND)
            return BattleChildUnwound(&battle);
        if (frame == BATTLE_FRAME_FINISHED)
            break;
    }

    ClearBattleTilemapPort(memory, cpu);

    OpLda(memory, cpu, 0x7ff8a2u);
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        if (!BattleCall(&battle, 0x88fau, 0x85e7bcu, 3u))
            return BattleChildUnwound(&battle);
        LoadA8(cpu, 29u);
        if (!BattleCall(&battle, 0x8900u, 0x8093feu, 3u))
            return BattleChildUnwound(&battle);
        if (!BattleCall(&battle, 0x8904u, 0x81d9e1u, 2u))
            return BattleChildUnwound(&battle);
    }

    return ExecutionReturned(0x818909u);
}
