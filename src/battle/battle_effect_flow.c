#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    FLOW_STREAM = 0xc3u,
    FLOW_WORK_BANK = 0x7eu,
    FLOW_SLOT_ACTIVE = 0x7e0000u,
    FLOW_SLOT_DELAY = 0x7e0002u,
    FLOW_SLOT_REPEAT_TARGET = 0x7e0005u,
    FLOW_ACTIVE_COUNT = WRAM_BATTLE_EFFECT_ACTIVE_COUNT,
    FLOW_LOOP_FIRST = 0x7e0007u,
    FLOW_LOOP_STRIDE = 3u
};

static bool FlowContext(const Lufia2CpuState *cpu, bool leaves_dispatch) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        cpu->direct_page == 0u && cpu->program_bank == 0x81u &&
        cpu->stack >= 0x1f00u &&
        cpu->stack <= (leaves_dispatch ? 0x1ffau : 0x1ffcu);
}

Lufia2ExecutionResult Lufia2BattleEffectEnd(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!FlowContext(cpu, true))
        return ExecutionHandoff(cpu, 0x81915bu);
    OpSetDataBank(memory, cpu, FLOW_WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    TransferDirectToA(cpu);
    WramWriteAt(wram, FLOW_SLOT_ACTIVE, cpu->y, A8(cpu));
    SetNz8(cpu, WramStep(wram, FLOW_ACTIVE_COUNT, -1));
    /* Discard the opcode return before leaving its dispatcher. */
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x818c59u);
}

Lufia2ExecutionResult Lufia2BattleEffectDelay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!FlowContext(cpu, false))
        return ExecutionHandoff(cpu, 0x81917fu);
    OpSetDataBank(memory, cpu, FLOW_WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, FLOW_STREAM)));
    WramWriteAt(wram, FLOW_SLOT_DELAY, cpu->y, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetNz16(cpu, WramStep16(wram, FLOW_STREAM, 1));
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81918eu);
}

Lufia2ExecutionResult Lufia2BattleEffectJump(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!FlowContext(cpu, false))
        return ExecutionHandoff(cpu, 0x81918fu);
    SetAccumulatorWidth(cpu, 0);
    wram = WramViewOfCaller(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, FLOW_STREAM)));
    WramWrite16(wram, FLOW_STREAM, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x819197u);
}

Lufia2ExecutionResult Lufia2BattleEffectRepeatStart(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!FlowContext(cpu, false))
        return ExecutionHandoff(cpu, 0x819198u);
    OpSetDataBank(memory, cpu, FLOW_WORK_BANK);
    SetAccumulatorWidth(cpu, 0);
    wram = WramViewOfCaller(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, FLOW_STREAM)));
    LoadX16(cpu, WramRead16(wram, FLOW_STREAM));
    IncrementX16(cpu);
    IncrementX16(cpu);
    WramWrite16(wram, FLOW_STREAM, cpu->accumulator);
    LoadA16(cpu, cpu->x);
    WramWrite16At(wram, FLOW_SLOT_REPEAT_TARGET, cpu->y, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x8191acu);
}

Lufia2ExecutionResult Lufia2BattleEffectRepeatJump(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!FlowContext(cpu, false))
        return ExecutionHandoff(cpu, 0x8191adu);
    OpSetDataBank(memory, cpu, FLOW_WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    LoadX16(cpu, WramRead16At(wram, FLOW_SLOT_REPEAT_TARGET, cpu->y));
    WramWrite16(wram, FLOW_STREAM, cpu->x);
    return ExecutionReturned(0x8191b6u);
}

static Lufia2ExecutionResult BeginCountedLoop(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    unsigned level, uint32_t entry, uint32_t returned) {
    Lufia2Wram wram;
    uint16_t next;
    const uint32_t counter = FLOW_LOOP_FIRST + level * FLOW_LOOP_STRIDE;

    if (!FlowContext(cpu, false))
        return ExecutionHandoff(cpu, entry);
    OpSetDataBank(memory, cpu, FLOW_WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, FLOW_STREAM)));
    WramWriteAt(wram, counter, cpu->y, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    (void)WramStep16(wram, FLOW_STREAM, 1);
    next = WramRead16(wram, FLOW_STREAM);
    WramWrite16At(wram, counter + 1u, cpu->y, next);
    LoadA16(cpu, next);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(returned);
}

static Lufia2ExecutionResult AdvanceCountedLoop(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    unsigned level, uint32_t entry, uint32_t returned) {
    Lufia2Wram wram;
    uint8_t remaining;
    const uint32_t counter = FLOW_LOOP_FIRST + level * FLOW_LOOP_STRIDE;

    if (!FlowContext(cpu, false))
        return ExecutionHandoff(cpu, entry);
    OpSetDataBank(memory, cpu, FLOW_WORK_BANK);
    wram = WramViewOfCaller(memory, cpu);
    LoadX16(cpu, cpu->y);
    remaining = (uint8_t)(WramReadAt(wram, counter, cpu->x) - 1u);
    WramWriteAt(wram, counter, cpu->x, remaining);
    SetNz8(cpu, remaining);
    if (remaining != 0u) {
        uint16_t target;
        SetAccumulatorWidth(cpu, 0);
        target = WramRead16At(wram, counter + 1u, cpu->y);
        WramWrite16(wram, FLOW_STREAM, target);
        LoadA16(cpu, target);
        SetAccumulatorWidth(cpu, 1);
    }
    return ExecutionReturned(returned);
}

Lufia2ExecutionResult Lufia2BattleEffectLoopStart(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BeginCountedLoop(memory, cpu, 0u, 0x8194ebu, 0x8194ffu);
}

Lufia2ExecutionResult Lufia2BattleEffectLoopStart2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BeginCountedLoop(memory, cpu, 1u, 0x819500u, 0x819514u);
}

Lufia2ExecutionResult Lufia2BattleEffectLoopStart3(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BeginCountedLoop(memory, cpu, 2u, 0x819515u, 0x819529u);
}

Lufia2ExecutionResult Lufia2BattleEffectLoopStart4(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return BeginCountedLoop(memory, cpu, 3u, 0x81952au, 0x81953eu);
}

Lufia2ExecutionResult Lufia2BattleEffectLoopNext2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return AdvanceCountedLoop(memory, cpu, 1u, 0x819553u, 0x819566u);
}

Lufia2ExecutionResult Lufia2BattleEffectLoopNext3(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return AdvanceCountedLoop(memory, cpu, 2u, 0x819567u, 0x81957au);
}

Lufia2ExecutionResult Lufia2BattleEffectLoopNext4(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return AdvanceCountedLoop(memory, cpu, 3u, 0x81957bu, 0x81958eu);
}
