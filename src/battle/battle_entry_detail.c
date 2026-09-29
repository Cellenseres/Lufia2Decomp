#include "battle/battle_lifecycle_detail.h"

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
