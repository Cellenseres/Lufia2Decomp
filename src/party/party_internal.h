#ifndef LUFIA2_PARTY_INTERNAL_H
#define LUFIA2_PARTY_INTERNAL_H

#include "lufia2/execution.h"

/* $82:C38B: $09C4 = record of capsule and form. */
void Lufia2CapsuleRecordPointer(const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

/* $82:C4B3: skill id of slot A (learned or not). */
void Lufia2CapsuleSkill(const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* $82:F6A4 / $82:F6D4: clear bonus words of the block at [$2A]. */
void Lufia2BonusClear(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t first, unsigned words);

/* $83:C652 carry = a member of $0A7B has byte A in the 36-byte list at
   record + $96; DB kept, M1X0. */
Lufia2ExecutionResult Lufia2PartyListHasEntry(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu);

#endif
