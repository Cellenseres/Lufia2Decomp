#include "battle/battle_internal.h"

enum {
    HANDLER_ACTOR = WRAM_BATTLE_ACTION_WORK,
    HANDLER_PARAMETER = WRAM_BATTLE_ACTION_WORK + 8u,
    HANDLER_EFFECT = WRAM_PALETTE_FADE & 0xffffu,
    HANDLER_MESSAGE_COUNT = WRAM_BATTLE_MESSAGE_COUNT & 0xffffu,
    HANDLER_ITEM_ID = WRAM_ITEM_RECORD_ID & 0xffffu,
    HANDLER_ITEM_NAME = WRAM_ITEM_RECORD_POINTER & 0xffffu,
    HANDLER_SPELL_ID = WRAM_MENU_SPELL_RECORD_ID & 0xffffu,
    HANDLER_SPELL_NAME = WRAM_SPELL_RECORD_POINTER & 0xffffu,
    HANDLER_MESSAGE_NAME = WRAM_BATTLE_MESSAGE_NAME_POINTER & 0xffffu,
    HANDLER_MESSAGE_BANK = WRAM_BATTLE_MESSAGE_NAME_BANK & 0xffffu,
    HANDLER_ITEM_EFFECT = (WRAM_RECORD_BUFFER & 0xffffu) + 0x30u,
    HANDLER_ATTACK_MESSAGE = (WRAM_RECORD_BUFFER & 0xffffu) + 0x1au,
    HANDLER_SPELL_MESSAGE = (WRAM_RECORD_BUFFER & 0xffffu) + 0x12u,
    HANDLER_ITEM_MESSAGE = (WRAM_RECORD_BUFFER & 0xffffu) + 0x18u,
    HANDLER_ITEM_FALLBACK_MESSAGE = (WRAM_RECORD_BUFFER & 0xffffu) + 0x16u,
    HANDLER_LOAD_ITEM = 0x81f1c5u,
    HANDLER_LOAD_SPELL = 0x81f414u,
    HANDLER_TARGET_RECORD = 0x81b2b5u,
    HANDLER_BUILD_MESSAGE = 0x81fac9u,
    HANDLER_RUN_ACTION = 0x81b057u,
    HANDLER_WAIT_LIMIT = 4096u
};

typedef struct ActionHandler {
    BattleContext battle;
    Lufia2ExecutionResult result;
} ActionHandler;

static bool HandlerContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->direct_page == 0u &&
        cpu->program_bank == 0x81u && cpu->data_bank == 0x97u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool HandlerCall(ActionHandler *action, uint16_t site,
                        uint32_t target, bool byte_accumulator) {
    Lufia2CpuState *cpu = action->battle.cpu;

    if (!BattleCall(&action->battle, site, target, 3u)) {
        action->result = BattleChildUnwound(&action->battle);
        return false;
    }
    if (cpu->accumulator_is_8_bit != byte_accumulator ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u) {
        action->result = ExecutionHandoff(cpu, 0x810000u | (uint16_t)(site + 4u));
        return false;
    }
    return true;
}

static void MessageName(ActionHandler *action, uint16_t name, uint8_t bank) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, name, 0u));
    Write16Absolute(memory, cpu, HANDLER_MESSAGE_NAME, cpu->y);
    LoadA8(cpu, bank);
    Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_MESSAGE_BANK, 0u), A8(cpu));
}

static bool HandlerWait(ActionHandler *action) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadA8(cpu, 30u);
    for (unsigned frame = 0u; frame < HANDLER_WAIT_LIMIT; ++frame) {
        PushAccumulator8(memory, cpu);
        if (!HandlerCall(action, 0xaa08u, BATTLE_ROUTINE_SPRITES, true))
            return false;
        LoadA8(cpu, 0xffu);
        Write8(memory, BATTLE_SPRITE_REBUILD_REQUEST, A8(cpu));
        if (!HandlerCall(action, 0xaa12u, BATTLE_ROUTINE_FRAME_INPUT, true))
            return false;
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
        if (cpu->zero)
            return true;
    }
    action->result = ExecutionHandoff(cpu, 0x81aa07u);
    return false;
}

Lufia2ExecutionResult Lufia2BattleSkipAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    (void)memory;
    (void)child;
    (void)child_context;
    return HandlerContext(cpu) ? ExecutionReturned(0x81a8a7u)
                               : ExecutionHandoff(cpu, 0x81a8a7u);
}

