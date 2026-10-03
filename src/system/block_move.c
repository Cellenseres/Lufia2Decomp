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
    OPCODE_RTS = 0x60u
};

/* MVN re-fetches operands after each byte, including self-overwrites. */
Lufia2ExecutionResult Lufia2RamBlockMove(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint32_t bank = (uint32_t)cpu->program_bank << 16;

    if (Read8(memory, bank | STUB_OPCODE) != OPCODE_MVN ||
        Read8(memory, bank | STUB_RETURN) != OPCODE_RTS)
        return ExecutionHandoff(cpu, bank | 0x057du);
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
