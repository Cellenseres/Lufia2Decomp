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

enum {
    ANIMATION_PROGRESS = 6u,
    ANIMATION_DELAY = 8u,
    ANIMATION_HEIGHT = 10u,
    ANIMATION_HANDLER = 12u,
    ANIMATION_AGE = 13u,
    ANIMATION_HOLD = 14u,
    ANIMATION_SPARK_EMISSION = WRAM_FIELD_LAYER_SCROLL_Y + 6u,
    ANIMATION_COLOR_VALUE = WRAM_FIELD_LAYER_SCROLL_Y + 4u,
    ANIMATION_COLOR_TARGET = WRAM_FIELD_LAYER_SCROLL_Y + 5u,
    ANIMATION_FRAME_PHASE = WRAM_FIELD_LAYER_SCROLL_Y + 2u,
    ANIMATION_LIMIT = ANIMATION_AGE,
    ANIMATION_STEP = 0u
};

Lufia2ExecutionResult Lufia2WorldMapGrowSceneWindow(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b454u);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b47du)};
    OpTyx(cpu);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_PROGRESS));
    cpu->carry = 0u;
    OpAdcValue(cpu, 10u);
    if (cpu->carry) {
        OpStz(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HANDLER));
        return ExecutionReturned(0x86b45eu);
    }
    OpSta(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_PROGRESS));
    OpLoadA(cpu, 0xffu);
    cpu->carry = 1u;
    OpSbcValue(cpu,
               OpReadM(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_PROGRESS)));
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpLsrA(cpu);
    OpCmpValue(cpu, 20u);
    if (cpu->carry)
        OpLoadA(cpu, 20u);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0xc0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
    OpStz(memory, cpu, OpDp(cpu, 0u));
    if (!CallWindowFrameChild(&frame, 0x86b47au, 0x86c4e0u))
        return frame.result;
    return frame.result;
}

static void StepRotatingWindowColor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStepMem(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_AGE), 1);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_AGE));
    OpCmpValue(cpu, 12u);
    if (cpu->carry) {
        OpCmpValue(cpu, 64u);
        if (!cpu->carry) {
            OpLoadA(cpu, 12u);
        } else {
            OpSbcValue(cpu, 64u);
            OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
            OpIncA(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 12u);
            OpCmpValue(cpu, 13u);
            if (cpu->carry) {
                OpStz(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HANDLER));
                OpStz(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HEIGHT));
                OpLoadA(cpu, 0u);
            }
        }
    }
    cpu->carry = 0u;
    OpAdcValue(cpu, 0xc0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
}

Lufia2ExecutionResult Lufia2WorldMapRotateSceneWindow(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b47eu);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b4bdu)};
    OpTyx(cpu);
    OpStepMem(memory, cpu, WorldSceneObjectField(cpu, 4u), 1);
    OpStepMem(memory, cpu, WorldSceneObjectField(cpu, 4u), 1);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, 5u));
    OpIncA(cpu);
    OpCmpValue(cpu, 32u);
    if (cpu->carry)
        OpLoadA(cpu, 32u);
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 5u));
    StepRotatingWindowColor(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    if (!CallWindowFrameChild(&frame, 0x86b4bau, 0x86c4e0u))
        return frame.result;
    return frame.result;
}

Lufia2ExecutionResult Lufia2WorldMapContractSceneWindow(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b4beu);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b4f1u)};
    OpTyx(cpu);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_LIMIT));
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, ANIMATION_STEP));
    OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_DELAY));
    bool contracting = !cpu->zero;
    if (!contracting) {
        OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HEIGHT));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, ANIMATION_STEP));
        OpCmp(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_LIMIT));
        contracting = cpu->carry;
        if (contracting)
            OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_DELAY));
    }
    if (contracting) {
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, ANIMATION_STEP));
        OpCmp(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_LIMIT));
        if (cpu->carry) {
            OpStz(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HANDLER));
            OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_LIMIT));
        }
        OpSta(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_DELAY));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpIncA(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_LIMIT));
    }
    OpSta(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HEIGHT));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    if (!CallWindowFrameChild(&frame, 0x86b4eeu, 0x86c4e0u))
        return frame.result;
    return frame.result;
}

Lufia2ExecutionResult Lufia2WorldMapSubmitPhasedSceneWindow(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            Lufia2PushedChildCall child,
                                                            void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b4f2u);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b507u)};
    OpTyx(cpu);
    OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
    OpCmpValue(cpu, 3u);
    if (cpu->carry)
        OpSbcValue(cpu, 3u);
    OpCmp(memory, cpu, OpAbs(cpu, ANIMATION_FRAME_PHASE));
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        if (!CallWindowFrameChild(&frame, 0x86b504u, 0x86c4e0u))
            return frame.result;
    }
    return frame.result;
}

