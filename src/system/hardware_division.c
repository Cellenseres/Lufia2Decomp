#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/system.h"

enum {
    DP_DIVISOR = 0x54u,
    DP_DIVIDEND_LOW = 0x5du,
    DP_DIVIDEND_HIGH = 0x5eu,
    DP_QUOTIENT_HIGH = 0x5fu
};

Lufia2ExecutionResult Lufia2SystemDivide24ByByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x85dc6fu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, DP_DIVIDEND_HIGH)));
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WRDIVL), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, DP_DIVISOR));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVB));
    PushIndex(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_DIVIDEND_LOW));
    ExchangeAccumulatorBytes(cpu);
    OpLdx(cpu, OpRead16(memory, OpAbs(cpu, SNES_RDDIVL)));
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    ExchangeAccumulatorBytes(cpu);
    OpTay(cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WRDIVL), cpu->y);
    OpLda(memory, cpu, OpDp(cpu, DP_DIVISOR));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVB));
    OpTxa(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_QUOTIENT_HIGH));
    OpLoadA(cpu, 0u);
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDDIVL));
    OpSta(memory, cpu, OpDp(cpu, DP_DIVIDEND_LOW));
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85dca2u);
}
