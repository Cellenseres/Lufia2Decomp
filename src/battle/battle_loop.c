/* Battle loop. */

#include "battle/battle_lifecycle_detail.h"

Lufia2ExecutionResult Lufia2BattleMainLoop(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    BattleSaveLoopStack(&battle);

    if (!BattleBeginMainLoop(&battle))
        return BattleChildUnwound(&battle);

    for (;;) {
        const BattleFrameResult frame = BattleRunFrame(&battle);

        if (frame == BATTLE_FRAME_UNWOUND)
            return BattleChildUnwound(&battle);
        if (frame == BATTLE_FRAME_FINISHED)
            break;
    }

    BattleClearTilemap(&battle);

    if (!BattleFinishMainLoop(&battle))
        return BattleChildUnwound(&battle);

    return ExecutionReturned(0x818909u);
}
