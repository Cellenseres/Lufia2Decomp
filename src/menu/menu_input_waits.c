#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "lufia2/menu_wait.h"
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

typedef enum WaitButtonChoice {
    WAIT_NO_BUTTON,
    WAIT_FIRST_BUTTON,
    WAIT_SECOND_BUTTON
} WaitButtonChoice;

static bool AdvanceWaitCountdown(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, Pull8(memory, cpu));
    OpDecA(cpu);
    return !cpu->zero;
}

static bool WaitSpriteMoving(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WAIT_SPRITE_MOTION));
    return !cpu->zero;
}

static WaitButtonChoice ReadWaitButtonChoice(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_A));
    OpBit(memory, cpu, OpAbs(cpu, WAIT_FIRST_MASK));
    if (!cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        cpu->carry = 0u;
        return WAIT_FIRST_BUTTON;
    }
    OpBit(memory, cpu, OpAbs(cpu, WAIT_SECOND_MASK));
    if (!cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        cpu->carry = 1u;
        return WAIT_SECOND_BUTTON;
    }
    OpSepWidths(cpu, 0x20u);
    return WAIT_NO_BUTTON;
}

Lufia2ExecutionResult Lufia2MenuPollInputWait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2MenuWaitPoll poll) {
    static const uint32_t entries[] = {
        0x82933au, 0x829384u, 0x829361u, 0x82934cu, 0x829358u, 0x829396u};
    if ((unsigned)poll >= sizeof(entries) / sizeof(entries[0]))
        return ExecutionHandoff(cpu, 0u);
    const uint32_t entry = entries[poll];
    if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f20u || cpu->stack > 0x1ffbu)
        return ExecutionHandoff(cpu, entry);
    if (poll == LUFIA2_MENU_WAIT_FRAME_BUTTONS)
        return ExecutionReturned(WaitButtonPressed(memory, cpu) ? 0x829351u
                                                               : 0x829348u);
    if (poll == LUFIA2_MENU_WAIT_MOTION_BUTTONS)
        return ExecutionReturned(WaitButtonPressed(memory, cpu) ? 0x82939bu
                                                               : 0x829392u);
    if (poll == LUFIA2_MENU_WAIT_BUTTON_MASKS) {
        const WaitButtonChoice choice = ReadWaitButtonChoice(memory, cpu);
        return ExecutionReturned(choice == WAIT_FIRST_BUTTON ? 0x82937bu
            : choice == WAIT_SECOND_BUTTON ? 0x82937fu : 0x829372u);
    }
    if (poll == LUFIA2_MENU_WAIT_FRAME_COUNT)
        return ExecutionReturned(AdvanceWaitCountdown(memory, cpu) ? 0x829335u
                                                                  : 0x829350u);
    if (poll == LUFIA2_MENU_WAIT_SPRITE_COUNT)
        return ExecutionReturned(AdvanceWaitCountdown(memory, cpu) ? 0x829353u
                                                                  : 0x82935cu);
    return ExecutionReturned(WaitSpriteMoving(memory, cpu) ? 0x829380u : 0x82939bu);
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
    } while (AdvanceWaitCountdown(memory, cpu));
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
    } while (AdvanceWaitCountdown(memory, cpu));
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
        const WaitButtonChoice choice = ReadWaitButtonChoice(memory, cpu);
        if (choice != WAIT_NO_BUTTON)
            return ExecutionReturned(choice == WAIT_FIRST_BUTTON ? 0x82937bu
                                                                 : 0x82937fu);
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
    } while (WaitSpriteMoving(memory, cpu));
    return ExecutionReturned(0x82939bu);
}
