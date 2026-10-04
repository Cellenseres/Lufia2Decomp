/* Callers fill the RAM block-move bank operands. */

#include "core/cpu_ops.h"
#include "core/cpu_internal.h"
#include "lufia2/system.h"

enum {
    STUB_OPCODE = 0x057du,
    STUB_DESTINATION = 0x057eu,
    STUB_SOURCE = 0x057fu,
    STUB_RETURN = 0x0580u,
    OPCODE_MVN = 0x54u,
    OPCODE_RTS = 0x60u,
    WORK_BANK = 0x7eu,
    SECOND_WORK_BANK = 0x7fu,
    COPY_OUTPUT_MIN = 0x2000u,
    COPY_OUTPUT_MAX = 0xffffu,
    COPY_STACK_MIN = 0x1f00u,
    COPY_STACK_MAX = 0x1ffcu
};

/* The copy must leave its live code and caller frame untouched. */
static bool BlockMoveBufferReady(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu) {
    const uint8_t code_bank = cpu->program_bank;
    const uint32_t bank = (uint32_t)code_bank << 16;
    uint8_t destination;

    if (cpu->index_is_8_bit || cpu->stack < COPY_STACK_MIN ||
        cpu->stack > COPY_STACK_MAX ||
        !(code_bank < 0x40u || code_bank == WORK_BANK ||
          code_bank == SECOND_WORK_BANK ||
          (code_bank >= 0x80u && code_bank < 0xc0u)))
        return false;
    if (Read8(memory, bank | STUB_OPCODE) != OPCODE_MVN ||
        Read8(memory, bank | STUB_RETURN) != OPCODE_RTS)
        return false;
    destination = Read8(memory, bank | STUB_DESTINATION);
    return (destination == WORK_BANK || destination == SECOND_WORK_BANK) &&
        cpu->y >= COPY_OUTPUT_MIN &&
        (uint32_t)cpu->y + cpu->accumulator <= COPY_OUTPUT_MAX;
}

/* The original reads the bank operands again for every copied byte. */
Lufia2ExecutionResult Lufia2RamBlockMove(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint32_t bank = (uint32_t)cpu->program_bank << 16;

    if (!BlockMoveBufferReady(memory, cpu))
        return ExecutionHandoff(cpu, bank | STUB_OPCODE);
    for (;;) {
        const uint8_t destination = Read8(memory, bank | STUB_DESTINATION);
        const uint8_t source = Read8(memory, bank | STUB_SOURCE);
        const uint8_t value = Read8(memory, ((uint32_t)source << 16) | cpu->x);

        cpu->data_bank = destination;
        Write8(memory, ((uint32_t)destination << 16) | cpu->y, value);
        cpu->x = OpIndexValue(cpu, (uint16_t)(cpu->x + 1u));
        cpu->y = OpIndexValue(cpu, (uint16_t)(cpu->y + 1u));
        cpu->accumulator = (uint16_t)(cpu->accumulator - 1u);
        if (cpu->accumulator == 0xffffu) {
            if (Read8(memory, bank | STUB_RETURN) != OPCODE_RTS)
                return ExecutionHandoff(cpu, bank | STUB_RETURN);
            return ExecutionReturned(bank | STUB_RETURN);
        }
        if (Read8(memory, bank | STUB_OPCODE) != OPCODE_MVN)
            return ExecutionHandoff(cpu, bank | STUB_OPCODE);
    }
}
