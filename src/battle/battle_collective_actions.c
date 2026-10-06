#include "battle/battle_internal.h"

enum {
    COLLECTIVE_ACTOR = WRAM_BATTLE_ACTION_WORK,
    COLLECTIVE_PARAMETER = WRAM_BATTLE_ACTION_WORK + 8u,
    COLLECTIVE_MESSAGE_COUNT = WRAM_BATTLE_MESSAGE_COUNT & 0xffffu,
    COLLECTIVE_RESULT_COUNT = WRAM_BATTLE_RESULT_COUNT & 0xffffu,
    COLLECTIVE_WAIT_LIMIT = 4096u
};

typedef struct CollectiveAction {
    BattleContext battle;
    Lufia2ExecutionResult result;
} CollectiveAction;

typedef struct CollectiveWait {
    uint8_t frames;
    uint16_t start;
    uint16_t sprites;
    uint16_t input;
} CollectiveWait;

static bool CollectiveContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->direct_page == 0u &&
        cpu->program_bank == 0x81u && cpu->data_bank == 0x97u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool CollectiveCall(CollectiveAction *action, uint16_t site,
                           uint32_t target, uint8_t frame, bool byte_accumulator) {
    Lufia2CpuState *cpu = action->battle.cpu;

    if (!BattleCall(&action->battle, site, target, frame)) {
        action->result = BattleChildUnwound(&action->battle);
        return false;
    }
    if (cpu->accumulator_is_8_bit != byte_accumulator ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u) {
        action->result = ExecutionHandoff(cpu,
            0x810000u | (uint16_t)(site + (frame == 2u ? 3u : 4u)));
        return false;
    }
    return true;
}

static bool CollectiveWaitFrames(CollectiveAction *action, CollectiveWait wait) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadA8(cpu, wait.frames);
    for (unsigned frame = 0u; frame < COLLECTIVE_WAIT_LIMIT; ++frame) {
        PushAccumulator8(memory, cpu);
        if (!CollectiveCall(action, wait.sprites, BATTLE_ROUTINE_SPRITES, 3u, true))
            return false;
        LoadA8(cpu, 0xffu);
        Write8(memory, BATTLE_SPRITE_REBUILD_REQUEST, A8(cpu));
        if (!CollectiveCall(action, wait.input, BATTLE_ROUTINE_FRAME_INPUT, 3u, true))
            return false;
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
        if (cpu->zero)
            return true;
    }
    action->result = ExecutionHandoff(cpu, 0x810000u | wait.start);
    return false;
}

static void CollectiveResultCount(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  bool increment) {
    const uint16_t value = Read16AbsoluteIndexed(memory, cpu, COLLECTIVE_RESULT_COUNT, 0u);
    const uint16_t changed = (uint16_t)(increment ? value + 1u : value - 1u);

    Write8(memory, AbsoluteIndexedAddress(cpu, COLLECTIVE_RESULT_COUNT + 1u, 0u),
        (uint8_t)(changed >> 8));
    Write8(memory, AbsoluteIndexedAddress(cpu, COLLECTIVE_RESULT_COUNT, 0u), (uint8_t)changed);
    SetNz16(cpu, changed);
}

static bool CollectiveMessage(CollectiveAction *action, uint16_t message,
                               uint16_t build, uint16_t display) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadX16(cpu, message);
    if (!CollectiveCall(action, build, 0x8594e7u, 3u, true))
        return false;
    LoadX16(cpu, 1u);
    Write16Absolute(memory, cpu, COLLECTIVE_MESSAGE_COUNT, cpu->x);
    return CollectiveCall(action, display, 0x8595feu, 3u, true);
}

