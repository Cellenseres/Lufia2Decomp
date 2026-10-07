#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    OBJECT_REGION_POSITION = 0xd04au,
    OBJECT_REGION_SIZE = 0xd04cu,
    OBJECT_REGION_PHASE = 0x7fd049u,
    OBJECT_REGION_RECORD = 0x7ef000u,
    OBJECT_REGION_SNAPSHOT = 0x7fd067u
};

static bool ObjectAnimationContext(const Lufia2CpuState *cpu, uint16_t minimum_stack) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
        cpu->stack >= minimum_stack && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult ObjectAnimationUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t ObjectAnimationCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t target, uint32_t site, uint8_t frame) {
    if (frame == 3u)
        SimulateJslFrame(memory, cpu, cpu->program_bank, (uint16_t)(site + 3u));
    else
        SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return child(context, cpu, target, site, frame);
}

Lufia2ExecutionResult Lufia2FieldSaveAnimationRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectAnimationContext(cpu, 0x1f20u))
        return ExecutionHandoff(cpu, 0x838927u);
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpSta(memory, cpu, OBJECT_REGION_SNAPSHOT);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    OpSta(memory, cpu, OBJECT_REGION_SNAPSHOT + 2u);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    OpSta(memory, cpu, OBJECT_REGION_SNAPSHOT + 4u);
    SetAccumulatorWidth(cpu, 1u);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    OpSta(memory, cpu, OBJECT_REGION_SNAPSHOT + 6u);
    return ExecutionReturned(0x83894bu);
}

Lufia2ExecutionResult Lufia2FieldRestoreAnimationRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ObjectAnimationContext(cpu, 0x1f20u))
        return ExecutionHandoff(cpu, 0x83894cu);
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, OBJECT_REGION_SNAPSHOT);
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpLda(memory, cpu, OBJECT_REGION_SNAPSHOT + 2u);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    OpLda(memory, cpu, OBJECT_REGION_SNAPSHOT + 4u);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    SetAccumulatorWidth(cpu, 1u);
    OpLda(memory, cpu, OBJECT_REGION_SNAPSHOT + 6u);
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_FLAGS);
    return ExecutionReturned(0x838970u);
}

Lufia2ExecutionResult Lufia2FieldRedrawAnimatedRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const uint32_t targets[] = {
        0x838927u, 0x80d227u, 0x80d1e1u, 0x83894cu,
        0x838b6au, 0x80d19fu, 0x83894cu, 0x838a0au
    };
    static const uint32_t sites[] = {
        0x838971u, 0x838974u, 0x838978u, 0x83897cu,
        0x83897fu, 0x838983u, 0x838987u, 0x83898au
    };
    static const uint8_t frames[] = {2u, 3u, 3u, 2u, 3u, 3u, 2u, 2u};
    if (!child || !ObjectAnimationContext(cpu, 0x1f40u))
        return ExecutionHandoff(cpu, 0x838971u);
    for (unsigned step = 0u; step < 8u; ++step) {
        if (!ObjectAnimationCall(memory, cpu, child, context,
                targets[step], sites[step], frames[step]))
            return ObjectAnimationUnwound(sites[step]);
    }
    return ExecutionReturned(0x83898du);
}

Lufia2ExecutionResult Lufia2FieldQueueObjectControlSound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ObjectAnimationContext(cpu, 0x1f40u))
        return ExecutionHandoff(cpu, 0x83881du);
    OpLda(memory, cpu, 0x0009a7u);
    OpBitValue(cpu, 2u);
    if (!cpu->zero)
        return ExecutionReturned(0x838847u);
    OpLda(memory, cpu, 0x000583u);
    if (cpu->negative)
        return ExecutionReturned(0x838847u);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_CONTROL_KIND);
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        OpLoadA(cpu, 0x1eu);
    } else {
        OpLda(memory, cpu, OpLongX(cpu, OBJECT_REGION_RECORD + 4u));
        OpCmpValue(cpu, 2u);
        OpLoadA(cpu, 0x0eu);
        if (cpu->carry)
            OpLoadA(cpu, 0x0fu);
    }
    if (!ObjectAnimationCall(memory, cpu, child, context,
            0x848766u, 0x838843u, 3u))
        return ObjectAnimationUnwound(0x838843u);
    return ExecutionReturned(0x838847u);
}

