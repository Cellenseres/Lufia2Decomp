#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum { ACTOR_SLOT = 0xa7u, ACTOR_SLOT_LIMIT = 40u };

/* Returns the RTL of the path taken, or $80:BFA4 when the child unwound. */
uint32_t Lufia2FieldSelectActorByIdBody(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    LoadX16(cpu, 0u);
    do {
        OpCmp(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
        if (cpu->zero) {
            Write16Direct(memory, cpu, ACTOR_SLOT, cpu->x);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x80bfa4u, 0x8482d5u, 3u, 0x80u))
                return 0x80bfa4u;
            cpu->carry = 0u;
            return 0x80bfa9u;
        }
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, ACTOR_SLOT_LIMIT);
    } while (!cpu->zero);
    cpu->carry = 1u;
    return 0x80bfa1u;
}

Lufia2ExecutionResult Lufia2FieldSelectActorById(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x80bf92u);
    const uint32_t exit = Lufia2FieldSelectActorByIdBody(memory, cpu, child, context);
    if (exit == 0x80bfa4u) {
        Lufia2ExecutionResult result = ExecutionReturned(exit);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    return ExecutionReturned(exit);
}
