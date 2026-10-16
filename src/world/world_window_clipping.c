#include "core/child_call.h"
#include "lufia2/world_map.h"
#include "world/world_scene_resources_internal.h"

enum {
    CLIP_MULTIPLICAND = 0u,
    CLIP_PRODUCT_INTEGER = 1u,
    CLIP_MULTIPLIER = 4u,
    CLIP_ANGLE = 0x11u,
    CLIP_X = 0x15u,
    CLIP_Y = 0x17u,
    CLIP_WIDTH = 0x100u,
    CLIP_HEIGHT = 0xe0u,
    CLIP_SLOPES = 0xdbb9u
};

typedef struct WindowClip {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} WindowClip;

static bool CallWindowClipChild(WindowClip *clip, uint32_t site, uint32_t target) {
    if (!CallChildWithFrame(clip->memory, clip->cpu, clip->child, clip->context, site,
                            target, 2u, 0x86u)) {
        clip->result = ExecutionReturned(site);
        clip->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceMode(clip->cpu, false)) {
        clip->result = ExecutionHandoff(clip->cpu, site + 3u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2WorldMapResolveWindowRightIntersection(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2PushedChildCall child,
    void *context) {
    if (!child || !WorldSceneResourceMode(cpu, false))
        return ExecutionHandoff(cpu, 0x86cb9eu);
    WindowClip clip = {memory, cpu, child, context, ExecutionReturned(0x86cbbeu)};
    OpLda(memory, cpu, OpDp(cpu, CLIP_X));
    cpu->carry = 1u;
    OpSbcValue(cpu, CLIP_WIDTH - 1u);
    OpSta(memory, cpu, OpDp(cpu, CLIP_MULTIPLIER));
    OpLoadA(cpu, 0x40u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, CLIP_ANGLE)));
    OpAndValue(cpu, 0x3fu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, CLIP_SLOPES));
    OpSta(memory, cpu, OpDp(cpu, CLIP_MULTIPLICAND));
    if (!CallWindowClipChild(&clip, 0x86cbb6u, 0x86a52fu))
        return clip.result;
    OpLda(memory, cpu, OpDp(cpu, CLIP_Y));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, CLIP_PRODUCT_INTEGER)));
    return clip.result;
}

static void RejectWindowEndpoint(WindowClip *clip) {
    OpLoadA(clip->cpu, 0xffffu);
    OpSta(clip->memory, clip->cpu, OpDp(clip->cpu, CLIP_Y));
    clip->result = ExecutionReturned(0x86cb9du);
}

static void KeepWindowRightIntersection(WindowClip *clip) {
    OpSta(clip->memory, clip->cpu, OpDp(clip->cpu, CLIP_Y));
    OpLoadA(clip->cpu, CLIP_WIDTH - 1u);
    OpSta(clip->memory, clip->cpu, OpDp(clip->cpu, CLIP_X));
}

static void ClipWindowBottom(WindowClip *clip) {
    const Lufia2Memory *memory = clip->memory;
    Lufia2CpuState *cpu = clip->cpu;
    OpLda(memory, cpu, OpDp(cpu, CLIP_Y));
    cpu->carry = 1u;
    OpSbcValue(cpu, CLIP_HEIGHT - 1u);
    OpSta(memory, cpu, OpDp(cpu, CLIP_MULTIPLIER));
    OpLda(memory, cpu, OpDp(cpu, CLIP_ANGLE));
    OpAndValue(cpu, 0x3fu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, CLIP_SLOPES));
    OpSta(memory, cpu, OpDp(cpu, CLIP_MULTIPLICAND));
    if (!CallWindowClipChild(clip, 0x86cb77u, 0x86a52fu))
        return;
    OpLda(memory, cpu, OpDp(cpu, CLIP_X));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, CLIP_PRODUCT_INTEGER)));
    OpSta(memory, cpu, OpDp(cpu, CLIP_X));
    OpLoadA(cpu, CLIP_HEIGHT - 1u);
    OpSta(memory, cpu, OpDp(cpu, CLIP_Y));
}

Lufia2ExecutionResult Lufia2WorldMapClipWindowLowerRight(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!child || !WorldSceneResourceMode(cpu, false))
        return ExecutionHandoff(cpu, 0x86cb49u);
    WindowClip clip = {memory, cpu, child, context, ExecutionReturned(0x86cb97u)};
    OpLda(memory, cpu, OpDp(cpu, CLIP_X));
    if (cpu->negative) {
        RejectWindowEndpoint(&clip);
        return clip.result;
    }
    OpCmpValue(cpu, CLIP_WIDTH);
    if (!cpu->carry) {
        ClipWindowBottom(&clip);
        return clip.result;
    }
    OpLda(memory, cpu, OpDp(cpu, CLIP_Y));
    if (cpu->negative) {
        RejectWindowEndpoint(&clip);
        return clip.result;
    }
    OpCmpValue(cpu, CLIP_HEIGHT);
    if (!cpu->carry) {
        if (!CallWindowClipChild(&clip, 0x86cb88u, 0x86cb9eu))
            return clip.result;
        OpCmpValue(cpu, CLIP_HEIGHT);
        if (cpu->carry)
            RejectWindowEndpoint(&clip);
        else
            KeepWindowRightIntersection(&clip);
        return clip.result;
    }
    if (!CallWindowClipChild(&clip, 0x86cb5bu, 0x86cb9eu))
        return clip.result;
    OpCmpValue(cpu, CLIP_HEIGHT);
    if (cpu->carry)
        ClipWindowBottom(&clip);
    else
        KeepWindowRightIntersection(&clip);
    return clip.result;
}

Lufia2ExecutionResult Lufia2WorldMapClipWindowEndpoint(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86cb21u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, CLIP_X));
    OpCmpValue(cpu, CLIP_WIDTH);
    bool outside = cpu->carry;
    if (!outside) {
        OpLda(memory, cpu, OpDp(cpu, CLIP_Y));
        OpCmpValue(cpu, CLIP_HEIGHT);
        outside = cpu->carry;
    }
    if (outside) {
        OpLda(memory, cpu, OpDp(cpu, CLIP_ANGLE));
        for (unsigned bit = 0u; bit < 3u; ++bit)
            OpAslA(cpu);
        ExchangeAccumulatorBytes(cpu);
        OpAndValue(cpu, 6u);
        OpTax(cpu);
        uint32_t target =
            0x860000u | Read16Bank(memory, 0x86u, (uint16_t)(0xcb41u + cpu->x));
        if (!CallChildWithFrame(memory, cpu, child, context, 0x86cb3bu, target, 2u,
                                0x86u)) {
            Lufia2ExecutionResult result = ExecutionReturned(0x86cb3bu);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        if (cpu->program_bank != 0x86u || cpu->data_bank != 0x86u ||
            cpu->index_is_8_bit || cpu->direct_page || cpu->decimal)
            return ExecutionHandoff(cpu, 0x86cb3eu);
    }
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x86cb40u);
}
