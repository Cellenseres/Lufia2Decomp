/* The block move kept in work RAM at $057D: an MVN with its two bank
 * operands filled in by the caller, followed by an RTS. */

#include "core/cpu_ops.h"
#include "core/cpu_internal.h"
#include "lufia2/system.h"

enum {
    STUB_OPCODE = 0x7e057du,
    STUB_DESTINATION = 0x7e057eu,
    STUB_SOURCE = 0x7e057fu,
    STUB_RETURN = 0x7e0580u,
    OPCODE_MVN = 0x54u,
    OPCODE_RTS = 0x60u
};

/* $00:057D: copies A + 1 bytes ascending from the source bank at X to the
 * destination bank at Y; the data bank becomes the destination. Works in
 * every mirrored bank. A stub that does not hold the expected bytes is left
 * to the original code. */
Lufia2ExecutionResult Lufia2RamBlockMove(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint32_t bank = (uint32_t)cpu->program_bank << 16;

    if (Read8(memory, STUB_OPCODE) != OPCODE_MVN ||
        Read8(memory, STUB_RETURN) != OPCODE_RTS)
        return ExecutionHandoff(cpu, bank | 0x057du);
    OpMoveNext(memory, cpu, Read8(memory, STUB_DESTINATION),
        Read8(memory, STUB_SOURCE));
    return ExecutionReturned(bank | 0x0580u);
}
