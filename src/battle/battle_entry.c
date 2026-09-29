/* Battle entry. */

#include "battle/battle_internal.h"

enum {
    BATTLE_WORK_CLEAR_START = 0x11d8u,
    BATTLE_WORK_CLEAR_END = 0x1c0cu,
};

static void ClearBattleWorkArea(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, 0x40u));
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_WORK_CLEAR_START));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, BATTLE_WORK_CLEAR_START);
    OpLdy(cpu, BATTLE_WORK_CLEAR_START + 1u);
    LoadA16(cpu, BATTLE_WORK_CLEAR_END - BATTLE_WORK_CLEAR_START - 1u);
    OpMoveNext(memory, cpu, 0x00u, 0x00u);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2BattleEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    BattleContext battle = {memory, cpu, child, child_context, 0x81u, 0u};

    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);

    ClearBattleWorkArea(memory, cpu);
    OpSetDataBank(memory, cpu, 0x97u);
    OpStz(memory, cpu, OpAbs(cpu, 0x11a5u));

    OpLdx(cpu, cpu->stack);
    OpWriteX(memory, cpu, OpAbs(cpu, BATTLE_SAVED_ENTRY_STACK), cpu->x);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SCRIPT_CONTEXT));

    if (!BattleRunSetup(&battle))
        return BattleChildUnwound(&battle);
    if (!BattleRunMainLoop(&battle))
        return BattleChildUnwound(&battle);

    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, BATTLE_SAVED_ENTRY_STACK)));
    cpu->stack = cpu->x;

    if (!BattleCall(&battle, 0x8859u, 0x85edbbu, 3u))
        return BattleChildUnwound(&battle);
    if (!BattleCall(&battle, 0x885du, 0x85eea1u, 3u))
        return BattleChildUnwound(&battle);
    if (!BattleRunExit(&battle))
        return BattleChildUnwound(&battle);

    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SCRIPT_CONTEXT));
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81886eu);
}
