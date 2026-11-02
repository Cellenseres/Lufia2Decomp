#ifndef LUFIA2_ACTOR_RECORD_OFFSETS_INTERNAL_H
#define LUFIA2_ACTOR_RECORD_OFFSETS_INTERNAL_H

#include "core/cpu_internal.h"
#include "system/wram.h"

#if defined(_MSC_VER)
static __forceinline void ActorPrepareRecordOffsets(
#else
static inline void ActorPrepareRecordOffsets(
#endif
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Write8(memory, DirectAddress(cpu, DP_ACTOR_SLOT + 1u), 0u);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, DP_SLOT_WORD_OFFSET), A8(cpu));
    Write8(memory, DirectAddress(cpu, DP_SLOT_WORD_OFFSET + 1u), 0u);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    Write8(memory, DirectAddress(cpu, DP_SLOT_RECORD_OFFSET), A8(cpu));
    Write8(memory, DirectAddress(cpu, DP_SLOT_RECORD_OFFSET + 1u), 0u);
    Write8(memory, DirectAddress(cpu, DP_SLOT_RECORD_OFFSET + 2u), 0u);
}

#endif
