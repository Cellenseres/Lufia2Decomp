#ifndef LUFIA2_BATTLE_STATUS_TARGET_INTERNAL_H
#define LUFIA2_BATTLE_STATUS_TARGET_INTERNAL_H
#include "lufia2/decomp.h"
typedef enum BattleStatusTargetState {
    BATTLE_STATUS_TARGET_STOPPED,
    BATTLE_STATUS_TARGET_SKIPPED,
    BATTLE_STATUS_TARGET_READY
} BattleStatusTargetState;
BattleStatusTargetState BattlePrepareStatusTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint8_t recovery,
    Lufia2ExecutionResult *result);
#endif
