#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/special_menu.h"
#include "system/wram.h"

enum {
    SPECIAL_MENU_SELECTOR = 0x30u,
    SPECIAL_MENU_TEXT_FLAGS = 0x71u,
    SPECIAL_MENU_EXIT_FLAGS = 0x72u,
    SPECIAL_MENU_TEXT_DELAY = 0x74u,
    SPECIAL_MENU_MULTIPLIER = 0x6008u,
    SPECIAL_MENU_MULTIPLICAND = 0x600au,
    SPECIAL_MENU_PRODUCT_LOW = 0x600cu,
    SPECIAL_MENU_PRODUCT_HIGH = 0x600eu,
    SPECIAL_MENU_FRAME_LIMIT = 65536u
};

static bool SpecialMenuReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

static bool SpecialMenuCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            Lufia2PushedChildCall child, void *context,
                            uint32_t site, uint32_t target,
                            Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, target, 2u, 0x84u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!SpecialMenuReady(cpu)) {
        *result = ExecutionHandoff(cpu, site + 3u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2SpecialMenuClearTilemaps(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SpecialMenuReady(cpu))
        return ExecutionHandoff(cpu, 0x84a4ddu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, 0x07feu);
    do {
        for (unsigned layer = 0u; layer < 4u; ++layer)
            OpStz(memory, cpu, OpAbsX(cpu, (uint16_t)(0x2000u + layer * 0x800u)));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84a4fcu);
}

Lufia2ExecutionResult Lufia2SpecialMenuHideSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SpecialMenuReady(cpu))
        return ExecutionHandoff(cpu, 0x84a4fdu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, 128u);
    OpLdy(cpu, 0u);
    do {
        OpLoadA(cpu, 0xf000u);
        OpSta(memory, cpu, OpAbsY(cpu, 0x0100u));
        OpIny(cpu);
        OpIny(cpu);
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, OpAbsY(cpu, 0x0100u));
        OpIny(cpu);
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, 16u);
    OpLoadA(cpu, 0u);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, 0x0100u));
        OpIny(cpu);
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84a528u);
}

Lufia2ExecutionResult Lufia2SpecialMenuMultiplyWords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x84a5dfu);
    PushIndex(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpStz(memory, cpu, OpAbs(cpu, SPECIAL_MENU_PRODUCT_LOW));
    OpStz(memory, cpu, OpAbs(cpu, SPECIAL_MENU_PRODUCT_HIGH));
    OpLdx(cpu, 16u);
    do {
        LsrAbsolute16(memory, cpu, SPECIAL_MENU_MULTIPLIER);
        if (cpu->carry) {
            OpLda(memory, cpu, OpAbs(cpu, SPECIAL_MENU_PRODUCT_HIGH));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpAbs(cpu, SPECIAL_MENU_MULTIPLICAND));
            OpSta(memory, cpu, OpAbs(cpu, SPECIAL_MENU_PRODUCT_HIGH));
        }
        RorAbsolute16(memory, cpu, SPECIAL_MENU_PRODUCT_HIGH);
        RorAbsolute16(memory, cpu, SPECIAL_MENU_PRODUCT_LOW);
        OpDex(cpu);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    return ExecutionReturned(0x84a606u);
}

static Lufia2ExecutionResult ContinueSpecialMenuFade(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry,
    bool completed_frame) {
    Lufia2ExecutionResult result = ExecutionReturned(entry + 20u);
    for (unsigned frame = 0u; frame < SPECIAL_MENU_FRAME_LIMIT; ++frame) {
        if (!completed_frame &&
            !SpecialMenuCall(memory, cpu, child, context, entry + 11u,
                             0x84a5a8u, &result))
            return result;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
        if (cpu->zero) {
            UnpackStatus(cpu, Pull8(memory, cpu));
            return result;
        }
        completed_frame = false;
    }
    return ExecutionHandoff(cpu, entry + 11u);
}

static Lufia2ExecutionResult FadeSpecialMenu(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t entry, uint8_t control) {
    if (!child || !SpecialMenuReady(cpu))
        return ExecutionHandoff(cpu, entry);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_FADE_LEVEL));
    OpLoadA(cpu, control);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FADE_CONTROL));
    return ContinueSpecialMenuFade(memory, cpu, child, context, entry, false);
}

Lufia2ExecutionResult Lufia2SpecialMenuResumeFade(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t landing) {
    if (!child || !SpecialMenuReady(cpu) ||
        (landing != 0x84a5c3u && landing != 0x84a5d8u))
        return ExecutionHandoff(cpu, landing);
    return ContinueSpecialMenuFade(memory, cpu, child, context, landing - 14u, true);
}

Lufia2ExecutionResult Lufia2SpecialMenuFadeIn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return FadeSpecialMenu(memory, cpu, child, context, 0x84a5b5u, 0x88u);
}

Lufia2ExecutionResult Lufia2SpecialMenuFadeOut(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return FadeSpecialMenu(memory, cpu, child, context, 0x84a5cau, 0xc8u);
}

Lufia2ExecutionResult Lufia2SpecialMenuRun(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SpecialMenuReady(cpu))
        return ExecutionHandoff(cpu, 0x84a689u);
    Lufia2ExecutionResult result = ExecutionReturned(0x84a704u);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, SPECIAL_MENU_TEXT_FLAGS));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
    if (!SpecialMenuCall(memory, cpu, child, context, 0x84a696u, 0x84a529u, &result) ||
        !SpecialMenuCall(memory, cpu, child, context, 0x84a699u, 0x84a455u, &result))
        return result;
    OpLda(memory, cpu, OpDp(cpu, SPECIAL_MENU_SELECTOR));
    unsigned selection;
    for (selection = 0u; selection < 7u; ++selection) {
        OpCmpValue(cpu, (uint16_t)(0xf0u + selection));
        if (cpu->zero)
            break;
    }
    if (selection == 2u) {
        OpStz(memory, cpu, OpDp(cpu, SPECIAL_MENU_SELECTOR));
        OpStepMem(memory, cpu, OpDp(cpu, SPECIAL_MENU_SELECTOR), 1);
    } else if (selection == 3u || selection >= 5u) {
        OpLoadA(cpu, selection == 5u ? 10u : selection == 6u ? 100u : 1u);
        OpSta(memory, cpu, OpDp(cpu, SPECIAL_MENU_SELECTOR));
    }
    static const uint16_t handlers[] = {
        0xa705u, 0xc826u, 0xda9bu, 0xf38au,
        0xb827u, 0xf38au, 0xf38au, 0xda9bu
    };
    static const uint16_t call_sites[] = {
        0xa6a2u, 0xa6abu, 0xa6b8u, 0xa6c5u,
        0xa6ceu, 0xa6dbu, 0xa6e8u, 0xa6f1u
    };
    const uint32_t site = 0x840000u | call_sites[selection];
    const uint32_t target = 0x840000u | handlers[selection];
    if (!SpecialMenuCall(memory, cpu, child, context, site, target, &result))
        return result;
    OpSepWidths(cpu, 0x20u);
    if (!SpecialMenuCall(memory, cpu, child, context, 0x84a6f6u, 0x84a4fdu, &result) ||
        !SpecialMenuCall(memory, cpu, child, context, 0x84a6f9u, 0x84a4ddu, &result))
        return result;
    OpLoadA(cpu, 0xa8u);
    OpSta(memory, cpu, OpDp(cpu, SPECIAL_MENU_TEXT_DELAY));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, SPECIAL_MENU_EXIT_FLAGS));
    return result;
}
