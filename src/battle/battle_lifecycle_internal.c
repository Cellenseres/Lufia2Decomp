#include "battle/battle_lifecycle_internal.h"
#include "core/snes_registers.h"
#include "system/scene_nmi_internal.h"

enum {
    BATTLE_WORK_CLEAR_START = 0x11d8u,
    BATTLE_WORK_CLEAR_END = 0x1c0cu,
};

static void ClearBattleWorkArea(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpStz(memory, cpu, OpDp(cpu, 0x40u));
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_WORK_CLEAR_START));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, BATTLE_WORK_CLEAR_START);
    OpLdy(cpu, BATTLE_WORK_CLEAR_START + 1u);
    LoadA16(cpu, BATTLE_WORK_CLEAR_END - BATTLE_WORK_CLEAR_START - 1u);
    OpMoveNext(memory, cpu, 0x00u, 0x00u);
    OpSepWidths(cpu, 0x20u);
}

void BattleBeginSession(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);

    ClearBattleWorkArea(battle);
    OpSetDataBank(memory, cpu, 0x97u);
    OpStz(memory, cpu, OpAbs(cpu, 0x11a5u));

    OpLdx(cpu, cpu->stack);
    OpWriteX(memory, cpu, OpAbs(cpu, BATTLE_SAVED_ENTRY_STACK), cpu->x);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SCRIPT_CONTEXT));
}

void BattleRestoreSessionStack(BattleContext *battle) {
    Lufia2CpuState *cpu = battle->cpu;

    OpLdx(cpu, OpReadX(battle->memory, cpu, OpAbs(cpu, BATTLE_SAVED_ENTRY_STACK)));
    cpu->stack = cpu->x;
}

bool BattleRunPostLoopSteps(BattleContext *battle) {
    if (!BattleCall(battle, 0x8859u, 0x85edbbu, 3u))
        return false;

    return BattleCall(battle, 0x885du, 0x85eea1u, 3u);
}

void BattleEndSession(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SCRIPT_CONTEXT));
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
}

static bool UseRegularBattleFade(BattleContext *battle) {
    Lufia2CpuState *cpu = battle->cpu;

    OpLda(battle->memory, cpu, WRAM_FIELD_BATTLE_RESULT);
    OpCmpValue(cpu, 1u);
    if (!cpu->zero)
        return true;

    OpLda(battle->memory, cpu, WRAM_FIELD_BATTLE_SOURCE);
    if (!cpu->negative)
        return false;

    OpLda(battle->memory, cpu, 0x7ff8a4u);
    OpCmpValue(cpu, 11u);
    if (cpu->zero)
        return true;

    OpCmpValue(cpu, 37u);
    return cpu->zero;
}

bool BattlePrepareExit(BattleContext *battle) {
    if (!BattleCall(battle, 0x876bu, 0x85ee3eu, 3u))
        return false;

    return BattleCall(battle, 0x876fu, 0x85eddbu, 3u);
}

bool BattleFadeOut(BattleContext *battle) {
    if (UseRegularBattleFade(battle))
        return BattleCall(battle, 0x8793u, 0x81c321u, 2u);

    return BattleCall(battle, 0x878du, 0x85eaefu, 3u);
}

bool BattleTearDownDisplay(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleClearWindowTilemapForExit(battle))
        return false;
    if (!BattleClearTilemap3800ForExit(battle))
        return false;

    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0x879eu, 0x859bc3u, 3u))
        return false;
    OpSepWidths(cpu, 0x20u);

    if (!BattleCall(battle, 0x87a4u, 0x85ec81u, 3u))
        return false;

    Lufia2DisableSceneNmi(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    LoadA8(cpu, BRIGHTNESS_FORCED_BLANK);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));

    return BattleCall(battle, 0x87b2u, 0x85ec81u, 3u);
}
