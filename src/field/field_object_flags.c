/* Flag bytes of the field object slots. */

#include "core/cpu_internal.h"
#include "lufia2/field.h"

enum {
    FLAGS = 0x0622u,
    FIRST_SLOT = 0x0005u,
    END_SLOT = 0x0028u,
    FLAG_BIT = 0x01u
};

/* $80:C195: sets bit 0 of the flag bytes of slots 5 to $27; M8/X16, JSL. */
Lufia2ExecutionResult Lufia2FieldMarkObjectSlots(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x80c195u);
    LoadX16(cpu, FIRST_SLOT);
    do {
        LoadA8(cpu, AbsoluteByte(memory, cpu, FLAGS, cpu->x));
        Or8(cpu, FLAG_BIT);
        StoreAAbsolute8(memory, cpu, FLAGS, cpu->x);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, END_SLOT);
    } while (!cpu->zero);
    return ExecutionReturned(0x80c1a6u);
}
