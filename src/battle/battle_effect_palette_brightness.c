#include "core/cpu_ops.h"
#include "core/child_call.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_PALETTE_INDEX = 0x11u,
    DP_BRIGHTNESS_LEVEL = 0x13u,
    DP_BRIGHTNESS_STREAM = 0xc3u,
    PALETTE_BRIGHTNESS_LEVELS = WRAM_UNK_7E1BF1,
    PALETTE_WHITE_LIMIT = 0x40u,
    PALETTE_BLACK_LIMIT = 0xc0u
};

static uint8_t BrightnessCommandContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f06u && cpu->stack <= 0x1ffcu;
}

static void ReadBrightnessCommand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_BRIGHTNESS_STREAM));
    OpSta(memory, cpu, OpDp(cpu, DP_PALETTE_INDEX));
    OpRepWidths(cpu, 0x20u);
    OpTax(cpu);
    OpStepMem(memory, cpu, OpDp(cpu, DP_BRIGHTNESS_STREAM), 1);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_BRIGHTNESS_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_BRIGHTNESS_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_BRIGHTNESS_LEVEL));
}

static Lufia2ExecutionResult ApplyBrightnessCommand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t end) {
    OpSta(memory, cpu, OpAbsX(cpu, PALETTE_BRIGHTNESS_LEVELS));
    PushY(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context, site, 0x81b396u, 2u, 0x81u)) {
        const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return result;
    }
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(end);
}

Lufia2ExecutionResult Lufia2BattleEffectSetPaletteBrightness(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!BrightnessCommandContext(cpu))
        return ExecutionHandoff(cpu, 0x819853u);
    ReadBrightnessCommand(memory, cpu);
    return ApplyBrightnessCommand(memory, cpu, child, context, 0x819869u, 0x81986du);
}

Lufia2ExecutionResult Lufia2BattleEffectAdjustPaletteBrightness(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!BrightnessCommandContext(cpu))
        return ExecutionHandoff(cpu, 0x819935u);
    ReadBrightnessCommand(memory, cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsX(cpu, PALETTE_BRIGHTNESS_LEVELS));
    OpCmpValue(cpu, PALETTE_WHITE_LIMIT);
    if (cpu->carry) {
        OpCmpValue(cpu, PALETTE_BLACK_LIMIT);
        if (!cpu->carry) {
            OpLda(memory, cpu, OpDp(cpu, DP_BRIGHTNESS_LEVEL));
            LoadA8(cpu, cpu->negative ? PALETTE_BLACK_LIMIT : PALETTE_WHITE_LIMIT);
        }
    }
    OpSta(memory, cpu, OpDp(cpu, DP_BRIGHTNESS_LEVEL));
    return ApplyBrightnessCommand(memory, cpu, child, context, 0x819963u, 0x819967u);
}