static Lufia2ExecutionResult ObjectRegionSlide(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, bool closing) {
    const uint32_t entry = closing ? 0x83888cu : 0x8388d9u;
    if (!child || !ObjectAnimationContext(cpu, 0x1f40u) || cpu->data_bank != 0x7fu)
        return ExecutionHandoff(cpu, entry);
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_REGION_RECORD + (closing ? 6u : 2u)));
    OpSta(memory, cpu, OpAbs(cpu, OBJECT_REGION_POSITION));
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_REGION_RECORD + (closing ? 8u : 4u)));
    OpSta(memory, cpu, OpAbs(cpu, OBJECT_REGION_SIZE));
    SetAccumulatorWidth(cpu, 1u);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    cpu->carry = closing ? 0u : 1u;
    if (closing)
        OpAdc(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    else
        OpSbcValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_WIDTH));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    OpLda(memory, cpu, OBJECT_REGION_PHASE);
    OpBitValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, 0x000583u);
        if (cpu->negative)
            return ExecutionReturned(closing ? 0x8388b9u : 0x838926u);
    } else {
        OpLda(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
        cpu->carry = closing ? 1u : 0u;
        if (closing)
            OpSbcValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_OBJECT_WIDTH));
        else
            OpAdc(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
        OpSta(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    }
    const uint32_t redraw_site = closing ? 0x8388c7u : 0x838915u;
    if (!ObjectAnimationCall(memory, cpu, child, context, 0x838971u, redraw_site, 2u))
        return ObjectAnimationUnwound(redraw_site);
    const uint32_t tile_site = closing ? 0x8388cau : 0x838918u;
    const uint32_t tile_target = closing ? 0x83898eu : 0x8389ceu;
    if (!ObjectAnimationCall(memory, cpu, child, context, tile_target, tile_site, 2u))
        return ObjectAnimationUnwound(tile_site);
    OpStepMem(memory, cpu, OpAbs(cpu, 0xd04du), 1);
    OpLda(memory, cpu, 0x0005aau);
    const uint32_t attributes_site = closing ? 0x8388d4u : 0x838922u;
    if (!ObjectAnimationCall(memory, cpu, child, context,
            0x838c8au, attributes_site, 3u))
        return ObjectAnimationUnwound(attributes_site);
    return ExecutionReturned(closing ? 0x8388d8u : 0x838926u);
}

Lufia2ExecutionResult Lufia2FieldCloseAnimatedRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return ObjectRegionSlide(memory, cpu, child, context, true);
}

Lufia2ExecutionResult Lufia2FieldOpenAnimatedRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return ObjectRegionSlide(memory, cpu, child, context, false);
}

static Lufia2ExecutionResult ObjectApplyRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, bool alternate) {
    const uint32_t entry = alternate ? 0x838761u : 0x83873fu;
    if (!child || !ObjectAnimationContext(cpu, 0x1f40u))
        return ExecutionHandoff(cpu, entry);
    if (!ObjectAnimationCall(memory, cpu, child, context, 0x838874u, entry, 3u))
        return ObjectAnimationUnwound(entry);
    if (!cpu->carry)
        return ExecutionReturned(alternate ? 0x838782u : 0x838760u);
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_REGION_RECORD + (alternate ? 6u : 2u)));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_SOURCE_X);
    OpLda(memory, cpu, OpLongX(cpu, OBJECT_REGION_RECORD + (alternate ? 8u : 4u)));
    OpSta(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    SetAccumulatorWidth(cpu, 1u);
    const uint32_t copy_site = alternate ? 0x83877bu : 0x838759u;
    if (!ObjectAnimationCall(memory, cpu, child, context, 0x838b6au, copy_site, 3u))
        return ObjectAnimationUnwound(copy_site);
    const uint32_t tile_site = alternate ? 0x83877fu : 0x83875du;
    if (!ObjectAnimationCall(memory, cpu, child, context,
            alternate ? 0x83898eu : 0x8389ceu, tile_site, 2u))
        return ObjectAnimationUnwound(tile_site);
    return ExecutionReturned(alternate ? 0x838782u : 0x838760u);
}