Lufia2ExecutionResult Lufia2BattleAttackAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionHandler action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!HandlerContext(cpu))
        return ExecutionHandoff(cpu, 0x81a8a8u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, HANDLER_PARAMETER));
    Write16Absolute(memory, cpu, HANDLER_ITEM_ID, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    if (!HandlerCall(&action, 0xa8b3u, HANDLER_LOAD_ITEM, true))
        return action.result;
    LoadAAbsolute8(memory, cpu, HANDLER_ITEM_EFFECT, 0u);
    if (!cpu->zero) {
        DecrementA8(cpu);
        Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_EFFECT, 0u), A8(cpu));
    }
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, HANDLER_PARAMETER));
    const bool unarmed = cpu->zero;
    SetAccumulatorWidth(cpu, 1);
    if (unarmed) {
        LoadAAbsolute8(memory, cpu, HANDLER_EFFECT, 0u);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, HANDLER_ACTOR));
            if (cpu->negative) {
                LoadA8(cpu, 0xe0u);
            } else {
                Compare8(cpu, A8(cpu), BATTLE_ACTOR_CAPSULE);
                LoadA8(cpu, cpu->zero ? 0xf0u : 0x0eu);
            }
            Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_EFFECT, 0u), A8(cpu));
        }
    }
    MessageName(&action, HANDLER_ITEM_NAME, 0x96u);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, HANDLER_ATTACK_MESSAGE, 0u));
    if (!cpu->zero) {
        if (!HandlerCall(&action, 0xa8fcu, HANDLER_BUILD_MESSAGE, true))
            return action.result;
    } else {
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Read8(memory, HANDLER_ACTOR));
        const bool enemy = cpu->negative;
        LoadY16(cpu, 0u);
        Write16Absolute(memory, cpu, HANDLER_MESSAGE_NAME, cpu->y);
        LoadA8(cpu, 0x85u);
        Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_MESSAGE_BANK, 0u), A8(cpu));
        LoadY16(cpu, enemy ? 0xef24u : 0xef26u);
        if (!HandlerCall(&action, 0xa928u, HANDLER_BUILD_MESSAGE, true))
            return action.result;
    }
    LoadA8(cpu, Read8(memory, HANDLER_ACTOR));
    if (!cpu->negative) {
        Compare8(cpu, A8(cpu), BATTLE_ACTOR_CAPSULE);
        if (!cpu->zero) {
            if (!HandlerCall(&action, 0xa936u, HANDLER_TARGET_RECORD, true))
                return action.result;
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x006eu, cpu->x));
            And16(cpu, 0x01ffu);
            if (!cpu->zero) {
                Write16Absolute(memory, cpu, HANDLER_ITEM_ID, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                if (!HandlerCall(&action, 0xa949u, HANDLER_LOAD_ITEM, true))
                    return action.result;
                MessageName(&action, HANDLER_ITEM_NAME, 0x96u);
                LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, HANDLER_ATTACK_MESSAGE, 0u));
                if (!cpu->zero && !HandlerCall(&action, 0xa95du, HANDLER_BUILD_MESSAGE, true))
                    return action.result;
            }
        }
    }
    SetAccumulatorWidth(cpu, 1);
    if (!HandlerCall(&action, 0xa963u, HANDLER_RUN_ACTION, true))
        return action.result;
    return ExecutionReturned(0x81a967u);
}

