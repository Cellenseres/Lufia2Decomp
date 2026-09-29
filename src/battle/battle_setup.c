/* Battle setup. */

#include "battle/battle_lifecycle_detail.h"

Lufia2ExecutionResult Lufia2BattleSetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    if (!BattlePrepareOpening(&battle))
        return BattleChildUnwound(&battle);

    BattleCopyPartyFormation(&battle);

    if (!BattleInitializeRecords(&battle))
        return BattleChildUnwound(&battle);

    BattleApplyScenario(&battle);

    if (!BattleFinalizeSetup(&battle))
        return BattleChildUnwound(&battle);

    return ExecutionReturned(0x8181e5u);
}