Lufia2ExecutionResult Lufia2FieldApplyInitialObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return ObjectApplyRegion(memory, cpu, child, context, false);
}

Lufia2ExecutionResult Lufia2FieldApplyAlternateObjectRegion(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return ObjectApplyRegion(memory, cpu, child, context, true);
}

Lufia2ExecutionResult Lufia2FieldAnimateObjectAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ObjectAnimationContext(cpu, 0x1f40u))
        return ExecutionHandoff(cpu, 0x838783u);
    PushDataBank(memory, cpu);
    if (!ObjectAnimationCall(memory, cpu, child, context, 0x8387a3u, 0x838784u, 3u))
        return ObjectAnimationUnwound(0x838784u);
    if (!ObjectAnimationCall(memory, cpu, child, context, 0x838874u, 0x838788u, 3u))
        return ObjectAnimationUnwound(0x838788u);
    if (cpu->carry) {
        OpLoadA(cpu, 0x7fu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        OpLda(memory, cpu, OpAbs(cpu, 0xd049u));
        OpBitValue(cpu, 0x40u);
        const uint32_t site = cpu->zero ? 0x838799u : 0x83879eu;
        const uint32_t target = cpu->zero ? 0x8388d9u : 0x83888cu;
        if (!ObjectAnimationCall(memory, cpu, child, context, target, site, 2u))
            return ObjectAnimationUnwound(site);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8387a2u);
}

Lufia2ExecutionResult Lufia2FieldAnimateObjectControl(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ObjectAnimationContext(cpu, 0x1f40u))
        return ExecutionHandoff(cpu, 0x8387ccu);
    PushDataBank(memory, cpu);
    OpLda(memory, cpu, WRAM_FIELD_SELECTED_OBJECT_RECORD_ID);
    if (!ObjectAnimationCall(memory, cpu, child, context, 0x838848u, 0x8387d1u, 2u))
        return ObjectAnimationUnwound(0x8387d1u);
    if (!ObjectAnimationCall(memory, cpu, child, context, 0x838874u, 0x8387d4u, 3u))
        return ObjectAnimationUnwound(0x8387d4u);
    if (!cpu->carry) {
        PullDataBank(memory, cpu);
        return ExecutionReturned(0x83881cu);
    }
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0xd049u));
    OpBitValue(cpu, 0x40u);
    const bool closing = !cpu->zero;
    OpBitValue(cpu, 1u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, 0xd04eu));
        const uint32_t test_site = closing ? 0x838808u : 0x8387ecu;
        if (!ObjectAnimationCall(memory, cpu, child, context, 0x838ac9u, test_site, 3u))
            return ObjectAnimationUnwound(test_site);
        if (closing ? cpu->zero : !cpu->zero) {
            PullDataBank(memory, cpu);
            return ExecutionReturned(0x83881cu);
        }
        OpLda(memory, cpu, OpAbs(cpu, 0xd04eu));
        const uint32_t flag_site = closing ? 0x838811u : 0x8387f5u;
        const uint32_t flag_target = closing ? 0x838ae5u : 0x838ad5u;
        if (!ObjectAnimationCall(memory, cpu, child, context, flag_target, flag_site, 3u))
            return ObjectAnimationUnwound(flag_site);
        const uint32_t sound_site = closing ? 0x838815u : 0x8387f9u;
        if (!ObjectAnimationCall(memory, cpu, child, context, 0x83881du, sound_site, 2u))
            return ObjectAnimationUnwound(sound_site);
    }
    const uint32_t redraw_site = closing ? 0x838818u : 0x8387fcu;
    if (!ObjectAnimationCall(memory, cpu, child, context,
            closing ? 0x83888cu : 0x8388d9u, redraw_site, 2u))
        return ObjectAnimationUnwound(redraw_site);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x83881cu);
}
