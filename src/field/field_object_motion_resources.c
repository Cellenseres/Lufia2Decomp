#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum {
    OBJECT_SPRITE_ALLOCATION_COUNTS = 0x83abf4u
};

typedef Lufia2ExecutionResult (*ObjectResourceStep)(
    const Lufia2Memory *, Lufia2CpuState *);

static bool ObjectResourceContext(const Lufia2CpuState *cpu,
                                  uint16_t minimum_stack) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= minimum_stack && cpu->stack <= 0x1ffcu;
}

static bool ObjectResourceCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    ObjectResourceStep step, uint8_t bank, uint16_t back, uint8_t frame_size) {
    if (frame_size == 3u)
        SimulateJslFrame(memory, cpu, 0x83u, back);
    else
        SimulateJsrFrame(memory, cpu, back);
    cpu->program_bank = bank;
    Lufia2ExecutionResult result = step(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    uint8_t low = Pull8(memory, cpu);
    uint8_t high = Pull8(memory, cpu);
    uint8_t caller = frame_size == 3u ? Pull8(memory, cpu) : 0x83u;
    uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
    cpu->program_bank = caller;
    if (actual == back && caller == 0x83u)
        return true;
    cpu->resume_pc = ((uint32_t)caller << 16) | (uint16_t)(actual + 1u);
    return false;
}

Lufia2ExecutionResult Lufia2FieldSetPendingProbePosition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectResourceContext(cpu, 0x1f00u))
        return ExecutionHandoff(cpu, 0x83f435u);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    return ExecutionReturned(0x83f441u);
}

Lufia2ExecutionResult Lufia2ObjectStartInteractionEvent(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectResourceContext(cpu, 0x1f30u))
        return ExecutionHandoff(cpu, 0x83ec4fu);
    PushY(memory, cpu);
    if (!ObjectResourceCall(memory, cpu, Lufia2ObjectProbeNextTile,
        0x83u, 0xec52u, 2u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpLdx(cpu, 0x2au);
    OpLdy(cpu, 0x0eu);
    if (!ObjectResourceCall(memory, cpu, Lufia2FieldStartEventAtProbe,
        0x80u, 0xec5cu, 3u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpPullY(memory, cpu);
    return ExecutionReturned(0x83ec5eu);
}

Lufia2ExecutionResult Lufia2ObjectAllocateSpriteResources(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectResourceContext(cpu, 0x1f10u))
        return ExecutionHandoff(cpu, 0x83ecdeu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_OBJECT_STATE));
    OpAndValue(cpu, 0xfbu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_OBJECT_STATE));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SIZE));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_SPRITE_ALLOCATION_COUNTS));
    if (!ObjectResourceCall(memory, cpu, Lufia2SpriteReserveAllocation,
        0x83u, 0xecf6u, 3u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_A));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_SLOT_B));
    if (!ObjectResourceCall(memory, cpu, Lufia2SpriteComputeVramBase,
        0x83u, 0xed05u, 3u))
        return ExecutionHandoff(cpu, cpu->resume_pc);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_SPRITE_VRAM_BASE));
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x83ed10u);
}
