#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2MenuPartyMemberMask(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x829302u);
    OpSepWidths(cpu, 0x10u);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_MENU_PARTY_FIRST_ID));
    OpIncA(cpu);
    OpTay(cpu);
    OpLoadA(cpu, 0u);
    cpu->carry = 1u;
    do {
        RolA8(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x829312u);
}
