#include "core/cpu_ops.h"
#include "lufia2/system.h"

/* $80:834C: 24-bit product of $4E.word and $50.byte, through SNES MMIO. */
Lufia2ExecutionResult Lufia2Multiply16By8(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, 0x53u));
    OpLda(memory, cpu, OpDp(cpu, 0x50u));
    OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
    OpLda(memory, cpu, OpDp(cpu, 0x4eu));
    OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
    OpLda(memory, cpu, OpDp(cpu, 0x4fu));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x50u));
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x4216u)));
    OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
    OpWriteX(memory, cpu, OpDp(cpu, 0x51u), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, 0x52u));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, 0x4216u));
    OpSta(memory, cpu, OpDp(cpu, 0x52u));
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return ExecutionReturned(0x808377u);
}