static Lufia2ExecutionResult RunSpellAction(ActionHandler *action) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadA8(cpu, Read8(memory, HANDLER_PARAMETER));
    Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_SPELL_ID, 0u), A8(cpu));
    if (!HandlerCall(action, 0xa97eu, 0x81f3f4u, true))
        return action->result;
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    Write16Direct(memory, cpu, 0u, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, HANDLER_ACTOR));
    SetAccumulatorWidth(cpu, 1);
    if (!HandlerCall(action, 0xa98fu, HANDLER_TARGET_RECORD, true))
        return action->result;
    LoadAAbsolute8(memory, cpu, BATTLE_BATTLER_STATUS, cpu->x);
    BitImmediate8(cpu, 2u);
    bool blocked = !cpu->zero;
    if (!blocked) {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, 0x01dcu);
        Write16Long(memory, WRAM_BATTLE_ACTION_WORK + 12u, cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0013u, cpu->x));
        cpu->carry = true;
        Subtract16(cpu, Read16Direct(memory, cpu, 0u));
        if (cpu->carry) {
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0013u, cpu->x), cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Read8(memory, HANDLER_PARAMETER));
            Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_SPELL_ID, 0u), A8(cpu));
            cpu->carry = false;
            Adc8(cpu, 0x12u);
            Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_EFFECT, 0u), A8(cpu));
            if (!HandlerCall(action, 0xa9c0u, HANDLER_LOAD_SPELL, true))
                return action->result;
            MessageName(action, HANDLER_SPELL_NAME, 0x95u);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, HANDLER_SPELL_MESSAGE, 0u));
            if (!HandlerCall(action, 0xa9d2u, HANDLER_BUILD_MESSAGE, true))
                return action->result;
            LoadA8(cpu, Read8(memory, HANDLER_ACTOR));
            if (cpu->negative) {
                LoadAAbsolute8(memory, cpu, HANDLER_EFFECT, 0u);
                Compare8(cpu, A8(cpu), 0x17u);
                if (cpu->zero) {
                    LoadA8(cpu, 0x36u);
                    Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_EFFECT, 0u), A8(cpu));
                }
            }
            if (!HandlerCall(action, 0xa9e8u, HANDLER_RUN_ACTION, true))
                return action->result;
            return ExecutionReturned(0x81a9ecu);
        }
        SetAccumulatorWidth(cpu, 1);
    }
    LoadX16(cpu, blocked ? 0xf1feu : 0xf20cu);
    if (!HandlerCall(action, 0xa9f7u, 0x8594e7u, true))
        return action->result;
    LoadX16(cpu, 1u);
    Write16Absolute(memory, cpu, HANDLER_MESSAGE_COUNT, cpu->x);
    if (!HandlerCall(action, 0xaa01u, 0x8595feu, true) || !HandlerWait(action))
        return action->result;
    return ExecutionReturned(0x81aa1au);
}

Lufia2ExecutionResult Lufia2BattleBeginSpellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionHandler action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!HandlerContext(cpu))
        return ExecutionHandoff(cpu, 0x81a968u);
    LoadA8(cpu, Read8(memory, HANDLER_PARAMETER));
    Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_SPELL_ID, 0u), A8(cpu));
    if (!HandlerCall(&action, 0xa96fu, HANDLER_LOAD_SPELL, true) ||
        !HandlerCall(&action, 0xa973u, 0x859510u, true))
        return action.result;
    return RunSpellAction(&action);
}

Lufia2ExecutionResult Lufia2BattleSpellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionHandler action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    return HandlerContext(cpu) ? RunSpellAction(&action)
                              : ExecutionHandoff(cpu, 0x81a977u);
}

static Lufia2ExecutionResult RunItemAction(ActionHandler *action) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, HANDLER_PARAMETER));
    Write16Absolute(memory, cpu, HANDLER_ITEM_ID, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    if (!HandlerCall(action, 0xaa39u, HANDLER_LOAD_ITEM, true))
        return action->result;
    LoadAAbsolute8(memory, cpu, HANDLER_ITEM_EFFECT, 0u);
    DecrementA8(cpu);
    Compare8(cpu, A8(cpu), 0xffu);
    if (cpu->zero)
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    Write8(memory, AbsoluteIndexedAddress(cpu, HANDLER_EFFECT, 0u), A8(cpu));
    LoadA8(cpu, Read8(memory, HANDLER_ACTOR));
    if (!cpu->negative) {
        Compare8(cpu, A8(cpu), BATTLE_ACTOR_CAPSULE);
        if (!cpu->zero && !HandlerCall(action, 0xaa53u, 0x81f15bu, true))
            return action->result;
    }
    MessageName(action, HANDLER_ITEM_NAME, 0x96u);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, HANDLER_ITEM_MESSAGE, 0u));
    if (cpu->zero)
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, HANDLER_ITEM_FALLBACK_MESSAGE, 0u));
    if (!HandlerCall(action, 0xaa6au, HANDLER_BUILD_MESSAGE, true) ||
        !HandlerCall(action, 0xaa6eu, HANDLER_RUN_ACTION, true))
        return action->result;
    return ExecutionReturned(0x81aa72u);
}

Lufia2ExecutionResult Lufia2BattleBeginItemAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionHandler action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!HandlerContext(cpu))
        return ExecutionHandoff(cpu, 0x81aa1bu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, HANDLER_PARAMETER));
    Write16Absolute(memory, cpu, HANDLER_ITEM_ID, cpu->accumulator);
    if (!HandlerCall(&action, 0xaa24u, HANDLER_LOAD_ITEM, false))
        return action.result;
    SetAccumulatorWidth(cpu, 1);
    if (!HandlerCall(&action, 0xaa2au, 0x859510u, true))
        return action.result;
    return RunItemAction(&action);
}

Lufia2ExecutionResult Lufia2BattleItemAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionHandler action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    return HandlerContext(cpu) ? RunItemAction(&action)
                              : ExecutionHandoff(cpu, 0x81aa2eu);
}
