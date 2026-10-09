#include "core/cpu_ops.h"
#include "lufia2/system.h"

enum {
    DP_SAVE_CHECKSUM = 0x22u,
    DP_SAVE_CHECKSUM_LENGTH = 0x26u,
    DP_SAVE_CHECKSUM_SOURCE = 0x2au
};

Lufia2ExecutionResult Lufia2SaveRecordChecksum(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x85de6cu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushY(memory, cpu);
    PushIndex(memory, cpu);
    PushAccumulator16(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SAVE_CHECKSUM_LENGTH)));
    OpDey(cpu);
    TransferDirectToA(cpu);
    cpu->carry = 1u;
    do {
        OpAdc(memory, cpu, DirectLongIndirectY(memory, cpu, DP_SAVE_CHECKSUM_SOURCE));
        ExchangeAccumulatorBytes(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    ExchangeAccumulatorBytes(cpu);
    OpAdcValue(cpu, 0u);
    OpTay(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_SAVE_CHECKSUM), cpu->y);
    OpRepWidths(cpu, 0x30u);
    PullAccumulator16(memory, cpu);
    OpPullX(memory, cpu);
    OpPullY(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85de8du);
}
