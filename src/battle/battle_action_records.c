#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    IP_RECORD_POINTERS = 0x848f10u,
    IP_NAME_OFFSETS = 0x95f1efu,
    CAPSULE_ACTION_OFFSETS = 0x97f63bu
};

static bool ActionRecordContext(const Lufia2CpuState *cpu) {
    return !cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->program_bank == 0x81u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void ActionRecordOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint32_t table) {
    AslA16(cpu);
    LoadX16(cpu, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(table, cpu->x)));
    cpu->carry = false;
    Add16Value(cpu, (uint16_t)table);
}

Lufia2ExecutionResult Lufia2IpRecordPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ActionRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x81f45eu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    if (cpu->negative) {
        TransferDirectToA(cpu);
        return ExecutionReturned(0x81f46au);
    }
    AslA16(cpu);
    LoadX16(cpu, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(IP_RECORD_POINTERS, cpu->x)));
    cpu->carry = false;
    return ExecutionReturned(0x81f468u);
}

Lufia2ExecutionResult Lufia2IpNamePointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ActionRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x81f46bu);
    ActionRecordOffset(memory, cpu, IP_NAME_OFFSETS);
    return ExecutionReturned(0x81f475u);
}

Lufia2ExecutionResult Lufia2CapsuleActionRecordPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ActionRecordContext(cpu))
        return ExecutionHandoff(cpu, 0x81f476u);
    ActionRecordOffset(memory, cpu, CAPSULE_ACTION_OFFSETS);
    return ExecutionReturned(0x81f480u);
}
