#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"

enum {
    DP_CIRCLE_RADIUS = 0x00u,
    DP_EFFECT_STREAM = 0xc3u,
    DP_CURRENT_HDMA_CHANNELS = 0xd9u,
    DP_REQUESTED_HDMA_CHANNELS = 0xdau,
    DP_HDMA_CHANGED = 0xdbu,
    EFFECT_PARAMETER_BASE = 0x13u,
    CIRCLE_PARAMETER_OFFSET = WRAM_BATTLE_EFFECT_BRANCH_STREAM,
    CIRCLE_DMA_MODE = WRAM_BATTLE_CIRCLE_HDMA_DESCRIPTOR,
    CIRCLE_DMA_REGISTER = WRAM_BATTLE_CIRCLE_HDMA_DESCRIPTOR + 1u,
    CIRCLE_DMA_TABLE = WRAM_BATTLE_CIRCLE_HDMA_DESCRIPTOR + 2u,
    CIRCLE_DMA_TABLE_BANK = WRAM_BATTLE_CIRCLE_HDMA_DESCRIPTOR + 4u,
    CIRCLE_ENABLED = WRAM_BATTLE_CIRCLE_WINDOW_ENABLED,
    CIRCLE_PHASE = WRAM_BATTLE_CIRCLE_WINDOW_STATE,
    CIRCLE_RADIUS = WRAM_BATTLE_CIRCLE_WINDOW_RADIUS,
    CIRCLE_PREVIOUS_RADIUS = WRAM_BATTLE_CIRCLE_WINDOW_PREVIOUS_RADIUS,
    CIRCLE_OBJECT_WINDOW = WRAM_FIELD_STREAMED_ROW_VRAM,
    CIRCLE_SECOND_WINDOW_LEFT = WRAM_FIELD_STREAMED_ROW_VRAM + 3u,
    CIRCLE_SECOND_WINDOW_RIGHT = WRAM_FIELD_STREAMED_ROW_VRAM + 4u,
    CIRCLE_CHANNEL_MASK = 0x40u,
    CIRCLE_TABLE_ADDRESS = 0xa572u
};

static uint8_t CircleWindowContext(const Lufia2CpuState *cpu,
    uint8_t bank) {
    return cpu->program_bank == bank && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult CircleWindowUnwind(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void ConfigureCircleWindowDma(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x41u);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_DMA_MODE));
    LoadA8(cpu, 0x28u);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_DMA_REGISTER));
    OpLdx(cpu, CIRCLE_TABLE_ADDRESS);
    OpWriteX(memory, cpu, OpAbs(cpu, CIRCLE_DMA_TABLE), cpu->x);
    LoadA8(cpu, 0x85u);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_DMA_TABLE_BANK));
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DASB(6u)));
    LoadA8(cpu, CIRCLE_CHANNEL_MASK);
    OpTestBits(memory, cpu, OpDp(cpu, DP_REQUESTED_HDMA_CHANNELS), 1u);
    LoadA8(cpu, 0x0bu);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_PHASE));
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_ENABLED));
    OpSta(memory, cpu, OpDp(cpu, DP_HDMA_CHANGED));
}

Lufia2ExecutionResult Lufia2BattleEnableCircleWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CircleWindowContext(cpu, 0x85u) || !child)
        return ExecutionHandoff(cpu, 0x85b1d6u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85b1d9u, 0x85b208u, 2u, 0x85u))
        return CircleWindowUnwind(0x85b1d9u);
    ConfigureCircleWindowDma(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85b207u);
}

static void CloseCircleWindow(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    LoadA8(cpu, CIRCLE_CHANNEL_MASK);
    OpTestBits(memory, cpu, OpDp(cpu, DP_CURRENT_HDMA_CHANNELS), 0u);
    OpStz(memory, cpu, OpAbs(cpu, CIRCLE_ENABLED));
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_SECOND_WINDOW_LEFT));
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_SECOND_WINDOW_RIGHT));
    LoadA8(cpu, 0x33u);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_OBJECT_WINDOW));
}

Lufia2ExecutionResult Lufia2BattleEffectSetCircleRadius(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CircleWindowContext(cpu, 0x81u) || !child)
        return ExecutionHandoff(cpu, 0x819a54u);
    PushY(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_PARAMETER_OFFSET));
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, CIRCLE_PARAMETER_OFFSET));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, EFFECT_PARAMETER_BASE));
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero) {
        CloseCircleWindow(memory, cpu);
        cpu->y = PullIndexValue(memory, cpu);
        return ExecutionReturned(0x819ab0u);
    }
    OpSta(memory, cpu, OpDp(cpu, DP_CIRCLE_RADIUS));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_CURRENT_HDMA_CHANNELS));
    OpAndValue(cpu, CIRCLE_CHANNEL_MASK);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, DP_CIRCLE_RADIUS));
        OpSta(memory, cpu, OpAbs(cpu, CIRCLE_RADIUS));
        cpu->y = PullIndexValue(memory, cpu);
        return ExecutionReturned(0x819a97u);
    }
    OpLda(memory, cpu, OpDp(cpu, DP_CIRCLE_RADIUS));
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_RADIUS));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_PREVIOUS_RADIUS));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819a86u, 0x85b1d6u, 3u, 0x81u))
        return CircleWindowUnwind(0x819a86u);
    LoadA8(cpu, 0xc3u);
    OpSta(memory, cpu, OpAbs(cpu, CIRCLE_OBJECT_WINDOW));
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819a90u);
}