Lufia2ExecutionResult Lufia2BattleCollectiveAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    CollectiveAction action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!CollectiveContext(cpu))
        return ExecutionHandoff(cpu, 0x81ab94u);
    LoadA8(cpu, Read8(memory, COLLECTIVE_ACTOR));
    bool non_party = cpu->negative;
    if (!non_party) {
        Compare8(cpu, A8(cpu), BATTLE_ACTOR_CAPSULE);
        non_party = cpu->zero;
    }
    if (non_party) {
        if (!CollectiveCall(&action, 0xab9eu, 0x81b2b5u, 3u, true))
            return action.result;
        LoadAAbsolute8(memory, cpu, 0x000fu, cpu->x);
        Or8(cpu, 4u);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x000fu, cpu->x), A8(cpu));
        LoadA8(cpu, Read8(memory, COLLECTIVE_ACTOR));
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x09fau, 0u), A8(cpu));
        if (!CollectiveCall(&action, 0xabb1u, 0x85d9f0u, 3u, true) ||
            !CollectiveCall(&action, 0xabb5u, 0x859150u, 3u, true))
            return action.result;
        LoadA8(cpu, 2u);
        if (!CollectiveCall(&action, 0xabbbu, 0x859173u, 3u, true) ||
            !CollectiveCall(&action, 0xabbfu, 0x8595feu, 3u, true) ||
            !CollectiveCall(&action, 0xabc3u, BATTLE_ROUTINE_FRAME_INPUT, 3u, true) ||
            !CollectiveCall(&action, 0xabc7u, 0x81afc4u, 2u, true))
            return action.result;
        LoadA8(cpu, 0x31u);
        if (!CollectiveCall(&action, 0xabccu, 0x80953bu, 3u, true) ||
            !CollectiveWaitFrames(&action, (CollectiveWait){15u, 0xabd2u, 0xabd3u, 0xabddu}) ||
            !CollectiveCall(&action, 0xabe5u, 0x85948cu, 3u, true))
            return action.result;
        LoadA8(cpu, 0xffu);
        Write8(memory, BATTLE_SPRITE_REBUILD_REQUEST, A8(cpu));
        if (!CollectiveWaitFrames(&action, (CollectiveWait){15u, 0xabf1u, 0xabf2u, 0xabfcu}))
            return action.result;
        return ExecutionReturned(0x81ac04u);
    }
    LoadAAbsolute8(memory, cpu, WRAM_BATTLE_ACTION_SPECIAL_RESULT & 0xffffu, 0u);
    if (!cpu->zero) {
        if (!CollectiveMessage(&action, 0xf1f0u, 0xac0du, 0xac17u) ||
            !CollectiveWaitFrames(&action, (CollectiveWait){30u, 0xac1du, 0xac1eu, 0xac28u}))
            return action.result;
        return ExecutionReturned(0x81ac30u);
    }
    LoadA8(cpu, 2u);
    Write8(memory, WRAM_FIELD_BATTLE_RESULT, A8(cpu));
    if (!CollectiveMessage(&action, 0xf1e7u, 0xac3au, 0xac44u))
        return action.result;
    SetAccumulatorWidth(cpu, 0);
    CollectiveResultCount(memory, cpu, true);
    if (cpu->zero)
        CollectiveResultCount(memory, cpu, false);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x31u);
    if (!CollectiveCall(&action, 0xac56u, 0x80953bu, 3u, true) ||
        !CollectiveWaitFrames(&action, (CollectiveWait){60u, 0xac5cu, 0xac5du, 0xac67u}))
        return action.result;
    return ExecutionHandoff(cpu, 0x818855u);
}

static void ClearActionWindow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadX16(cpu, 0x2800u);
    LoadY16(cpu, 0x02a0u);
    Write16Absolute(memory, cpu, 0x2181u, cpu->x);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x2183u, 0u), 0u);
    LoadA8(cpu, 1u);
    do {
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x2180u, 0u), 0u);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x2180u, 0u), A8(cpu));
        cpu->y = (uint16_t)(cpu->y - 1u);
        SetNz16(cpu, cpu->y);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2BattleAction07(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    CollectiveAction action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!CollectiveContext(cpu))
        return ExecutionHandoff(cpu, 0x81ac72u);
    LoadA8(cpu, Read8(memory, COLLECTIVE_PARAMETER));
    if (!CollectiveCall(&action, 0xac76u, 0x85e0e4u, 3u, true))
        return action.result;
    if (cpu->carry) {
        LoadX16(cpu, 0xf245u);
        if (!CollectiveCall(&action, 0xac7fu, 0x8594e7u, 3u, true) ||
            !CollectiveCall(&action, 0xac83u, 0x8595feu, 3u, true) ||
            !CollectiveCall(&action, 0xac87u, 0x81afc4u, 2u, true) ||
            !CollectiveWaitFrames(&action, (CollectiveWait){20u, 0xac8cu, 0xac8du, 0xac97u}))
            return action.result;
        LoadX16(cpu, 0xf26au);
        if (!CollectiveCall(&action, 0xaca2u, 0x8594e7u, 3u, true) ||
            !CollectiveCall(&action, 0xaca6u, 0x8595feu, 3u, true) ||
            !CollectiveWaitFrames(&action, (CollectiveWait){30u, 0xacacu, 0xacadu, 0xacb7u}))
            return action.result;
        return ExecutionReturned(0x81acbfu);
    }
    if (!CollectiveMessage(&action, 0xf245u, 0xacc3u, 0xaccdu) ||
        !CollectiveCall(&action, 0xacd1u, 0x81afc4u, 2u, true) ||
        !CollectiveWaitFrames(&action, (CollectiveWait){30u, 0xacd6u, 0xacd7u, 0xace1u}))
        return action.result;
    LoadA8(cpu, Read8(memory, COLLECTIVE_PARAMETER));
    if (!CollectiveCall(&action, 0xacedu, 0x85e412u, 3u, true))
        return action.result;
    ClearActionWindow(memory, cpu);
    if (!CollectiveCall(&action, 0xad08u, BATTLE_ROUTINE_FRAME_INPUT, 3u, true))
        return action.result;
    SetAccumulatorWidth(cpu, 0);
    if (!CollectiveCall(&action, 0xad0eu, 0x859cd7u, 3u, false))
        return action.result;
    SetAccumulatorWidth(cpu, 1);
    if (!CollectiveMessage(&action, 0xf257u, 0xad17u, 0xad21u) ||
        !CollectiveWaitFrames(&action, (CollectiveWait){30u, 0xad27u, 0xad28u, 0xad32u}))
        return action.result;
    return ExecutionReturned(0x81ad3au);
}
