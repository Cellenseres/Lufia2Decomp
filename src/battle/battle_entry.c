/* Battle entry and return wrapper at $81:8821. */

#include "battle/battle_internal.h"

enum {
    BATTLE_WORK_CLEAR_START = 0x11d8u,
    BATTLE_WORK_CLEAR_END = 0x1c0cu,
};

static void ClearBattleWorkArea(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
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
    Lufia2BattleChildCalls calls = {
        memory, cpu, child, child_context, 0x81u, 0u};

    PushDataBank(memory, cpu);                              /* $81:8821 */
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);

    ClearBattleWorkArea(memory, cpu);
    OpSetDataBank(memory, cpu, 0x97u);
    OpStz(memory, cpu, OpAbs(cpu, 0x11a5u));

    OpLdx(cpu, cpu->stack);                                 /* TSX */
    OpWriteX(
        memory, cpu, OpAbs(cpu, BATTLE_WRAM_SAVED_ENTRY_STACK), cpu->x);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_WRAM_SCRIPT_CONTEXT));

    if (!Lufia2BattleCallChild(&calls, 0x884fu, 0x818000u, 2u))
        return Lufia2BattleChildUnwound(&calls);
    if (!Lufia2BattleCallChild(&calls, 0x8852u, 0x81886fu, 2u))
        return Lufia2BattleChildUnwound(&calls);

    OpLdx(
        cpu,
        OpReadX(
            memory, cpu, OpAbs(cpu, BATTLE_WRAM_SAVED_ENTRY_STACK)));
    cpu->stack = cpu->x;                                    /* TXS */

    if (!Lufia2BattleCallChild(&calls, 0x8859u, 0x85edbbu, 3u))
        return Lufia2BattleChildUnwound(&calls);
    if (!Lufia2BattleCallChild(&calls, 0x885du, 0x85eea1u, 3u))
        return Lufia2BattleChildUnwound(&calls);
    if (!Lufia2BattleCallChild(&calls, 0x8861u, 0x81876bu, 2u))
        return Lufia2BattleChildUnwound(&calls);

    OpStz(memory, cpu, OpAbs(cpu, BATTLE_WRAM_SCRIPT_CONTEXT));
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81886eu);
}
