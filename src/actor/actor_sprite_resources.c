#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum { SPRITE_VRAM_START = 0x2000 };

static bool SpriteResourceContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x83u &&
        cpu->stack >= 0x1f04u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2ActorSetRecordOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SpriteResourceContext(cpu))
        return ExecutionHandoff(cpu, 0x83ab4fu);
    Lufia2ActorRecordOffsets(memory, cpu);
    return ExecutionReturned(0x83ab60u);
}

Lufia2ExecutionResult Lufia2SpriteReleaseAllocation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SpriteResourceContext(cpu))
        return ExecutionHandoff(cpu, 0x83abccu);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_B));
    OpLsrA(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    RorA8(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpAndValue(cpu, 0xf0u);
    TrbDirect8(memory, cpu, DP_SCRATCH_A);
    OpLsrA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpTax(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpTay(cpu);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_SPRITE_ALLOCATION_FLAGS));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x83abe8u);
}

Lufia2ExecutionResult Lufia2SpriteComputeVramBase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SpriteResourceContext(cpu))
        return ExecutionHandoff(cpu, 0x83abe9u);
    (void)memory;
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    for (unsigned bit = 0; bit < 4u; ++bit)
        OpAslA(cpu);
    OpAdcValue(cpu, SPRITE_VRAM_START);
    return ExecutionReturned(0x83abf3u);
}
