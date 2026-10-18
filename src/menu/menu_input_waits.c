#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    WAIT_BUTTON_HIGH = 0x80u,
    WAIT_FRAME_COUNT_MASK = 0x7fu,
    WAIT_FIRST_MASK = WRAM_UNK_7E156C,
    WAIT_SECOND_MASK = WRAM_UNK_7E156E,
    WAIT_SPRITE_MOTION = WRAM_MENU_DISPLAY_REQUESTS + 2u
};

typedef struct MenuInputWait {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} MenuInputWait;

static bool InputWaitReady(const Lufia2CpuState *cpu, Lufia2PushedChildCall child) {
    return child && cpu->program_bank == 0x82u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
           cpu->stack >= 0x1f20u && cpu->stack <= 0x1ffcu;
}

static bool InputWaitChild(MenuInputWait *wait, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = wait->cpu;
    if (!CallChildWithFrame(wait->memory, cpu, wait->child, wait->context, site, target,
                            3u, 0x82u)) {
        wait->result =
            (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return false;
    }
    if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal) {
        wait->result = ExecutionHandoff(cpu, site + 4u);
        return false;
    }
    return true;
}

static bool WaitButtonPressed(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_A));
    OpBitValue(cpu, WAIT_BUTTON_HIGH);
    if (!cpu->zero)
        return true;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
    OpBitValue(cpu, WAIT_BUTTON_HIGH);
    return !cpu->zero;
}

Lufia2ExecutionResult Lufia2MenuWaitFramesOrButton(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!InputWaitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x829330u);
    OpAndValue(cpu, WAIT_FRAME_COUNT_MASK);
    if (cpu->zero)
        return ExecutionReturned(0x829334u);
    MenuInputWait wait = {memory, cpu, child, context, {0}};
    do {
        PushAccumulator8(memory, cpu);
        if (!InputWaitChild(&wait, 0x829336u, 0x828b4bu))
            return wait.result;
        if (WaitButtonPressed(memory, cpu)) {
            OpLoadA(cpu, Pull8(memory, cpu));
            return ExecutionReturned(0x829352u);
        }
        if (!InputWaitChild(&wait, 0x829348u, 0x868b55u))
            return wait.result;
        OpLoadA(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x829350u);
}

Lufia2ExecutionResult Lufia2MenuWaitSpriteFrames(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (!InputWaitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x829353u);
    MenuInputWait wait = {memory, cpu, child, context, {0}};
    do {
        PushAccumulator8(memory, cpu);
        if (!InputWaitChild(&wait, 0x829354u, 0x868b55u))
            return wait.result;
        OpLoadA(cpu, Pull8(memory, cpu));
        OpDecA(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x82935cu);
}

Lufia2ExecutionResult Lufia2MenuWaitButtonMasks(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context) {
    if (!InputWaitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x82935du);
    MenuInputWait wait = {memory, cpu, child, context, {0}};
    for (;;) {
        if (!InputWaitChild(&wait, 0x82935du, 0x828b4bu))
            return wait.result;
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_A));
        OpBit(memory, cpu, OpAbs(cpu, WAIT_FIRST_MASK));
        if (!cpu->zero) {
            OpSepWidths(cpu, 0x20u);
            cpu->carry = 0u;
            return ExecutionReturned(0x82937bu);
        }
        OpBit(memory, cpu, OpAbs(cpu, WAIT_SECOND_MASK));
        if (!cpu->zero) {
            OpSepWidths(cpu, 0x20u);
            cpu->carry = 1u;
            return ExecutionReturned(0x82937fu);
        }
        OpSepWidths(cpu, 0x20u);
        if (!InputWaitChild(&wait, 0x829372u, 0x868b55u))
            return wait.result;
    }
}

Lufia2ExecutionResult Lufia2MenuWaitSpriteMotionOrButton(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!InputWaitReady(cpu, child))
        return ExecutionHandoff(cpu, 0x829380u);
    MenuInputWait wait = {memory, cpu, child, context, {0}};
    do {
        if (!InputWaitChild(&wait, 0x829380u, 0x828b4bu))
            return wait.result;
        if (WaitButtonPressed(memory, cpu))
            break;
        if (!InputWaitChild(&wait, 0x829392u, 0x868b55u))
            return wait.result;
        OpLda(memory, cpu, OpAbs(cpu, WAIT_SPRITE_MOTION));
    } while (!cpu->zero);
    return ExecutionReturned(0x82939bu);
}
