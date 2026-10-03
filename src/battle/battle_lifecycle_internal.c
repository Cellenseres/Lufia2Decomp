#include "battle/battle_lifecycle_internal.h"
#include "core/snes_registers.h"
#include "system/scene_nmi_internal.h"

enum {
    BATTLE_WORK_CLEAR_START = 0x11d8u,
    BATTLE_WORK_CLEAR_END = 0x1c0cu,
};

/* Clears DP $40 and zero-fills the battle work area from $11D8 up to $1C0B. */
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

/* Saves the data bank and flags, keeps X (the caller's stack pointer) in
 * $1395 and marks the battle script context active ($FF). */
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

/* Sets the stack pointer back to the one saved by BattleBeginSession. */
void BattleRestoreSessionStack(BattleContext *battle) {
    Lufia2CpuState *cpu = battle->cpu;

    OpLdx(cpu, OpReadX(battle->memory, cpu, OpAbs(cpu, BATTLE_SAVED_ENTRY_STACK)));
    cpu->stack = cpu->x;
}

/* Runs $85:EDBB and then $85:EEA1 after the battle loop. */
bool BattleRunPostLoopSteps(BattleContext *battle) {
    if (!BattleCall(battle, 0x8859u, 0x85edbbu, 3u))
        return false;

    return BattleCall(battle, 0x885du, 0x85eea1u, 3u);
}

/* Clears the script context and restores Y, X, A, the flags and the data bank
 * saved by BattleBeginSession. */
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

/* True for the normal fade: any result but 1, or a result of 1 from a source
 * with bit 7 set while $7F:F8A4 is 11 or 37. */
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

/* Runs $85:EE3E and then $85:EDDB before leaving the battle. */
bool BattlePrepareExit(BattleContext *battle) {
    if (!BattleCall(battle, 0x876bu, 0x85ee3eu, 3u))
        return false;

    return BattleCall(battle, 0x876fu, 0x85eddbu, 3u);
}

/* Fades out with $81:C321 for the normal fade, otherwise with $85:EAEF. */
bool BattleFadeOut(BattleContext *battle) {
    if (UseRegularBattleFade(battle))
        return BattleCall(battle, 0x8793u, 0x81c321u, 2u);

    return BattleCall(battle, 0x878du, 0x85eaefu, 3u);
}

/* Clears the window and $3800 tilemaps, runs $85:9BC3, waits a frame, turns
 * the scene NMI and HDMA off and forces blank, then waits another frame. */
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

    if (!BattleCall(battle, 0x87a4u, BATTLE_ROUTINE_FRAME_INPUT, 3u))
        return false;

    Lufia2DisableSceneNmi(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, SNES_HDMAEN));
    LoadA8(cpu, BRIGHTNESS_FORCED_BLANK);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));

    return BattleCall(battle, 0x87b2u, BATTLE_ROUTINE_FRAME_INPUT, 3u);
}
