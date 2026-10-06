#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    SELECT_STREAM = 0xc3u,
    SELECT_BRANCH = WRAM_BATTLE_EFFECT_BRANCH_STREAM,
    SELECT_PARENT = WRAM_BATTLE_EFFECT_BRANCH_PARENT,
    SELECT_RETURN = 5u,
    SELECT_TARGET = 0x7e0025u
};

static bool SelectionContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        cpu->direct_page == 0u && cpu->program_bank == 0x81u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void AdvanceSelectionStream(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetNz16(cpu, WramStep16(WramViewOfCaller(memory, cpu), SELECT_STREAM, 1));
}

Lufia2ExecutionResult Lufia2BattleEffectBranchIfField(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!SelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x8191b7u);
    OpSetDataBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    wram = WramViewOfCaller(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, SELECT_STREAM)));
    WramWrite16(wram, SELECT_BRANCH, cpu->accumulator);
    AdvanceSelectionStream(memory, cpu);
    AdvanceSelectionStream(memory, cpu);
    WramWrite16(wram, SELECT_PARENT, cpu->y);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, SELECT_STREAM)) & 0xffu);
    cpu->carry = false;
    OpAdcValue(cpu, WramRead16(wram, SELECT_PARENT));
    LoadX16(cpu, cpu->accumulator);
    AdvanceSelectionStream(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, SELECT_STREAM)));
    AdvanceSelectionStream(memory, cpu);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, WramRead16(wram, SELECT_STREAM));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, SELECT_RETURN, cpu->y),
                cpu->accumulator);
    LoadA16(cpu, WramRead16(wram, SELECT_BRANCH));
    WramWrite16(wram, SELECT_STREAM, cpu->accumulator);
    PullAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    Compare8(cpu, A8(cpu), Read8(memory, AbsoluteIndexedAddress(cpu, 0u, cpu->x)));
    if (!cpu->zero) {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SELECT_RETURN, cpu->y));
        WramWrite16(wram, SELECT_STREAM, cpu->x);
    }
    return ExecutionReturned(0x8191f1u);
}

Lufia2ExecutionResult Lufia2BattleEffectSelectPortraitStream(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!SelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x8194cau);
    wram = WramViewOfCaller(memory, cpu);
    TransferYToX(cpu);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(SELECT_TARGET, cpu->x)));
    if (cpu->negative)
        return ExecutionHandoff(cpu, 0x8194eau);
    Compare8(cpu, A8(cpu), 4u);
    if (cpu->zero)
        return ExecutionHandoff(cpu, 0x8194eau);
    LoadX16(cpu, cpu->accumulator);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(
        WRAM_BATTLE_PARTY_IDS & 0xffffu, cpu->x)));
    AslA8(cpu);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = false;
    OpAdcValue(cpu, WramRead16(wram, SELECT_STREAM));
    WramWrite16(wram, SELECT_STREAM, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, SELECT_STREAM)));
    WramWrite16(wram, SELECT_STREAM, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x8194e9u);
}

Lufia2ExecutionResult Lufia2BattleEffectSkipArgument(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SelectionContext(cpu))
        return ExecutionHandoff(cpu, 0x819ba3u);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, SELECT_STREAM)));
    SetAccumulatorWidth(cpu, 0);
    AdvanceSelectionStream(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x819babu);
}
