#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"
#include <stddef.h>

enum {
    SELECT_TRANSITION_FRAMES = 4096u,
    SELECT_OBJECT_UPLOAD = 0x72u,
    SELECT_RECORD_SCRIPT = 11u
};

static uint8_t SelectTransitionContext(const Lufia2CpuState *cpu,
                                       uint8_t either_accumulator_width) {
    return cpu->program_bank == 0x86u && cpu->data_bank == 0x7eu && !cpu->direct_page &&
           !cpu->decimal && !cpu->index_is_8_bit &&
           (either_accumulator_width || cpu->accumulator_is_8_bit) &&
           cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static uint8_t SelectTransitionCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    Lufia2PushedChildCall child, void *context,
                                    uint32_t site, uint32_t target,
                                    uint8_t frame_bytes) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, frame_bytes,
                              0x86u);
}

static Lufia2ExecutionResult SelectTransitionUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static bool SelectResumeCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             Lufia2PushedChildCall child, void *context, uint32_t site,
                             uint32_t target, uint8_t frame,
                             Lufia2ExecutionResult *result) {
    if (!child) {
        *result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!SelectTransitionCall(memory, cpu, child, context, site, target, frame)) {
        *result = SelectTransitionUnwound(site);
        return false;
    }
    if (cpu->program_bank != 0x86u || cpu->data_bank != 0x7eu ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->direct_page ||
        cpu->decimal) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static Lufia2ExecutionResult ContinueSelectFade(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context, bool resume) {
    static const struct {
        uint32_t site, target;
        uint8_t frame;
    } updates[] = {{0x86831fu, 0x86838cu, 2u},
                   {0x868322u, 0x868996u, 2u},
                   {0x868325u, 0x868bcfu, 3u},
                   {0x868329u, 0x8688beu, 2u}};
    if (resume) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
        if (cpu->zero)
            return ExecutionReturned(0x868338u);
    }
    Lufia2ExecutionResult result;
    for (unsigned frame = 0u; frame < SELECT_TRANSITION_FRAMES; ++frame) {
        if (!SelectResumeCall(memory, cpu, child, context, 0x868317u, 0x8686edu, 2u,
                              &result))
            return result;
        if (!SelectResumeCall(memory, cpu, child, context, 0x86831au, 0x868637u, 2u,
                              &result))
            return result;
        if (!cpu->carry) {
            for (unsigned update = 0u; update < 4u; ++update) {
                if (!SelectResumeCall(memory, cpu, child, context, updates[update].site,
                                      updates[update].target, updates[update].frame,
                                      &result))
                    return result;
            }
            OpLoadA(cpu, 0x80u);
            OpSta(memory, cpu, OpDp(cpu, SELECT_OBJECT_UPLOAD));
        }
        if (!SelectResumeCall(memory, cpu, child, context, 0x868330u, 0x868b48u, 2u,
                              &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
        if (cpu->zero)
            return ExecutionReturned(0x868338u);
    }
    return ExecutionHandoff(cpu, 0x868317u);
}

static Lufia2ExecutionResult SelectScreenFade(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              void *context, uint8_t request) {
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, request);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    return ContinueSelectFade(memory, cpu, child, context, false);
}

Lufia2ExecutionResult Lufia2SelectScreenFadeIn(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *context) {
    if (!child || !SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x868305u);
    return SelectScreenFade(memory, cpu, child, context, 0x84u);
}

Lufia2ExecutionResult Lufia2SelectScreenFadeOut(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context) {
    if (!child || !SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x86830fu);
    return SelectScreenFade(memory, cpu, child, context, 0xc4u);
}

static Lufia2ExecutionResult ContinueSelectInput(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context, bool resume) {
    if (resume) {
        OpPullX(memory, cpu);
        OpDex(cpu);
        if (cpu->zero) {
            cpu->carry = 0u;
            return ExecutionReturned(0x868357u);
        }
    }
    Lufia2ExecutionResult result;
    for (unsigned frame = 0u; frame < SELECT_TRANSITION_FRAMES; ++frame) {
        OpPushX(memory, cpu);
        if (!SelectResumeCall(memory, cpu, child, context, 0x86833au, 0x828b4bu, 3u,
                              &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_A));
        OpBitValue(cpu, 0x80u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
            OpBitValue(cpu, 0x90u);
        }
        if (!cpu->zero) {
            OpPullX(memory, cpu);
            cpu->carry = 1u;
            return ExecutionReturned(0x86835au);
        }
        if (!SelectResumeCall(memory, cpu, child, context, 0x86834cu, 0x8686edu, 2u,
                              &result))
            return result;
        if (!SelectResumeCall(memory, cpu, child, context, 0x86834fu, 0x868b48u, 2u,
                              &result))
            return result;
        OpPullX(memory, cpu);
        OpDex(cpu);
        if (cpu->zero) {
            cpu->carry = 0u;
            return ExecutionReturned(0x868357u);
        }
    }
    return ExecutionHandoff(cpu, 0x868339u);
}

Lufia2ExecutionResult Lufia2SelectScreenWaitForInput(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    if (!child || !SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x868339u);
    return ContinueSelectInput(memory, cpu, child, context, false);
}

Lufia2ExecutionResult Lufia2SelectScreenConfigureObject(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !SelectTransitionContext(cpu, 1u))
        return ExecutionHandoff(cpu, 0x86835bu);
    OpRepWidths(cpu, 0x20u);
    OpPushX(memory, cpu);
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x868a41u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TITLE_OBJECT_RECORD_POINTER));
    OpLda(memory, cpu, OpLongX(cpu, 0x868a47u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TITLE_SPRITE_RECORD_POINTER));
    OpLda(memory, cpu, OpLongX(cpu, 0x868a4du));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TITLE_PARTICLE_POOL_POINTER));
    OpTyx(cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_TITLE_SPRITE_RECORD_POINTER)));
    OpLda(memory, cpu, OpLongX(cpu, 0x868a53u));
    OpSta(memory, cpu, OpAbsY(cpu, SELECT_RECORD_SCRIPT));
    OpSepWidths(cpu, 0x20u);
    OpPullX(memory, cpu);
    if (!SelectTransitionCall(memory, cpu, child, context, 0x868385u, 0x868655u, 2u))
        return SelectTransitionUnwound(0x868385u);
    if (!SelectTransitionCall(memory, cpu, child, context, 0x868388u, 0x86866au, 2u))
        return SelectTransitionUnwound(0x868388u);
    return ExecutionReturned(0x86838bu);
}

