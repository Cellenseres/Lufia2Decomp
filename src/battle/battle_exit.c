/* Battle exit. */

#include "battle/battle_lifecycle_detail.h"

Lufia2ExecutionResult Lufia2BattleExit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    if (!BattlePrepareExit(&battle))
        return BattleChildUnwound(&battle);
    if (!BattleFadeOut(&battle))
        return BattleChildUnwound(&battle);
    if (!BattleTearDownDisplay(&battle))
        return BattleChildUnwound(&battle);

    return ExecutionReturned(0x8187b6u);
}
