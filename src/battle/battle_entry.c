/* Battle entry. */

#include "battle/battle_lifecycle_detail.h"

Lufia2ExecutionResult Lufia2BattleEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    BattleContext battle = {memory, cpu, child, child_context, 0x81u, 0u};

    BattleBeginSession(&battle);

    if (!BattleRunSetup(&battle))
        return BattleChildUnwound(&battle);
    if (!BattleRunMainLoop(&battle))
        return BattleChildUnwound(&battle);

    BattleRestoreSessionStack(&battle);

    if (!BattleCall(&battle, 0x8859u, 0x85edbbu, 3u))
        return BattleChildUnwound(&battle);
    if (!BattleCall(&battle, 0x885du, 0x85eea1u, 3u))
        return BattleChildUnwound(&battle);
    if (!BattleRunExit(&battle))
        return BattleChildUnwound(&battle);

    BattleEndSession(&battle);
    return ExecutionReturned(0x81886eu);
}
