#include "lufia2/menu_options_screen.h"
#include "menu/menu_screen_child_internal.h"
#include "system/wram.h"

enum {
    OPTION_TEXT_BANK = 0x5fu,
    OPTION_FIRST_INDICATOR = 0u,
    OPTION_REFRESH_REQUEST = 0x74u,
    OPTION_MESSAGE_SPEED = WRAM_UNK_7E0B52,
    OPTION_SECOND_SETTING = WRAM_UNK_7E0B53,
    OPTION_SOUND_SETTING = WRAM_UNK_7E0B54
};

static bool OptionsChild(MenuActionCall *call, uint32_t site, uint32_t target,
                          uint8_t frame, uint8_t width) {
    return MenuScreenChild(call, site, target, frame, width, 0u);
}

static uint32_t DispatchOption(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint32_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    return Lufia2MenuDispatchInlineTable(memory, cpu).pc;
}

Lufia2ExecutionResult Lufia2MenuSetOptionIndicators(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x82a0c7u);
    Push8(memory, cpu, A8(cpu));
    OpLoadA(cpu, 1u);
    for (unsigned indicator = 0u; indicator < 3u; ++indicator)
        OpSta(memory, cpu, OpDp(cpu, OPTION_FIRST_INDICATOR + indicator));
    OpLoadA(cpu, Pull8(memory, cpu));
    const uint32_t previous_resume = cpu->resume_pc;
    const uint32_t target = DispatchOption(memory, cpu, 0x82a0d1u);
    uint16_t indicator;
    uint32_t finish;
    if (target == 0x82a0dau) {
        indicator = 0u;
        finish = 0x82a0dcu;
    } else if (target == 0x82a0ddu) {
        indicator = 1u;
        finish = 0x82a0dfu;
    } else if (target == 0x82a0e0u) {
        indicator = 2u;
        finish = 0x82a0e2u;
    } else {
        return ExecutionHandoff(cpu, target);
    }
    cpu->resume_pc = previous_resume;
    OpStz(memory, cpu, OpDp(cpu, indicator));
    return ExecutionReturned(finish);
}

Lufia2ExecutionResult Lufia2MenuRefreshOptions(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82a07du);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    static const uint16_t settings[] = {OPTION_MESSAGE_SPEED,
        OPTION_SECOND_SETTING, WRAM_MENU_RESTORE_SELECTION_ENABLED,
        OPTION_SOUND_SETTING};
    static const uint16_t strings[] = {0xc93cu, 0xc95du, 0xc98cu, 0xc975u};
    static const uint32_t indicator_sites[] = {
        0x82a085u, 0x82a096u, 0x82a0a7u, 0x82a0b8u};
    static const uint32_t text_sites[] = {
        0x82a08fu, 0x82a0a0u, 0x82a0b1u, 0x82a0c2u};
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    for (unsigned row = 0u; row < 4u; ++row) {
        OpLda(memory, cpu, OpAbs(cpu, settings[row]));
        if (!OptionsChild(&call, indicator_sites[row], 0x82a0c7u, 2u, 1u))
            return call.result;
        OpLoadA(cpu, 0x8eu);
        OpSta(memory, cpu, OpDp(cpu, OPTION_TEXT_BANK));
        OpLdy(cpu, strings[row]);
        if (!OptionsChild(&call, text_sites[row], 0x808878u, 3u, 1u))
            return call.result;
    }
    return ExecutionReturned(0x82a0c6u);
}

