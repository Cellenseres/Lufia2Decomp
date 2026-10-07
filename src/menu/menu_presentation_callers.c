#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/menu.h"
#include "system/wram.h"

enum {
    MENU_FADE_CONTROL = WRAM_FADE_CONTROL,
    MENU_FADE_LEVEL = WRAM_FADE_LEVEL,
    MENU_FADE_IN = 0x90u,
    MENU_FADE_OUT = 0xd0u,
    MENU_DISPLAY_TEXT_SLOT = WRAM_NMI_SCROLL_REGISTERS + 2u,
    MENU_DISPLAY_REQUEST = WRAM_MENU_DISPLAY_REQUESTS,
    DISPLAY_TEXT_FLAGS = 0xf2u,
    DISPLAY_TEXT_MODE = 0xf3u,
    DISPLAY_TEXT_SOURCE = 0xf4u,
    DISPLAY_TEXT_BANK = 0xf6u,
    DISPLAY_TEXT_DESTINATION = 0xf7u,
    DISPLAY_TEXT_DESTINATION_BANK = 0xf9u,
    DISPLAY_TEXT_FIRST = 0xfau,
    DISPLAY_TEXT_SECOND = 0xfbu,
    ROM_DISPLAY_TEXT_SLOTS = 0x8ee57eu,
    ROM_DISPLAY_TEXT_SOURCES = 0x8ee5a0u
};

static Lufia2ExecutionResult MenuFadeDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint8_t request,
    uint32_t site, uint32_t finish) {
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, MENU_FADE_LEVEL));
    OpLoadA(cpu, request);
    OpSta(memory, cpu, OpAbs(cpu, MENU_FADE_CONTROL));
    do {
        if (!CallChildWithFrame(memory, cpu, child, context,
                site, 0x868b55u, 3u, 0x86u)) {
            Lufia2ExecutionResult result = ExecutionReturned(site);
            result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
            return result;
        }
        OpLda(memory, cpu, OpAbs(cpu, MENU_FADE_CONTROL));
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(finish);
}

Lufia2ExecutionResult Lufia2MenuFadeDisplayIn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868b1cu);
    return MenuFadeDisplay(memory, cpu, child, context,
        MENU_FADE_IN, 0x868b27u, 0x868b31u);
}

Lufia2ExecutionResult Lufia2MenuFadeDisplayOut(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x86u || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868b32u);
    return MenuFadeDisplay(memory, cpu, child, context,
        MENU_FADE_OUT, 0x868b3du, 0x868b47u);
}

Lufia2ExecutionResult Lufia2MenuPrepareDisplayText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    (void)child;
    (void)context;
    if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8293f6u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, OpA(cpu) & 0x00ffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT_SLOTS));
    OpSta(memory, cpu, OpAbs(cpu, MENU_DISPLAY_TEXT_SLOT));
    OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT_SOURCES));
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_TEXT_SOURCE));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_TEXT_BANK));
    OpLoadA(cpu, 0x10u);
    OpTestBits(memory, cpu, OpDp(cpu, DISPLAY_TEXT_FLAGS), 1u);
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_TEXT_MODE));
    OpLoadA(cpu, 0x12u);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_TEXT_FIRST));
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_TEXT_SECOND));
    OpLdx(cpu, 0x80c0u);
    OpWriteX(memory, cpu, OpDp(cpu, DISPLAY_TEXT_DESTINATION), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, DISPLAY_TEXT_DESTINATION_BANK));
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbs(cpu, MENU_DISPLAY_REQUEST));
    return ExecutionReturned(0x82942eu);
}
