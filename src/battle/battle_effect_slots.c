#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    SLOT_STREAM = 0xc3u,
    SLOT_SOURCE = WRAM_BATTLE_EFFECT_SPAWN_STREAM,
    SLOT_POSITION = WRAM_BATTLE_EFFECT_SPAWN_POSITION,
    SLOT_ATTRIBUTES = WRAM_BATTLE_EFFECT_SPAWN_ATTRIBUTES,
    SLOT_SCRIPT_FIRST = WRAM_BATTLE_EFFECT_SCRIPT_SLOTS & 0xffffu,
    SLOT_SCRIPT_STRIDE = 0x2cu,
    SLOT_ACTOR_FIRST = WRAM_BATTLE_EFFECT_ACTOR_SLOTS & 0xffffu,
    SLOT_ACTOR_STRIDE = 0x2du,
    SLOT_ACTIVE_COUNT = WRAM_BATTLE_EFFECT_ACTIVE_COUNT,
    SLOT_DISPATCH_COUNT = WRAM_BATTLE_EFFECT_SLOT_COUNTER
};

static bool SlotContext(const Lufia2CpuState *cpu, bool allocates) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->direct_page == 0u &&
        cpu->program_bank == 0x81u &&
        cpu->stack >= (allocates ? 0x1f02u : 0x1f00u) &&
        cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult FindEffectSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool actor) {
    const uint16_t first = actor ? SLOT_ACTOR_FIRST : SLOT_SCRIPT_FIRST;
    const uint16_t stride = actor ? SLOT_ACTOR_STRIDE : SLOT_SCRIPT_STRIDE;
    const unsigned count = actor ? WRAM_BATTLE_EFFECT_ACTOR_SLOTS_COUNT
                                 : WRAM_BATTLE_EFFECT_SCRIPT_SLOTS_COUNT;
    const uint16_t end = (uint16_t)(first + stride * count);

    LoadX16(cpu, first);
    for (unsigned slot = 0; slot < count; ++slot) {
        LoadA8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0u, cpu->x)));
        if (cpu->zero)
            return ExecutionReturned(actor ? 0x818faau : 0x818fc4u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, cpu->x);
        cpu->carry = false;
        Add16Value(cpu, stride);
        LoadX16(cpu, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        Compare16(cpu, cpu->x, end);
    }
    return ExecutionHandoff(cpu, actor ? 0x818fa8u : 0x818fc2u);
}

Lufia2ExecutionResult Lufia2BattleEffectFindActorSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SlotContext(cpu, false))
        return ExecutionHandoff(cpu, 0x818f91u);
    return FindEffectSlot(memory, cpu, true);
}

Lufia2ExecutionResult Lufia2BattleEffectFindScriptSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SlotContext(cpu, false))
        return ExecutionHandoff(cpu, 0x818fabu);
    return FindEffectSlot(memory, cpu, false);
}

static Lufia2ExecutionResult AllocateEffectSlot(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool actor, uint16_t site) {
    Lufia2ExecutionResult result;

    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    result = FindEffectSlot(memory, cpu, actor);
    if (result.flow == LUFIA2_EXECUTION_RETURNED) {
        (void)Pull8(memory, cpu);
        (void)Pull8(memory, cpu);
    }
    return result;
}

static void CopyEffectParameters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (uint16_t field = 0x13u; field <= 0x19u; field += 2u) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, field, cpu->y));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, field, cpu->x),
                    cpu->accumulator);
    }
}

static void InitializeActorAxis(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool moving, bool vertical) {
    const uint16_t source = (uint16_t)(SLOT_POSITION + (vertical ? 1u : 0u));
    const uint16_t velocity = vertical ? 0x1du : 0x1bu;
    const uint16_t position = vertical ? 0x21u : 0x1fu;
    const uint16_t encoded = Read16AbsoluteIndexed(memory, cpu, source, 0u);
    const uint16_t offset = encoded & 0xffu;

    LoadA16(cpu, offset & 0x80u ? (uint16_t)(offset | 0xff00u) : offset);
    if (moving) {
        Write16Long(memory, AbsoluteIndexedAddress(cpu, velocity, cpu->x),
                    cpu->accumulator);
    } else {
        cpu->carry = false;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, velocity, cpu->y));
        cpu->carry = false;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, position, cpu->y));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, position, cpu->x),
                    cpu->accumulator);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, velocity, cpu->x), 0u);
    }
}

