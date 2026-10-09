#include "core/cpu_ops.h"
#include "lufia2/menu.h"

enum {
    DP_PALETTE_DESTINATION = 0x02u,
    SELECTION_PALETTE_SOURCE = 0xbf00u,
    SELECTION_PALETTE_SOURCE_BANK = 0xa6u,
    SELECTION_PALETTE_DESTINATION = 0x04a0u,
    SELECTION_PALETTE_BYTES = 0x20u,
    SELECTION_PALETTE_FIRST = 1u,
    SELECTION_PALETTE_END = 3u
};

Lufia2ExecutionResult Lufia2MenuLoadSelectionPalettes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x86916cu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, SELECTION_PALETTE_DESTINATION);
    OpWriteX(memory, cpu, OpDp(cpu, DP_PALETTE_DESTINATION), cpu->x);
    OpLdx(cpu, SELECTION_PALETTE_FIRST);
    do {
        OpPushX(memory, cpu);
        PushDataBank(memory, cpu);
        OpTxa(cpu);
        for (unsigned bit = 0u; bit < 5u; ++bit)
            OpAslA(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, SELECTION_PALETTE_SOURCE);
        OpTax(cpu);
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_PALETTE_DESTINATION)));
        OpLoadA(cpu, SELECTION_PALETTE_BYTES - 1u);
        OpMoveNext(memory, cpu, 0u, SELECTION_PALETTE_SOURCE_BANK);
        PullDataBank(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, DP_PALETTE_DESTINATION));
        cpu->carry = 0u;
        OpAdcValue(cpu, SELECTION_PALETTE_BYTES);
        OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_DESTINATION));
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpCpx(cpu, SELECTION_PALETTE_END);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86919du);
}
