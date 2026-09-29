/* Battle entry. */

#include "battle/battle_lifecycle_detail.h"

Lufia2ExecutionResult Lufia2BattleEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    BattleBeginSession(&battle);

    if (!BattleRunSetup(&battle))
        return BattleChildUnwound(&battle);
    if (!BattleRunMainLoop(&battle))
        return BattleChildUnwound(&battle);

    BattleRestoreSessionStack(&battle);

    if (!BattleRunPostLoopSteps(&battle))
        return BattleChildUnwound(&battle);
    if (!BattleRunExit(&battle))
        return BattleChildUnwound(&battle);

    BattleEndSession(&battle);
    return ExecutionReturned(0x81886eu);
}
