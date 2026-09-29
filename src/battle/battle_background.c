/* Battle background setup. */

#include "battle/battle_lifecycle_detail.h"

Lufia2ExecutionResult Lufia2BattleBackgroundPrepare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    BattleContext battle = {memory, cpu, child, child_context, 0x81u, 0u};
    uint8_t background_id = 0;

    if (BattleBackgroundIsBlank(&battle, &background_id)) {
        BattleClearBackground(&battle);
    } else if (!BattleLoadBackground(&battle, background_id)) {
        return BattleChildUnwound(&battle);
    }

    if (!BattleFinishBackground(&battle))
        return BattleChildUnwound(&battle);

    return ExecutionReturned(0x81bacau);
}
