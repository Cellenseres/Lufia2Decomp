/* Transitions between placed map objects and their claimed actors. */

#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    OBJECT_INDEX_SCRATCH = 0x54,
    OBJECT_VERTICAL_OFFSET = 0x58,
    OBJECT_RECORD_SIZE = 10,
    OBJECT_HEADER_SECTION = 0x16,
};

static uint8_t ObjectTransitionChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t target, uint32_t site, uint8_t frame_size) {
    const uint16_t return_word = (uint16_t)(site + frame_size);
    if (frame_size == 3u)
        SimulateJslFrame(memory, cpu, 0x83u, return_word);
    else
        SimulateJsrFrame(memory, cpu, return_word);
    return child(context, cpu, target, site, frame_size);
}

static Lufia2ExecutionResult ObjectTransitionUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2FieldLoadObjectRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, OBJECT_RECORD_SIZE);
    ExchangeAccumulatorBytes(cpu);
    OpLdx(cpu, OBJECT_HEADER_SECTION);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x80bfaau, 0x838b47u, 3u))
        return ObjectTransitionUnwound(0x838b47u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef002u));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef004u));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x7ef001u));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    return ExecutionReturned(0x838b69u);
}

Lufia2ExecutionResult Lufia2FieldInitializeObjectActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x838b40u, 0x83f5f0u, 3u))
        return ObjectTransitionUnwound(0x83f5f0u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f7f8u, 0x83f5f4u, 2u))
        return ObjectTransitionUnwound(0x83f5f4u);
    (void)Lufia2ActorResetObjectOffsets(memory, cpu);
    OpTxa(cpu);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f6cau, 0x83f60du, 2u))
        return ObjectTransitionUnwound(0x83f60du);
    return ExecutionReturned(0x83f610u);
}

Lufia2ExecutionResult Lufia2FieldRefreshObjectActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x838b40u, 0x83f5bfu, 3u))
        return ObjectTransitionUnwound(0x83f5bfu);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f7f8u, 0x83f5c3u, 2u))
        return ObjectTransitionUnwound(0x83f5c3u);
    (void)Lufia2ActorResetObjectOffsets(memory, cpu);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f611u, 0x83f5dbu, 2u))
        return ObjectTransitionUnwound(0x83f5dbu);
    OpTxa(cpu);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x838a6fu, 0x83f5dfu, 3u))
        return ObjectTransitionUnwound(0x83f5dfu);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f933u, 0x83f5e3u, 2u))
        return ObjectTransitionUnwound(0x83f5e3u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f6cau, 0x83f5e6u, 2u))
        return ObjectTransitionUnwound(0x83f5e6u);
    return ExecutionReturned(0x83f5e9u);
}

Lufia2ExecutionResult Lufia2FieldPlaceActorObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x838b40u, 0x83f7b7u, 3u))
        return ObjectTransitionUnwound(0x83f7b7u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f422u, 0x83f7bbu, 3u))
        return ObjectTransitionUnwound(0x83f7bbu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_PENDING_OBJECT));
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f86bu, 0x83f7c5u, 3u))
        return ObjectTransitionUnwound(0x83f7c5u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f933u, 0x83f7c9u, 2u))
        return ObjectTransitionUnwound(0x83f7c9u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f7dfu, 0x83f7ccu, 2u))
        return ObjectTransitionUnwound(0x83f7ccu);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f7d4u, 0x83f7cfu, 3u))
        return ObjectTransitionUnwound(0x83f7cfu);
    return ExecutionReturned(0x83f7d3u);
}

Lufia2ExecutionResult Lufia2FieldRebuildActorObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    /* The original LDA leaves the caller's X in use for the following load. */
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x838b40u, 0x83f79bu, 3u))
        return ObjectTransitionUnwound(0x83f79bu);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f7f8u, 0x83f79fu, 2u))
        return ObjectTransitionUnwound(0x83f79fu);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x838b6au, 0x83f7a2u, 3u))
        return ObjectTransitionUnwound(0x83f7a2u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f933u, 0x83f7a6u, 2u))
        return ObjectTransitionUnwound(0x83f7a6u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f7dfu, 0x83f7a9u, 2u))
        return ObjectTransitionUnwound(0x83f7a9u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f7d4u, 0x83f7acu, 3u))
        return ObjectTransitionUnwound(0x83f7acu);
    return ExecutionReturned(0x83f7b0u);
}

