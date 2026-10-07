#ifndef LUFIA2_SPRITE_ALLOCATION_INTERNAL_H
#define LUFIA2_SPRITE_ALLOCATION_INTERNAL_H

#include "core/cpu_internal.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

static inline void SpriteAllocateSlotsBody(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_A), A8(cpu));
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadX8(cpu, 0x00u);
    LoadY8(cpu, 0x00u);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), 0x00u);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), 0x00u);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0xe100u, cpu->x);
        if (cpu->zero) {

            LoadY8(cpu, (uint8_t)(cpu->y + 1u));
            Compare8(cpu, (uint8_t)cpu->y,
                     Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
            if (cpu->zero)
                break;
            LoadX8(cpu, (uint8_t)(cpu->x + 1u));
            continue;
        }
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_C)));
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_A)));
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_C), A8(cpu));
        Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
        TransferAToX(cpu);
        Compare8(cpu, A8(cpu), 0x80u);
        if (cpu->zero) {
            PullDataBank(memory, cpu);
            cpu->carry = 1;
            return;
        }
    }
    LoadX8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_ACTOR_SLOT)));
    Or8(cpu, 0x80u);
    do {
        StoreAAbsolute8(memory, cpu, 0xe100u, cpu->x);
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        DecrementDirect8(memory, cpu, DP_SCRATCH_A);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
    AslA8(cpu);
    Write8(memory, DirectAddress(cpu, DP_SCRATCH_B), A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
    And8(cpu, 0xf0u);
    TrbDirect8(memory, cpu, DP_SCRATCH_B);
    AslA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    {
        const uint8_t carry = cpu->carry;

        cpu->carry = 0;
        LoadA8(cpu, (uint8_t)((A8(cpu) << 1) | carry));
    }
    ExchangeAccumulatorBytes(cpu);
    Adc8(cpu, Read8(memory, DirectAddress(cpu, DP_SCRATCH_B)));
    cpu->carry = 0;
}

#endif
