/* Battle entry. */

#include "battle/battle_lifecycle_internal.h"

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

    return BattleFinishSession(&battle);
}
