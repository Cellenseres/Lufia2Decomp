/* Battle display setup. */

#include "battle/battle_lifecycle_detail.h"

Lufia2ExecutionResult Lufia2BattleDisplaySetup(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    BattleResetDisplayWork(&battle);

    if (!BattleClearDisplayBuffers(&battle))
        return BattleChildUnwound(&battle);

    BattleBuildPartyDisplayState(&battle);
    BattleInitializeDisplayRecords(&battle);

    if (!BattlePrepareDisplayRecords(&battle))
        return BattleChildUnwound(&battle);

    BattleApplyPpuTable(&battle);

    if (!BattleLoadBaseGraphics(&battle))
        return BattleChildUnwound(&battle);

    BattleUploadBaseTiles(&battle);

    if (!BattleLoadPresentationAssets(&battle))
        return BattleChildUnwound(&battle);

    BattlePrepareSpriteState(&battle);

    if (!BattleFinishDisplay(&battle))
        return BattleChildUnwound(&battle);

    return ExecutionReturned(0x81876au);
}