static Lufia2ExecutionResult SpawnEffectActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool moving) {
    static const uint16_t counters[] = {0u, 1u, 2u, 7u, 0xau, 0xdu, 0x10u};
    Lufia2ExecutionResult result;
    Lufia2Wram wram;
    const uint32_t entry = moving ? 0x81905bu : 0x818fc5u;

    if (!SlotContext(cpu, true))
        return ExecutionHandoff(cpu, entry);
    OpSetDataBank(memory, cpu, 0x7eu);
    result = AllocateEffectSlot(memory, cpu, true, moving ? 0x905fu : 0x8fc9u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    wram = WramViewOfCaller(memory, cpu);
    LoadA8(cpu, 1u);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0u, cpu->x), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, SLOT_SOURCE));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 3u, cpu->x), cpu->accumulator);
    InitializeActorAxis(memory, cpu, moving, false);
    InitializeActorAxis(memory, cpu, moving, true);
    CopyEffectParameters(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, WramRead(wram, SLOT_ATTRIBUTES));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x23u, cpu->x), A8(cpu));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x24u, cpu->x), A8(cpu));
    LoadA8(cpu, 1u);
    for (unsigned counter = 0; counter < sizeof(counters) / sizeof(counters[0]);
         ++counter)
        Write8(memory, AbsoluteIndexedAddress(cpu, counters[counter], cpu->x), A8(cpu));
    if (moving) {
        LoadA8(cpu, 0x40u);
        cpu->carry = true;
        Sbc8(cpu, WramRead(wram, SLOT_DISPATCH_COUNT));
    } else {
        LoadA8(cpu, 0xffu);
    }
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x25u, cpu->x), A8(cpu));
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x26u, cpu->x), 0u);
    SetNz8(cpu, WramStep(wram, SLOT_ACTIVE_COUNT, 1));
    return ExecutionReturned(moving ? 0x8190deu : 0x81905au);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnEffectActor(memory, cpu, false);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnMovingActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return SpawnEffectActor(memory, cpu, true);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    Lufia2Wram wram;

    if (!SlotContext(cpu, true))
        return ExecutionHandoff(cpu, 0x8190dfu);
    result = AllocateEffectSlot(memory, cpu, false, 0x90dfu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    wram = WramViewOfCaller(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    CopyEffectParameters(memory, cpu);
    LoadA16(cpu, WramRead16(wram, SLOT_SOURCE));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 3u, cpu->x), cpu->accumulator);
    LoadA16(cpu, (uint16_t)(WramRead16(wram, SLOT_POSITION) & 0xffu));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1fu, cpu->x), cpu->accumulator);
    LoadA16(cpu, (uint16_t)(WramRead16(wram, SLOT_POSITION + 1u) & 0xffu));
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x21u, cpu->x), cpu->accumulator);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1bu, cpu->x), 0u);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1du, cpu->x), 0u);
    LoadA16(cpu, 0x101u);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x23u, cpu->x), cpu->accumulator);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 0u, cpu->x), cpu->accumulator);
    Write16Long(memory, AbsoluteIndexedAddress(cpu, 1u, cpu->x), cpu->accumulator);
    SetNz16(cpu, WramStep16(wram, SLOT_ACTIVE_COUNT, 1));
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81912bu);
}

