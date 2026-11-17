#include "battle/battle_lifecycle_internal.h"
#include "core/snes_registers.h"
#include "lufia2/battle_tilemap_steps.h"

static void ClearBattleTilemapCell(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
    OpDex(cpu);
}

void BattleSaveLoopStack(BattleContext *battle) {
    Lufia2CpuState *cpu = battle->cpu;

    OpLdx(cpu, cpu->stack);
    OpWriteX(battle->memory, cpu, OpAbs(cpu, BATTLE_SAVED_LOOP_STACK), cpu->x);
}

bool BattleBeginMainLoop(BattleContext *battle) {
    return BattleCall(battle, 0x8873u, 0x8593b7u, 3u);
}

BattleFrameResult BattleRunFrame(BattleContext *battle) {
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
    if (!BattleCall(battle, 0x88bdu, BATTLE_ROUTINE_CLEAR_SPRITE_OFFSETS, 3u))
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

void BattleClearTilemap(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLdx(cpu, 0x2800u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));

    OpLdx(cpu, 0x0280u);
    LoadA8(cpu, 0x21u);
    do {
        ClearBattleTilemapCell(memory, cpu);
    } while (!cpu->zero);
}

bool BattleFinishMainLoop(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpLda(memory, cpu, WRAM_FIELD_BATTLE_RESULT);
    OpCmpValue(cpu, 0u);
    if (!cpu->zero)
        return true;

    if (!BattleCall(battle, 0x88fau, 0x85e7bcu, 3u))
        return false;

    LoadA8(cpu, 29u);
    if (!BattleCall(battle, 0x8900u, 0x8093feu, 3u))
        return false;

    return BattleCall(battle, 0x8904u, 0x81d9e1u, 2u);
}

Lufia2ExecutionResult Lufia2BattleClearTilemapCells(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, unsigned max_cells) {
    if (cpu->program_bank != 0x81u || cpu->accumulator_is_8_bit != 1u ||
        cpu->index_is_8_bit || !cpu->x || cpu->x > 0x0280u ||
        !max_cells || max_cells > LUFIA2_BATTLE_TILEMAP_SPAN_MAX_CELLS)
        return ExecutionHandoff(cpu, 0x8188e9u);
    unsigned cells = 0u;
    do {
        ClearBattleTilemapCell(memory, cpu);
        ++cells;
    } while (!cpu->zero && cells < max_cells);
    return ExecutionHandoff(cpu, cpu->zero ? 0x8188f2u : 0x8188e9u);
}