Lufia2ExecutionResult Lufia2MenuBuildOptionsScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82a023u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_WINDOW_REFRESH));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_ENABLED));
    OpLdx(cpu, 0xffffu);
    if (!OptionsChild(&call, 0x82a02cu, 0x829214u, 3u, 1u) ||
        !OptionsChild(&call, 0x82a030u, 0x82838fu, 2u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0082u);
    OpLdx(cpu, 0x0a03u);
    if (!OptionsChild(&call, 0x82a03bu, 0x82810eu, 2u, 0u))
        return call.result;
    OpLoadA(cpu, 0x0142u);
    OpLdx(cpu, 0x1e0eu);
    if (!OptionsChild(&call, 0x82a044u, 0x82810eu, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, OPTION_TEXT_BANK));
    OpLdy(cpu, 0xd312u);
    if (!OptionsChild(&call, 0x82a055u, 0x808878u, 3u, 1u))
        return call.result;
    OpLdy(cpu, 0xc8fau);
    if (!OptionsChild(&call, 0x82a05cu, 0x808878u, 3u, 1u) ||
        !OptionsChild(&call, 0x82a060u, 0x82a07du, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 6u);
    if (!OptionsChild(&call, 0x82a068u, 0x82895bu, 2u, 1u))
        return call.result;
    OpLdy(cpu, 0x38u);
    OpLdx(cpu, 1u);
    if (!OptionsChild(&call, 0x82a071u, 0x82891eu, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 1u);
    if (!OptionsChild(&call, 0x82a076u, 0x8293f6u, 2u, 1u) ||
        !OptionsChild(&call, 0x82a079u, 0x828704u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82a07cu);
}

static bool ChangeOption(MenuActionCall *call, uint32_t target) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    if (target == 0x82bf79u || target == 0x82bf88u) {
        OpLda(memory, cpu, OpAbs(cpu, OPTION_MESSAGE_SPEED));
        if (target == 0x82bf79u) {
            if (cpu->zero)
                OpLoadA(cpu, 2u);
            else
                OpDecA(cpu);
        } else {
            OpCmpValue(cpu, 2u);
            if (cpu->zero)
                OpLoadA(cpu, 0u);
            else
                OpIncA(cpu);
        }
        OpSta(memory, cpu, OpAbs(cpu, OPTION_MESSAGE_SPEED));
    } else {
        const uint16_t setting = target == 0x82bf99u ? OPTION_SECOND_SETTING :
            target == 0x82bfa3u ? WRAM_MENU_RESTORE_SELECTION_ENABLED :
            OPTION_SOUND_SETTING;
        OpLda(memory, cpu, OpAbs(cpu, setting));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
        OpSta(memory, cpu, OpAbs(cpu, setting));
        if (target == 0x82bfadu &&
            !OptionsChild(call, 0x82bfb5u, 0x809623u, 3u, 1u))
            return false;
    }
    if (!OptionsChild(call, 0x82bfbbu, 0x82a07du, 2u, 1u))
        return false;
    OpLoadA(cpu, 8u);
    TestBitsDirect(memory, cpu, OPTION_REFRESH_REQUEST, 1u);
    return true;
}

Lufia2ExecutionResult Lufia2MenuRunOptionsScreen(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82bf13u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 5u);
    OpLdx(cpu, 5u);
    if (!OptionsChild(&call, 0x82bf18u, 0x82895bu, 2u, 1u) ||
        !OptionsChild(&call, 0x82bf1bu, 0x868b55u, 3u, 1u) ||
        !OptionsChild(&call, 0x82bf1fu, 0x868b32u, 3u, 1u) ||
        !OptionsChild(&call, 0x82bf23u, 0x82a023u, 2u, 1u))
        return call.result;
    OpLoadA(cpu, 0u);
    for (;;) {
        if (!OptionsChild(&call, 0x82bf28u, 0x828b08u, 2u, 1u))
            return call.result;
        OpCmpValue(cpu, 1u);
        bool previous = false;
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_PRESSED_B));
            OpBitValue(cpu, 2u);
            if (!cpu->zero) {
                previous = true;
            } else {
                OpBitValue(cpu, 1u);
                if (cpu->zero) {
                    OpLoadA(cpu, 1u);
                    continue;
                }
            }
        } else {
            OpCmpValue(cpu, 2u);
            if (cpu->zero) {
                OpLoadA(cpu, 2u);
                if (!OptionsChild(&call, 0x82bf68u, 0x80953bu, 3u, 1u))
                    return call.result;
            } else {
                OpCmpValue(cpu, 3u);
                if (cpu->zero)
                    break;
                OpLoadA(cpu, 0u);
                continue;
            }
        }
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX));
        const uint32_t previous_resume = cpu->resume_pc;
        const uint32_t target = DispatchOption(memory, cpu,
            previous ? 0x82bf4fu : 0x82bf5bu);
        if (target != 0x82bf79u && target != 0x82bf88u &&
            target != 0x82bf99u && target != 0x82bfa3u && target != 0x82bfadu)
            return ExecutionHandoff(cpu, target);
        cpu->resume_pc = previous_resume;
        if (!ChangeOption(&call, target))
            return call.result;
        OpLoadA(cpu, 1u);
    }
    if (!OptionsChild(&call, 0x82bf6eu, 0x868b32u, 3u, 1u) ||
        !OptionsChild(&call, 0x82bf72u, 0x82838fu, 2u, 1u) ||
        !OptionsChild(&call, 0x82bf75u, 0x829ae0u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82bf78u);
}
