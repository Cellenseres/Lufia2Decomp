#ifndef LUFIA2_BATTLE_ACTION_RECORDS_INTERNAL_H
#define LUFIA2_BATTLE_ACTION_RECORDS_INTERNAL_H

#include "lufia2/execution.h"

Lufia2ExecutionResult Lufia2BattleReadIpRecordPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#endif
