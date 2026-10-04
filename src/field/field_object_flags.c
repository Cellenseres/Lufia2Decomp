/* Flag bytes of the field object slots. */

#include "core/cpu_internal.h"
#include "core/wram_view.h"
#include "lufia2/field.h"

enum {
    FLAGS = 0x0622u,
    FIRST_SLOT = 5u,
    END_SLOT = 0x28u,
    FLAG_BIT = 1u
};

/* $80:C195: mark slots 5 through 39, preserving their other flag bits. */
Lufia2ExecutionResult Lufia2FieldMarkObjectSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t slot;
    uint8_t flags = 0;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x80c195u);
    for (slot = FIRST_SLOT; slot < END_SLOT; ++slot) {
        flags = (uint8_t)(WramReadAt(wram, FLAGS, slot) | FLAG_BIT);
        WramWriteAt(wram, FLAGS, slot, flags);
    }
    cpu->accumulator = (uint16_t)((cpu->accumulator & 0xff00u) | flags);
    cpu->x = END_SLOT;
    cpu->carry = 1;
    cpu->zero = 1;
    cpu->negative = 0;
    return ExecutionReturned(0x80c1a6u);
}
