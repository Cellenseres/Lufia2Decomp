#include "core/child_call.h"
#include "core/snes_registers.h"
#include "lufia2/world_map.h"
#include "system/wram.h"
#include "world/world_scene_internal.h"
#include "world/world_scene_resources_internal.h"

enum {
    FRAME_WINDOW_INDEX = 0x63u,
    FRAME_WINDOW_RECORDS = WRAM_WORLD_SCENE_WINDOW_RECORD_STORAGE,
    FRAME_SCENE_COUNTER = WRAM_FIELD_LAYER_SCROLL_Y,
    FRAME_COLOR_PHASE = WRAM_FIELD_LAYER_SCROLL_Y + 2u,
    FRAME_COLOR_DIRECTION = WRAM_FIELD_LAYER_SCROLL_Y + 4u,
    FRAME_COLOR_INTENSITY = WRAM_FIELD_LAYER_SCROLL_Y + 5u
};

typedef struct WindowFrame {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} WindowFrame;

static bool CallWindowFrameChild(WindowFrame *frame, uint32_t site, uint32_t target) {
    if (!CallChildWithFrame(frame->memory, frame->cpu, frame->child, frame->context,
                            site, target, 2u, 0x86u)) {
        frame->result = ExecutionReturned(site);
        frame->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WorldSceneResourceEntry(frame->cpu)) {
        frame->result = ExecutionHandoff(frame->cpu, site + 3u);
        return false;
    }
    return true;
}

static void ExpandSceneWindow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    cpu->carry = 0u;
    OpAdcValue(cpu, 10u);
    OpCmp(memory, cpu, WorldSceneObjectField(cpu, 12u));
    if (cpu->carry) {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, WorldSceneObjectField(cpu, 8u));
        OpLda(memory, cpu, WorldSceneObjectField(cpu, 12u));
    }
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 10u));
    OpLda(memory, cpu, WorldSceneObjectField(cpu, 8u));
    if (cpu->zero)
        return;
    cpu->carry = 0u;
    OpAdcValue(cpu, 20u);
    OpCmp(memory, cpu, WorldSceneObjectField(cpu, 12u));
    if (cpu->carry) {
        OpStz(memory, cpu, WorldSceneObjectField(cpu, 10u));
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, WorldSceneObjectField(cpu, 8u));
    } else {
        OpSta(memory, cpu, WorldSceneObjectField(cpu, 8u));
        OpLda(memory, cpu, WorldSceneObjectField(cpu, 12u));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, WorldSceneObjectField(cpu, 8u)));
        OpSta(memory, cpu, WorldSceneObjectField(cpu, 10u));
    }
}

static bool ExpandSceneCircle(WindowFrame *frame) {
    const Lufia2Memory *memory = frame->memory;
    Lufia2CpuState *cpu = frame->cpu;
    OpLdx(cpu, FRAME_WINDOW_RECORDS);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, 2u));
    if (cpu->zero)
        return true;
    OpIncA(cpu);
    OpCmpValue(cpu, 0x60u);
    if (cpu->carry)
        OpLoadA(cpu, 0x60u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 2u));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x48u);
    if (!cpu->carry)
        OpLoadA(cpu, 0u);
    OpLsrA(cpu);
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpIncA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, FRAME_COLOR_PHASE));
    OpLoadA(cpu, 0u);
    OpStz(memory, cpu, OpDp(cpu, 0u));
    return CallWindowFrameChild(frame, 0x86b0a1u, 0x86c2d9u);
}

Lufia2ExecutionResult
Lufia2WorldMapStepExpandingSceneWindows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b039u);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b0b9u)};
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
    do {
        OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
        if (!CallWindowFrameChild(&frame, 0x86b03fu, 0x86c536u))
            return frame.result;
        OpLda(memory, cpu, WorldSceneObjectField(cpu, 10u));
        if (!cpu->zero) {
            ExpandSceneWindow(memory, cpu);
            OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
            OpSta(memory, cpu, OpDp(cpu, 0u));
            if (!CallWindowFrameChild(&frame, 0x86b075u, 0x86c4e0u))
                return frame.result;
        }
        OpStepMem(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX), -1);
    } while (!cpu->negative);
    if (!ExpandSceneCircle(&frame))
        return frame.result;
    OpLoadA(cpu, 0xc0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLda(memory, cpu, OpAbs(cpu, FRAME_SCENE_COUNTER));
    OpLsrA(cpu);
    OpLoadA(cpu, 0x60u);
    if (cpu->carry)
        OpLoadA(cpu, 0x20u);
    OpOra(memory, cpu, OpAbs(cpu, FRAME_COLOR_PHASE));
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    return frame.result;
}

static bool AnimateSceneWindowRecords(WindowFrame *frame, bool circles) {
    const Lufia2Memory *memory = frame->memory;
    Lufia2CpuState *cpu = frame->cpu;
    OpLoadA(cpu, circles ? 2u : 5u);
    OpSta(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
    do {
        OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
        if (!CallWindowFrameChild(frame, circles ? 0x86b1feu : 0x86b1e4u,
                                  circles ? 0x86c321u : 0x86c536u))
            return false;
        OpLoadA(cpu, 0u);
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, WorldSceneObjectField(cpu, circles ? 4u : 12u));
        if (!cpu->zero) {
            OpAslA(cpu);
            OpTxy(cpu);
            OpTax(cpu);
            uint16_t vector = (uint16_t)((circles ? 0xb450u : 0xb448u) + cpu->x);
            uint32_t target = 0x860000u | Read16Bank(memory, 0x86u, vector);
            if (!CallWindowFrameChild(frame, circles ? 0x86b20bu : 0x86b1f1u, target))
                return false;
        }
        OpStepMem(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX), -1);
    } while (!cpu->negative);
    return true;
}

static void StepSceneWindowColor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, FRAME_COLOR_PHASE));
    OpIncA(cpu);
    OpCmpValue(cpu, 3u);
    if (cpu->carry)
        OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, FRAME_COLOR_PHASE));
    OpLda(memory, cpu, OpAbs(cpu, FRAME_COLOR_DIRECTION));
    if (cpu->zero)
        return;
    if (cpu->negative) {
        OpLda(memory, cpu, OpAbs(cpu, FRAME_COLOR_INTENSITY));
        OpDecA(cpu);
        if (cpu->negative) {
            OpStz(memory, cpu, OpAbs(cpu, FRAME_COLOR_DIRECTION));
            OpLoadA(cpu, 0u);
        }
    } else {
        OpLda(memory, cpu, OpAbs(cpu, FRAME_COLOR_INTENSITY));
        OpIncA(cpu);
        OpCmpValue(cpu, 12u);
        if (cpu->carry) {
            OpStz(memory, cpu, OpAbs(cpu, FRAME_COLOR_DIRECTION));
            OpLoadA(cpu, 12u);
        }
    }
    OpSta(memory, cpu, OpAbs(cpu, FRAME_COLOR_INTENSITY));
    OpOraValue(cpu, 0xc0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
}

Lufia2ExecutionResult
Lufia2WorldMapStepSceneWindowAnimations(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        Lufia2PushedChildCall child, void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b1deu);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b24bu)};
    if (!AnimateSceneWindowRecords(&frame, false) ||
        !AnimateSceneWindowRecords(&frame, true))
        return frame.result;
    StepSceneWindowColor(memory, cpu);
    if (!CallWindowFrameChild(&frame, 0x86b248u, 0x86b51cu))
        return frame.result;
    return frame.result;
}
