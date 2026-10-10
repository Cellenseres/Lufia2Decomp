#include "lufia2/field.h"
#include "field_animation_queue_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "field/field_internal.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

void Lufia2FieldQueueAnimationSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_STATE));
        if (cpu->negative) {
            OpLda(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_OBJECT));
            OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_SCRATCH_D)));
            if (cpu->zero)
                return;
        }
        OpInx(cpu);
        OpCpx(cpu, EVENT_ANIMATION_SLOT_COUNT);
        if (cpu->carry)
            break;
    }
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_STATE));
        if (!cpu->negative)
            break;
        OpInx(cpu);
        OpCpx(cpu, EVENT_ANIMATION_SLOT_COUNT);
        if (cpu->zero) {
            OpLdx(cpu, 0u);
            break;
        }
    }
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    OpOraValue(cpu, 0x90u);
    OpSta(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_STATE));
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_D));
    OpSta(memory, cpu, OpLongX(cpu, EVENT_ANIMATION_SLOT_OBJECT));
}

static uint8_t AnimationQueueWidths(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

static Lufia2ExecutionResult AnimationQueueUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2FieldRequestObjectAnimation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!AnimationQueueWidths(cpu) || !child || cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83f559u);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_D));
    OpLda(memory, cpu, WRAM_BRIGHTNESS);
    if (!cpu->negative) {
        Lufia2FieldQueueAnimationSlot(memory, cpu);
        return ExecutionReturned(0x83f5b8u);
    }
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_D));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x83f566u, 0x838848u, 2u, 0x83u))
        return AnimationQueueUnwound(0x83f566u);
    if (!AnimationQueueWidths(cpu))
        return ExecutionHandoff(cpu, 0x83f569u);
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    OpBitValue(cpu, 0x40u);
    if (!cpu->zero) {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x83f57cu, 0x838761u, 2u, 0x83u))
            return AnimationQueueUnwound(0x83f57cu);
    } else {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x83f56fu, 0x83873fu, 2u, 0x83u))
            return AnimationQueueUnwound(0x83f56fu);
        if (!AnimationQueueWidths(cpu))
            return ExecutionHandoff(cpu, 0x83f572u);
        OpLda(memory, cpu, EVENT_OBJECT_OPERAND);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x83f576u, 0x838ad5u, 3u, 0x83u))
            return AnimationQueueUnwound(0x83f576u);
    }
    return ExecutionReturned(0x83f5b8u);
}
