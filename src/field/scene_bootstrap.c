#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    SCENE_RECORD_COUNT = 8u,
    SCENE_RECORD_TABLE = 0x878010u,
    SCENE_TEXT_FLAGS = 0xa7u
};

static bool SceneBootstrapChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, target, frame, 0x80u) != 0u;
}

static Lufia2ExecutionResult SceneBootstrapUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2SceneResetTextState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u)
        return ExecutionHandoff(cpu, 0x80a368u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_DISPLAY_MODE));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_SCENE_TEXT_AUXILIARY));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    return ExecutionReturned(0x80a371u);
}

static void SceneBootstrapResetText(const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t back) {
    SimulateJsrFrame(memory, cpu, back);
    Lufia2SceneResetTextState(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static void SceneBootstrapClearText(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpAbs(cpu, WRAM_TEXT_STATE));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_SCENE_TEXT_CONTROL));
}

Lufia2ExecutionResult Lufia2SceneRunStartRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80be89u);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 2u);
    if (!SceneBootstrapChild(memory, cpu, child, context, 0x80be8eu, 0x80c12eu, 3u))
        return SceneBootstrapUnwound(0x80be8eu);
    OpCpy(cpu, 0xffffu);
    if (!cpu->zero) {
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09B7), cpu->y);
        SetAccumulatorWidth(cpu, 1u);
        SceneBootstrapResetText(memory, cpu, 0xbe9eu);
        OpLoadA(cpu, 0x40u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_SCENE_TEXT_CONTROL), 1u);
        if (!SceneBootstrapChild(memory, cpu, child, context, 0x80bea4u, 0x809cb8u, 3u))
            return SceneBootstrapUnwound(0x80bea4u);
        SceneBootstrapClearText(memory, cpu);
    }
    return ExecutionReturned(0x80beaeu);
}

static Lufia2ExecutionResult SceneBootstrapSavedRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint8_t record, uint32_t entry, uint32_t site) {
    if (!child || cpu->program_bank != 0x80u || cpu->decimal)
        return ExecutionHandoff(cpu, entry);
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    OpLoadA(cpu, record);
    if (!SceneBootstrapChild(memory, cpu, child, context, site, 0x80be89u, 2u))
        return SceneBootstrapUnwound(site);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return ExecutionReturned(site + 8u);
}

Lufia2ExecutionResult Lufia2SceneRunInitialRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SceneBootstrapSavedRecord(memory, cpu, child, context, 0u, 0x80be4du, 0x80be58u);
}

Lufia2ExecutionResult Lufia2SceneRunResumeRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SceneBootstrapSavedRecord(memory, cpu, child, context, 2u, 0x80be61u, 0x80be6cu);
}

Lufia2ExecutionResult Lufia2SceneRunTransitionRecord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return SceneBootstrapSavedRecord(memory, cpu, child, context, 1u, 0x80be75u, 0x80be80u);
}

Lufia2ExecutionResult Lufia2SceneRunMapText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->program_bank != 0x80u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x80beafu);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0u);
    SetIndexWidth(cpu, 0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    for (unsigned bit = 0u; bit < 3u; ++bit)
        OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_TABLE + 3u));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x8000u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09B7));
    SetAccumulatorWidth(cpu, 1u);
    OpLda(memory, cpu, OpLongX(cpu, SCENE_RECORD_TABLE + 5u));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x87u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK));
    SetAccumulatorWidth(cpu, 0u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09B7));
    if (!SceneBootstrapChild(memory, cpu, child, context, 0x80bed5u, 0x80c102u, 2u))
        return SceneBootstrapUnwound(0x80bed5u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_UNK_7E09B7), cpu->y);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BASE));
    PushAccumulator16(memory, cpu);
    OpLoadA(cpu, 0x8000u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BASE));
    OpLoadA(cpu, SCENE_RECORD_COUNT);
    OpSta(memory, cpu, OpDp(cpu, SCENE_TEXT_FLAGS));
    SetAccumulatorWidth(cpu, 1u);
    SceneBootstrapResetText(memory, cpu, 0xbeeeu);
    OpLoadA(cpu, 0x40u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_SCENE_TEXT_CONTROL), 1u);
    if (!SceneBootstrapChild(memory, cpu, child, context, 0x80bef4u, 0x809cb8u, 3u))
        return SceneBootstrapUnwound(0x80bef4u);
    SceneBootstrapClearText(memory, cpu);
    SetAccumulatorWidth(cpu, 0u);
    PullAccumulator16(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BASE));
    SetAccumulatorWidth(cpu, 1u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x80bf07u);
}
