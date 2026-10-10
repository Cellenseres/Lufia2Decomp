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

/* $81:F481 copied member totals; native PB81/DP0/S1F00..1FFC, any widths.
 * Restores A/X/Y/DB/P from their stack slots; PLB supplies N/Z. JSL/RTL.
 * Shared stat checkpoint $81:F576 remains available before copy-back. */
Lufia2ExecutionResult Lufia2PartyStatTotalsOfCopy(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F4E9 far call of $81:F4ED; M1X0. */
Lufia2ExecutionResult Lufia2PartyStatTotalsFar(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:F7CA one level gain; A.low member 0-6, M1X0/native binary/DP0.
 * DB maps low WRAM; carry set when gained. Child frames remain explicit. */
Lufia2ExecutionResult Lufia2PartyApplyLevel(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $81:F7ED gains for record X, member $09FA=0-6, record level 1-99.
 * Same widths/DP/DB as F7CA; DB/X restored.
 * Inconsistent growth retains the original BRK boundary at $81:F87E. */
Lufia2ExecutionResult Lufia2PartyStatGrowth(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

/* $81:F979: level-up of member $09FA; M1X0. */
Lufia2ExecutionResult Lufia2PartyLevelUpCheck(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $81:FC0B new record at $7E:[$B2] for character $09F2; M1X0. */
Lufia2ExecutionResult Lufia2PartyNewRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleClearFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleResolveRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleFormIndex(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleSavedOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleLoadSavedStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleBuildLevelExperience(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleAdvanceExperienceStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleRebuildStatBlock(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PartyClearSecondaryModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2PartyClearPrimaryModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleBuildStatValues(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleAccumulateStatGrowth(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleGetFlagMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleGetFormAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleGetItemAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleChooseMenuItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleGetStatusAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleCheckMenuForm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleEnsureMenuItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleUpdateItemCursor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleRefreshMenuItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PartySelectEquipmentSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2PartyLoadEquipmentItem(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PartyAccumulateEquipmentModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2PartyRebuildEquipmentModifiers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PartyRebuildAllEquipmentStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PartySelectActiveEquipmentMember(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2PartyRefreshActiveEquipmentStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleStoreSavedStats(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleSelectAbilityFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleReadAbilityByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleBuildAbilityMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

Lufia2ExecutionResult Lufia2CapsuleUnlockAbility(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PartyLearnSelectedSpell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2CapsuleOpenSelection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context);

Lufia2ExecutionResult Lufia2PartyListHasEntry(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

#ifdef __cplusplus
}
#endif

#endif
