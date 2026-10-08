#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/party.h"

enum { CAPSULE_STATUS_BYTES = WRAM_CAPSULE_FORM_STATES & 0xffffu };

Lufia2ExecutionResult Lufia2CapsuleGetStatusAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || cpu->decimal ||
        cpu->direct_page != 0u || cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82c482u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_SELECTED_ID));
    OpAndValue(cpu, 0xffu);
    cpu->carry = 0u;
    OpAdcValue(cpu, CAPSULE_STATUS_BYTES);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82c491u);
}