Lufia2ExecutionResult Lufia2FieldClaimPlacedObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83fb9fu, 0x83f620u, 2u))
        return ObjectTransitionUnwound(0x83f620u);
    OpWriteX(memory, cpu, OpDp(cpu, OBJECT_INDEX_SCRATCH), cpu->x);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_OBJECT_RECORD));
    cpu->carry = 1;
    OpSbcValue(cpu, 0x0010u);
    OpSta(memory, cpu, WRAM_FIELD_CLAIMED_OBJECT_RECORD);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
    OpLda(memory, cpu, OpDp(cpu, OBJECT_INDEX_SCRATCH));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_PENDING_OBJECT));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_OBJECT_RECORD));
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x838b40u, 0x83f640u, 3u))
        return ObjectTransitionUnwound(0x83f640u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f422u, 0x83f644u, 3u))
        return ObjectTransitionUnwound(0x83f644u);
    OpLoadA(cpu, 0x00f7u);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpStz(memory, cpu, OpDp(cpu, 0x55u));
    OpLoadA(cpu, 0x00bfu);
    OpSta(memory, cpu, OpDp(cpu, 0x56u));
    OpStz(memory, cpu, OpDp(cpu, 0x57u));
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f80du, 0x83f654u, 2u))
        return ObjectTransitionUnwound(0x83f654u);
    OpBitValue(cpu, 0x0040u);
    if (!cpu->zero) {
        PushDataBank(memory, cpu);
        OpSetDataBank(memory, cpu, 0x7fu);
        OpLda(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
        if (!cpu->zero)
            OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), 1);
        if (!ObjectTransitionChild(memory, cpu, child, context,
                0x83fb9fu, 0x83f666u, 2u))
            return ObjectTransitionUnwound(0x83f666u);
        OpTxy(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), 1);
        if (!ObjectTransitionChild(memory, cpu, child, context,
                0x83fb9fu, 0x83f66cu, 2u))
            return ObjectTransitionUnwound(0x83f66cu);
        if (!ObjectTransitionChild(memory, cpu, child, context,
                0x83f85au, 0x83f66fu, 2u))
            return ObjectTransitionUnwound(0x83f66fu);
        OpLda(memory, cpu, OpAbsY(cpu,
            (WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu)));
        OpSta(memory, cpu, OpAbsX(cpu,
            (WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu)));
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), -1);
        if (!ObjectTransitionChild(memory, cpu, child, context,
                0x83f9d4u, 0x83f67cu, 2u))
            return ObjectTransitionUnwound(0x83f67cu);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0x0000u));
        OpSta(memory, cpu, OpAbsY(cpu,
            (WRAM_FIELD_PENDING_OBJECT_TILES & 0xffffu)));
        OpSepWidths(cpu, 0x20u);
        PullDataBank(memory, cpu);
        OpLda(memory, cpu, OpDp(cpu, OBJECT_VERTICAL_OFFSET));
        if (!cpu->zero)
            OpStepMem(memory, cpu, OpDp(cpu, DP_PROBE_Y), -1);
    }
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f6b0u, 0x83f690u, 2u))
        return ObjectTransitionUnwound(0x83f690u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f747u, 0x83f693u, 3u))
        return ObjectTransitionUnwound(0x83f693u);
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f933u, 0x83f697u, 2u))
        return ObjectTransitionUnwound(0x83f697u);
    TransferDirectToA(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_CLAIMED_PENDING_OBJECT));
    OpTax(cpu);
    OpLoadA(cpu, 0x00ffu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_RECORD_X));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_PENDING_RECORD_Y));
    if (!ObjectTransitionChild(memory, cpu, child, context,
            0x83f6cau, 0x83f6acu, 2u))
        return ObjectTransitionUnwound(0x83f6acu);
    return ExecutionReturned(0x83f6afu);
}
