#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    FULL_BRIGHTNESS = 15u,
    LAST_FADE_FRAME = 14u
};

static uint8_t FadeEntryFits(const Lufia2CpuState *cpu,
    Lufia2PushedChildCall child) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f0au && cpu->stack <= 0x1ffcu && child;
}

static Lufia2ExecutionResult FadeChildUnwound(uint32_t site) {
    const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site};
    return result;
}

static Lufia2ExecutionResult DrawFadeFrame(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t command_site, uint32_t input_site) {
    PushAccumulator8(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            command_site, 0x81d9d0u, 2u, 0x81u))
        return FadeChildUnwound(command_site);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_BATTLE_EFFECT_FRAME_MARKER);
    if (!CallChildWithFrame(memory, cpu, child, context,
            input_site, 0x85ec81u, 3u, 0x81u))
        return FadeChildUnwound(input_site);
    OpLoadA(cpu, Pull8(memory, cpu));
    return ExecutionReturned(input_site + 4u);
}

Lufia2ExecutionResult Lufia2BattleFadeOutWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FadeEntryFits(cpu, child))
        return ExecutionHandoff(cpu, 0x81c321u);
    OpLoadA(cpu, LAST_FADE_FRAME);
    do {
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        const Lufia2ExecutionResult result = DrawFadeFrame(memory, cpu,
            child, context, 0x81c327u, 0x81c330u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpDecA(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x81c338u);
}

Lufia2ExecutionResult Lufia2BattleFadeInWindows(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FadeEntryFits(cpu, child))
        return ExecutionHandoff(cpu, 0x81c339u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81c339u, 0x81c2fbu, 2u, 0x81u))
        return FadeChildUnwound(0x81c339u);
    OpRepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81c33eu, 0x859c08u, 3u, 0x81u))
        return FadeChildUnwound(0x81c33eu);
    OpSepWidths(cpu, 0x20u);
    LoadA16(cpu, cpu->direct_page);
    for (;;) {
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BRIGHTNESS));
        OpCmpValue(cpu, FULL_BRIGHTNESS);
        if (cpu->zero)
            break;
        const Lufia2ExecutionResult result = DrawFadeFrame(memory, cpu,
            child, context, 0x81c348u, 0x81c351u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    return ExecutionReturned(0x81c35eu);
}