static Lufia2ExecutionResult ContinueSelectObjects(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    static const struct {
        uint32_t site, target;
        uint8_t frame;
    } updates[] = {{0x8682e5u, 0x8686edu, 2u},
                   {0x8682e8u, 0x86838cu, 2u},
                   {0x8682ebu, 0x868996u, 2u},
                   {0x8682eeu, 0x868bcfu, 3u},
                   {0x8682f2u, 0x8688beu, 2u}};
    Lufia2ExecutionResult result;
    for (unsigned frame = 0u; frame < SELECT_TRANSITION_FRAMES; ++frame) {
        if (!SelectResumeCall(memory, cpu, child, context, 0x8682ceu, 0x828b4bu, 3u,
                              &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_A));
        OpBitValue(cpu, 0x80u);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
            OpBitValue(cpu, 0x90u);
        }
        if (!cpu->zero) {
            cpu->carry = 1u;
            return ExecutionReturned(0x868304u);
        }
        if (!SelectResumeCall(memory, cpu, child, context, 0x8682e0u, 0x868637u, 2u,
                              &result))
            return result;
        if (cpu->carry) {
            if (!SelectResumeCall(memory, cpu, child, context, 0x8682feu, 0x8689fdu, 2u,
                                  &result))
                return result;
            cpu->carry = 0u;
            return ExecutionReturned(0x868302u);
        }
        for (unsigned update = 0u; update < 5u; ++update) {
            if (!SelectResumeCall(memory, cpu, child, context, updates[update].site,
                                  updates[update].target, updates[update].frame,
                                  &result))
                return result;
        }
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpDp(cpu, SELECT_OBJECT_UPLOAD));
        if (!SelectResumeCall(memory, cpu, child, context, 0x8682f9u, 0x868b48u, 2u,
                              &result))
            return result;
    }
    return ExecutionHandoff(cpu, 0x8682ceu);
}
Lufia2ExecutionResult Lufia2SelectScreenWaitForObjects(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!child || !SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x8682c8u);
    OpLoadA(cpu, 0x26u);
    if (!SelectTransitionCall(memory, cpu, child, context, 0x8682cau, 0x80953bu, 3u))
        return SelectTransitionUnwound(0x8682cau);
    if (!SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x8682ceu);
    return ContinueSelectObjects(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2SelectScreenResumeFade(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!child || !SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x868333u);
    return ContinueSelectFade(memory, cpu, child, context, true);
}

Lufia2ExecutionResult Lufia2SelectScreenResumeInputWait(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x868352u);
    return ContinueSelectInput(memory, cpu, child, context, true);
}

Lufia2ExecutionResult Lufia2SelectScreenResumeObjectWait(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!child || !SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x8682ceu);
    return ContinueSelectObjects(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2SelectScreenRepeatObjectWait(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!child || !SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x8682fcu);
    return ContinueSelectObjects(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2SelectScreenStepResumption(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       uint32_t entry) {
    if (!SelectTransitionContext(cpu, 0u))
        return ExecutionHandoff(cpu, entry);
    Lufia2ExecutionResult result;
    switch (entry) {
    case 0x868333u:
        result = ContinueSelectFade(memory, cpu, NULL, NULL, true);
        break;
    case 0x868352u:
        result = ContinueSelectInput(memory, cpu, NULL, NULL, true);
        break;
    case 0x8682ceu:
    case 0x8682fcu:
        result = ContinueSelectObjects(memory, cpu, NULL, NULL);
        break;
    default:
        return ExecutionHandoff(cpu, entry);
    }
    return ExecutionHandoff(cpu, result.pc);
}
