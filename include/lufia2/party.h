#ifndef LUFIA2_PARTY_H
#define LUFIA2_PARTY_H

/* Party members: experience and stats. */

#include "lufia2/execution.h"

#ifdef __cplusplus
extern "C" {
#endif

/* $81:F9E9 experience for level $09FE of member $09FA; M1X0. */
Lufia2ExecutionResult Lufia2PartyExperienceForLevel(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F4D5 derived stats of block X; any width. */
Lufia2ExecutionResult Lufia2PartyDerivedStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C261 capsule monster stats when $0A7F is 7; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleLoadStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C515 forms of present capsules; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleSetForms(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C2FD all capsules to level 1; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleReset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C352 present flags = A, capsule 0; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleSetAll(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:CD83 capsule level up; carry clear if gained; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleLevelUp(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:CE23 capsule experience range of the level; M1X0. */
Lufia2ExecutionResult Lufia2CapsuleExperienceRange(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F87F stats of member $09FA at level $09FE; DB = $97, M1X0. */
Lufia2ExecutionResult Lufia2PartyBaseStats(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:ED8E member block $7E:Y from its stored form at X; X16. */
Lufia2ExecutionResult Lufia2PartyUnpackMember(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:EE94 member block without equipment; X16. */
Lufia2ExecutionResult Lufia2PartyUnpackMemberBare(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F5ED $11 = $25 in each active party record; M1X0. */
Lufia2ExecutionResult Lufia2PartyRestore11(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F60B $13 = $27 in each active party record; M1X0. */
Lufia2ExecutionResult Lufia2PartyRestore13(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F7BD X = word X of $85:9EBA; P kept. */
Lufia2ExecutionResult Lufia2BattleTable9EBA(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F78D $0A80 = records of members $0A7B; M1X0. */
Lufia2ExecutionResult Lufia2PartyPointers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F789 far call of $81:F78D; M1X0. */
Lufia2ExecutionResult Lufia2PartyPointersFar(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F4ED member $C1 stats plus equipment; M1X0. */
Lufia2ExecutionResult Lufia2PartyStatTotals(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F4E9 far call of $81:F4ED; M1X0. */
Lufia2ExecutionResult Lufia2PartyStatTotalsFar(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F979: level-up of member $09FA; M1X0. */
Lufia2ExecutionResult Lufia2PartyLevelUpCheck(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:FC0B new record at $7E:[$B2] for character $09F2; M1X0. */
Lufia2ExecutionResult Lufia2PartyNewRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