Lufia2ExecutionResult Lufia2WorldMapContractSceneCircle(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b508u);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b51bu)};
    OpTyx(cpu);
    OpLda(memory, cpu, WorldSceneObjectField(cpu, 2u));
    OpDecA(cpu);
    OpCmp(memory, cpu, WorldSceneObjectField(cpu, 5u));
    if (!cpu->carry)
        OpLda(memory, cpu, WorldSceneObjectField(cpu, 5u));
    OpSta(memory, cpu, WorldSceneObjectField(cpu, 2u));
    OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
    OpStz(memory, cpu, OpDp(cpu, 0u));
    if (!CallWindowFrameChild(&frame, 0x86b518u, 0x86c2d9u))
        return frame.result;
    return frame.result;
}

static void RestartWindowColorPulse(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, ANIMATION_COLOR_TARGET));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, ANIMATION_COLOR_VALUE));
}

Lufia2ExecutionResult Lufia2WorldMapBeginWindowColorPulse(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b7d3u);
    OpLoadA(cpu, 10u);
    RestartWindowColorPulse(memory, cpu);
    return ExecutionReturned(0x86b7deu);
}

Lufia2ExecutionResult Lufia2WorldMapRestartWindowColorPulse(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b7d5u);
    RestartWindowColorPulse(memory, cpu);
    return ExecutionReturned(0x86b7deu);
}

Lufia2ExecutionResult Lufia2WorldMapStopWindowSparkEmission(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu) {
    if (!WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b7cfu);
    OpStz(memory, cpu, OpAbs(cpu, ANIMATION_SPARK_EMISSION));
    return ExecutionReturned(0x86b7d2u);
}

static bool StepLayeredWindowColor(WindowFrame *frame) {
    const Lufia2Memory *memory = frame->memory;
    Lufia2CpuState *cpu = frame->cpu;
    OpLda(memory, cpu, OpAbs(cpu, ANIMATION_COLOR_TARGET));
    if (cpu->zero)
        return true;
    OpCmp(memory, cpu, OpAbs(cpu, ANIMATION_COLOR_VALUE));
    if (cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLda(memory, cpu, OpAbs(cpu, ANIMATION_FRAME_PHASE));
        OpLsrA(cpu);
        if (cpu->carry)
            OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
        OpLda(memory, cpu, OpDp(cpu, 0u));
        if (cpu->zero) {
            OpStz(memory, cpu, OpAbs(cpu, ANIMATION_COLOR_TARGET));
            return CallWindowFrameChild(frame, 0x86b763u, 0x86c582u);
        }
        return CallWindowFrameChild(frame, 0x86b768u, 0x86b7d5u);
    }
    OpStepMem(memory, cpu, OpAbs(cpu, ANIMATION_COLOR_VALUE), 1);
    OpLda(memory, cpu, OpAbs(cpu, ANIMATION_COLOR_VALUE));
    if (cpu->negative) {
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
        OpIncA(cpu);
    }
    OpOraValue(cpu, 0xe0u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    return true;
}

static void StepLayeredWindowShape(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HEIGHT));
    cpu->carry = 0u;
    OpAdcValue(cpu, 12u);
    OpCmp(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_LIMIT));
    if (cpu->carry) {
        OpStepMem(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HOLD), 1);
        OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HOLD));
        OpCmpValue(cpu, 24u);
        if (cpu->carry) {
            OpLda(memory, cpu, WorldSceneObjectField(cpu, 5u));
            OpDecA(cpu);
            OpCmpValue(cpu, 1u);
            if (!cpu->carry)
                OpLoadA(cpu, 1u);
            OpSta(memory, cpu, WorldSceneObjectField(cpu, 5u));
            OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HOLD));
            OpCmpValue(cpu, 28u);
            if (cpu->carry) {
                OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_DELAY));
                cpu->carry = 0u;
                OpAdcValue(cpu, 10u);
                OpSta(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_DELAY));
                OpCmp(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_LIMIT));
                if (cpu->carry)
                    OpStz(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HANDLER));
            }
        }
        OpLoadA(cpu, 200u);
    }
    OpSta(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HEIGHT));
}