static void ReadSpawnArgument(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t destination) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, SLOT_STREAM)));
    (void)WramStep16(wram, SLOT_STREAM, 1);
    (void)WramStep16(wram, SLOT_STREAM, 1);
    WramWrite16(wram, destination, cpu->accumulator);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnScriptAt(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SlotContext(cpu, true))
        return ExecutionHandoff(cpu, 0x8191f2u);
    OpSetDataBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    ReadSpawnArgument(memory, cpu, SLOT_SOURCE);
    ReadSpawnArgument(memory, cpu, SLOT_POSITION);
    SetAccumulatorWidth(cpu, 1);
    return Lufia2BattleEffectSpawnScript(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnActorAt(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    if (!SlotContext(cpu, true))
        return ExecutionHandoff(cpu, 0x81929eu);
    SetAccumulatorWidth(cpu, 0);
    ReadSpawnArgument(memory, cpu, SLOT_SOURCE);
    ReadSpawnArgument(memory, cpu, SLOT_POSITION);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, SLOT_STREAM)));
    wram = WramViewOfCaller(memory, cpu);
    SetNz16(cpu, WramStep16(wram, SLOT_STREAM, 1));
    SetAccumulatorWidth(cpu, 1);
    WramWrite(wram, SLOT_ATTRIBUTES, A8(cpu));
    return Lufia2BattleEffectSpawnActor(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnScriptsForTargets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t targets = WRAM_BATTLE_EFFECT_TARGET_COUNT & 0xffffu;
    const uint16_t target_stride = 0xbu;
    Lufia2Wram wram;

    if (!SlotContext(cpu, true) || cpu->stack < 0x1f04u)
        return ExecutionHandoff(cpu, 0x81920fu);
    OpSetDataBank(memory, cpu, 0x7eu);
    SetAccumulatorWidth(cpu, 0);
    wram = WramViewOfCaller(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, SLOT_STREAM)));
    WramWrite16(wram, 0u, cpu->accumulator);
    (void)WramStep16(wram, SLOT_STREAM, 1);
    (void)WramStep16(wram, SLOT_STREAM, 1);
    LoadA16(cpu, WramRead16(wram, targets) & 0xffu);
    if (cpu->zero) {
        SetAccumulatorWidth(cpu, 1);
        return ExecutionReturned(0x81929du);
    }
    WramWrite16(wram, 2u, cpu->accumulator);
    for (uint16_t field = 0x13u; field <= 0x19u; field += 2u) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, field, cpu->y));
        WramWrite16(wram, (uint16_t)(field - 2u), cpu->accumulator);
    }
    PushY(memory, cpu);
    LoadY16(cpu, (uint16_t)(targets + 1u));
    for (;;) {
        Lufia2ExecutionResult result;

        SetAccumulatorWidth(cpu, 1);
        result = AllocateEffectSlot(memory, cpu, false, 0x9241u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, WramRead16(wram, 0u));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 3u, cpu->x), cpu->accumulator);
        for (uint16_t field = 0x13u; field <= 0x19u; field += 2u) {
            LoadA16(cpu, WramRead16(wram, (uint16_t)(field - 2u)));
            Write16Long(memory, AbsoluteIndexedAddress(cpu, field, cpu->x),
                        cpu->accumulator);
        }
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0u, cpu->y) & 0xffu);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1fu, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 1u, cpu->y) & 0xffu);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x21u, cpu->x), cpu->accumulator);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1bu, cpu->x), 0u);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x1du, cpu->x), 0u);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 2u, cpu->y));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x23u, cpu->x), cpu->accumulator);
        LoadA16(cpu, 0x101u);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0u, cpu->x), cpu->accumulator);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 1u, cpu->x), cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xau, cpu->y));
        OpDecA(cpu);
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x25u, cpu->x), cpu->accumulator);
        LoadA16(cpu, cpu->y);
        cpu->carry = false;
        Add16Value(cpu, target_stride);
        LoadY16(cpu, cpu->accumulator);
        SetNz16(cpu, WramStep16(wram, SLOT_ACTIVE_COUNT, 1));
        SetNz16(cpu, WramStep16(wram, 2u, -1));
        if (cpu->zero)
            break;
    }
    cpu->y = PullIndexValue(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x81929du);
}