Lufia2ExecutionResult Lufia2WorldMapStepLayeredSceneWindows(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            Lufia2PushedChildCall child,
                                                            void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b748u);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b7ceu)};
    if (!StepLayeredWindowColor(&frame))
        return frame.result;
    OpStepMem(memory, cpu, OpAbs(cpu, ANIMATION_FRAME_PHASE), 1);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
    do {
        OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
        if (!CallWindowFrameChild(&frame, 0x86b786u, 0x86c536u))
            return frame.result;
        OpLda(memory, cpu, WorldSceneObjectField(cpu, ANIMATION_HANDLER));
        if (!cpu->zero) {
            StepLayeredWindowShape(memory, cpu);
            OpLda(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
            OpSta(memory, cpu, OpDp(cpu, 0u));
            if (!CallWindowFrameChild(&frame, 0x86b7c4u, 0x86c4e0u))
                return frame.result;
        }
        OpStepMem(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX), -1);
    } while (!cpu->negative);
    if (!CallWindowFrameChild(&frame, 0x86b7cbu, 0x86b51cu))
        return frame.result;
    return frame.result;
}

static Lufia2ExecutionResult LoadLayeredSceneWindow(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context, unsigned index) {
    static const uint16_t entries[] = {0xb7dfu, 0xb7f7u, 0xb7feu};
    static const uint16_t records[] = {0xb805u, 0xb813u, 0xb821u};
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x860000u | entries[index]);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b7f6u)};
    OpLdy(cpu, records[index]);
    OpLoadA(cpu, (uint16_t)index);
    if (!CallWindowFrameChild(&frame, 0x86b7e4u, 0x86c54bu))
        return frame.result;
    OpLoadA(cpu, 0x31u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGADSUB));
    OpLoadA(cpu, 0x6fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_COLDATA));
    OpLoadA(cpu, 0x8eu);
    if (!CallWindowFrameChild(&frame, 0x86b7f3u, 0x86d378u))
        return frame.result;
    return frame.result;
}

Lufia2ExecutionResult Lufia2WorldMapLoadLayeredSceneWindow0(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            Lufia2PushedChildCall child,
                                                            void *context) {
    return LoadLayeredSceneWindow(memory, cpu, child, context, 0u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadLayeredSceneWindow1(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            Lufia2PushedChildCall child,
                                                            void *context) {
    return LoadLayeredSceneWindow(memory, cpu, child, context, 1u);
}

Lufia2ExecutionResult Lufia2WorldMapLoadLayeredSceneWindow2(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            Lufia2PushedChildCall child,
                                                            void *context) {
    return LoadLayeredSceneWindow(memory, cpu, child, context, 2u);
}

enum {
    ANIMATION_PALETTE_INPUT = WRAM_CGRAM_BUFFER + 0x20u,
    ANIMATION_PALETTE_WORK = 0xdc20u,
    ANIMATION_PALETTE_BYTES = 0x1e0u,
    ANIMATION_PALETTE_TRANSFER = 0xdc00u,
    ANIMATION_PALETTE_STEP = 0x15u,
    ANIMATION_COLORS_LEFT = 0x26u
};

static void RestoreScenePaletteWork(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, ANIMATION_PALETTE_WORK);
    OpLdy(cpu, ANIMATION_PALETTE_INPUT);
    OpLoadA(cpu, ANIMATION_PALETTE_BYTES - 1u);
    PushDataBank(memory, cpu);
    OpMoveNext(memory, cpu, 0x7eu, 0x7fu);
    PullDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, ANIMATION_PALETTE_TRANSFER);
    OpWrite16(memory, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_SOURCE), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_BANK));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_MODE));
    OpLdx(cpu, ANIMATION_PALETTE_BYTES);
    OpWrite16(memory, OpAbs(cpu, WRAM_WORLD_MAP_PALETTE_TRANSFER_LENGTH), cpu->x);
}

Lufia2ExecutionResult Lufia2WorldMapCycleScenePaletteFade(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!child || !WorldSceneResourceEntry(cpu))
        return ExecutionHandoff(cpu, 0x86b350u);
    WindowFrame frame = {memory, cpu, child, context, ExecutionReturned(0x86b39bu)};
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX));
    do {
        OpLdx(cpu, 0x421u);
        OpWrite16(memory, OpDp(cpu, ANIMATION_PALETTE_STEP), cpu->x);
        OpLdx(cpu, ANIMATION_PALETTE_INPUT);
        OpLoadA(cpu, 0xf0u);
        OpSta(memory, cpu, OpDp(cpu, ANIMATION_COLORS_LEFT));
        if (!CallWindowFrameChild(&frame, 0x86b360u, 0x86966cu))
            return frame.result;
        RestoreScenePaletteWork(memory, cpu);
        for (unsigned wait = 0u; wait < 4u; ++wait)
            if (!CallWindowFrameChild(&frame, 0x86b38bu + wait * 3u, 0x86974fu))
                return frame.result;
        OpStepMem(memory, cpu, OpDp(cpu, FRAME_WINDOW_INDEX), -1);
    } while (!cpu->zero);
    return frame.result;
}
