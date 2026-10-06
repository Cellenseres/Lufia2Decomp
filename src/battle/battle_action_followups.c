#include "battle/battle_internal.h"

enum {
    FOLLOWUP_ACTOR = WRAM_BATTLE_ACTION_WORK,
    FOLLOWUP_TARGET_MASK = WRAM_BATTLE_ACTION_WORK + 2u,
    FOLLOWUP_TYPE = WRAM_BATTLE_ACTION_WORK + 6u,
    FOLLOWUP_PARAMETER = WRAM_BATTLE_ACTION_WORK + 8u,
    FOLLOWUP_STATE = WRAM_BATTLE_ACTION_WORK + 10u,
    FOLLOWUP_EFFECT = WRAM_PALETTE_FADE & 0xffffu,
    FOLLOWUP_MESSAGE_COUNT = WRAM_BATTLE_MESSAGE_COUNT & 0xffffu,
    FOLLOWUP_SPELL_ID = WRAM_MENU_SPELL_RECORD_ID & 0xffffu,
    FOLLOWUP_SPELL_NAME = WRAM_SPELL_RECORD_POINTER & 0xffffu,
    FOLLOWUP_MESSAGE_NAME = WRAM_BATTLE_MESSAGE_NAME_POINTER & 0xffffu,
    FOLLOWUP_MESSAGE_BANK = WRAM_BATTLE_MESSAGE_NAME_BANK & 0xffffu,
    FOLLOWUP_SPELL_MESSAGE = (WRAM_RECORD_BUFFER & 0xffffu) + 0x12u,
    FOLLOWUP_WAIT_LIMIT = 4096u,
    FOLLOWUP_DISPLAY_MESSAGE = 0x8595feu,
    FOLLOWUP_PREPARE_EFFECT = 0x81afc4u,
    FOLLOWUP_PLAY_EFFECT = 0x81895eu,
    FOLLOWUP_POPUPS = 0x81c35fu,
    FOLLOWUP_TARGET_RECORD = 0x81b2b5u,
    FOLLOWUP_RUN_ACTION = 0x81b057u
};

typedef struct ActionFollowup {
    BattleContext battle;
    Lufia2ExecutionResult result;
} ActionFollowup;

typedef struct ActionWait {
    uint8_t frames;
    uint16_t start;
    uint16_t sprites;
    uint16_t input;
} ActionWait;

static bool FollowupContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->direct_page == 0u &&
        cpu->program_bank == 0x81u && cpu->data_bank == 0x97u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool FollowupCall(ActionFollowup *action, uint16_t site,
                         uint32_t target, uint8_t frame) {
    Lufia2CpuState *cpu = action->battle.cpu;

    if (!BattleCall(&action->battle, site, target, frame)) {
        action->result = BattleChildUnwound(&action->battle);
        return false;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->decimal || cpu->direct_page != 0u) {
        action->result = ExecutionHandoff(cpu,
            0x810000u | (uint16_t)(site + (frame == 2u ? 3u : 4u)));
        return false;
    }
    return true;
}

static bool FollowupWait(ActionFollowup *action, ActionWait wait) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadA8(cpu, wait.frames);
    for (unsigned frame = 0u; frame < FOLLOWUP_WAIT_LIMIT; ++frame) {
        PushAccumulator8(memory, cpu);
        if (!FollowupCall(action, wait.sprites, BATTLE_ROUTINE_SPRITES, 3u))
            return false;
        LoadA8(cpu, 0xffu);
        Write8(memory, BATTLE_SPRITE_REBUILD_REQUEST, A8(cpu));
        if (!FollowupCall(action, wait.input, BATTLE_ROUTINE_FRAME_INPUT, 3u))
            return false;
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
        if (cpu->zero)
            return true;
    }
    action->result = ExecutionHandoff(cpu, 0x810000u | wait.start);
    return false;
}

Lufia2ExecutionResult Lufia2BattleDefendAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionFollowup action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!FollowupContext(cpu))
        return ExecutionHandoff(cpu, 0x81aad0u);
    LoadA8(cpu, Read8(memory, FOLLOWUP_ACTOR));
    if (!FollowupCall(&action, 0xaad4u, FOLLOWUP_TARGET_RECORD, 3u))
        return action.result;
    LoadAAbsolute8(memory, cpu, 0x0010u, cpu->x);
    Or8(cpu, 1u);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0010u, cpu->x), A8(cpu));
    if (!FollowupCall(&action, 0xaae0u, 0x859150u, 3u))
        return action.result;
    LoadA8(cpu, 4u);
    if (!FollowupCall(&action, 0xaae6u, 0x859173u, 3u) ||
        !FollowupCall(&action, 0xaaeau, FOLLOWUP_DISPLAY_MESSAGE, 3u) ||
        !FollowupCall(&action, 0xaaeeu, FOLLOWUP_PREPARE_EFFECT, 2u) ||
        !FollowupWait(&action, (ActionWait){30u, 0xaaf3u, 0xaaf4u, 0xaafeu}))
        return action.result;
    return ExecutionReturned(0x81ab06u);
}

Lufia2ExecutionResult Lufia2BattleContinueAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionFollowup action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!FollowupContext(cpu))
        return ExecutionHandoff(cpu, 0x81ab07u);
    if (!FollowupCall(&action, 0xab07u, FOLLOWUP_RUN_ACTION, 3u))
        return action.result;
    return ExecutionReturned(0x81ab0bu);
}

static Lufia2ExecutionResult RunUncostedSpell(ActionFollowup *action) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadA8(cpu, Read8(memory, FOLLOWUP_ACTOR));
    if (!FollowupCall(action, 0xaf8du, FOLLOWUP_TARGET_RECORD, 3u))
        return action->result;
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x01dcu);
    Write16Long(memory, FOLLOWUP_ACTOR + 12u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, FOLLOWUP_PARAMETER));
    Write8(memory, AbsoluteIndexedAddress(cpu, FOLLOWUP_SPELL_ID, 0u), A8(cpu));
    cpu->carry = false;
    Adc8(cpu, 0x12u);
    Write8(memory, AbsoluteIndexedAddress(cpu, FOLLOWUP_EFFECT, 0u), A8(cpu));
    if (!FollowupCall(action, 0xafa9u, 0x81f414u, 3u))
        return action->result;
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, FOLLOWUP_SPELL_NAME, 0u));
    Write16Absolute(memory, cpu, FOLLOWUP_MESSAGE_NAME, cpu->y);
    LoadA8(cpu, 0x95u);
    Write8(memory, AbsoluteIndexedAddress(cpu, FOLLOWUP_MESSAGE_BANK, 0u), A8(cpu));
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, FOLLOWUP_SPELL_MESSAGE, 0u));
    if (!FollowupCall(action, 0xafbbu, 0x81fac9u, 3u) ||
        !FollowupCall(action, 0xafbfu, FOLLOWUP_RUN_ACTION, 3u))
        return action->result;
    return ExecutionReturned(0x81afc3u);
}

Lufia2ExecutionResult Lufia2BattleBeginUncostedSpellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionFollowup action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!FollowupContext(cpu))
        return ExecutionHandoff(cpu, 0x81af7au);
    LoadA8(cpu, Read8(memory, FOLLOWUP_PARAMETER));
    Write8(memory, AbsoluteIndexedAddress(cpu, FOLLOWUP_SPELL_ID, 0u), A8(cpu));
    if (!FollowupCall(&action, 0xaf81u, 0x81f414u, 3u) ||
        !FollowupCall(&action, 0xaf85u, 0x859510u, 3u))
        return action.result;
    return RunUncostedSpell(&action);
}

Lufia2ExecutionResult Lufia2BattleUncostedSpellAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionFollowup action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    return FollowupContext(cpu) ? RunUncostedSpell(&action)
                               : ExecutionHandoff(cpu, 0x81af89u);
}

Lufia2ExecutionResult Lufia2BattleFollowupAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionFollowup action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!FollowupContext(cpu))
        return ExecutionHandoff(cpu, 0x81af37u);
    LoadA8(cpu, Read8(memory, FOLLOWUP_ACTOR));
    if (!FollowupCall(&action, 0xaf3bu, FOLLOWUP_TARGET_RECORD, 3u))
        return action.result;
    LoadAAbsolute8(memory, cpu, BATTLE_BATTLER_STATUS, cpu->x);
    BitImmediate8(cpu, BATTLE_STATUS_TURN_SELECTED);
    if (cpu->zero)
        return ExecutionReturned(0x81af46u);
    LoadA8(cpu, Read8(memory, FOLLOWUP_ACTOR));
    bool party_weapon = !cpu->negative;
    if (party_weapon) {
        Compare8(cpu, A8(cpu), BATTLE_ACTOR_CAPSULE);
        party_weapon = !cpu->zero;
    }
    SetAccumulatorWidth(cpu, 0);
    if (party_weapon) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0066u, cpu->x));
        And16(cpu, 0x01ffu);
    } else {
        TransferDirectToA(cpu);
    }
    Write16Long(memory, FOLLOWUP_PARAMETER, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 2u);
    if (!FollowupCall(&action, 0xaf66u, 0x808299u, 3u))
        return action.result;
    LsrA8(cpu);
    TransferDirectToA(cpu);
    RorA8(cpu);
    Write8(memory, FOLLOWUP_TARGET_MASK, A8(cpu));
    LoadA8(cpu, BATTLE_ACTION_ATTACK);
    Write8(memory, FOLLOWUP_TYPE, A8(cpu));
    return ExecutionHandoff(cpu, 0x81a832u);
}

Lufia2ExecutionResult Lufia2BattleWaitAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionFollowup action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!FollowupContext(cpu))
        return ExecutionHandoff(cpu, 0x81ad8du);
    LoadA8(cpu, Read8(memory, FOLLOWUP_STATE));
    if (cpu->zero)
        return ExecutionReturned(0x81ae0eu);
    if (!FollowupCall(&action, 0xadbeu, 0x85cd8cu, 3u) ||
        !FollowupCall(&action, 0xadc2u, 0x859532u, 3u))
        return action.result;
    LoadX16(cpu, 1u);
    Write16Absolute(memory, cpu, FOLLOWUP_MESSAGE_COUNT, cpu->x);
    if (!FollowupCall(&action, 0xadccu, FOLLOWUP_DISPLAY_MESSAGE, 3u) ||
        !FollowupCall(&action, 0xadd0u, FOLLOWUP_PREPARE_EFFECT, 2u) ||
        !FollowupWait(&action, (ActionWait){30u, 0xadd5u, 0xadd6u, 0xade0u}))
        return action.result;
    LoadX16(cpu, 0xf2b8u);
    if (!FollowupCall(&action, 0xadebu, 0x8594e7u, 3u))
        return action.result;
    LoadX16(cpu, 1u);
    Write16Absolute(memory, cpu, FOLLOWUP_MESSAGE_COUNT, cpu->x);
    if (!FollowupCall(&action, 0xadf5u, FOLLOWUP_DISPLAY_MESSAGE, 3u) ||
        !FollowupWait(&action, (ActionWait){30u, 0xadfbu, 0xadfcu, 0xae06u}))
        return action.result;
    return ExecutionReturned(0x81ae0eu);
}

Lufia2ExecutionResult Lufia2BattleWaitLongAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionFollowup action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!FollowupContext(cpu))
        return ExecutionHandoff(cpu, 0x81aee1u);
    LoadA8(cpu, Read8(memory, FOLLOWUP_STATE));
    if (cpu->zero)
        return ExecutionReturned(0x81af36u);
    if (!FollowupCall(&action, 0xaf0cu, 0x85cd8cu, 3u) ||
        !FollowupCall(&action, 0xaf10u, 0x859532u, 3u))
        return action.result;
    LoadX16(cpu, 1u);
    Write16Absolute(memory, cpu, FOLLOWUP_MESSAGE_COUNT, cpu->x);
    if (!FollowupCall(&action, 0xaf1au, FOLLOWUP_DISPLAY_MESSAGE, 3u) ||
        !FollowupCall(&action, 0xaf1eu, FOLLOWUP_PREPARE_EFFECT, 2u) ||
        !FollowupWait(&action, (ActionWait){60u, 0xaf23u, 0xaf24u, 0xaf2eu}))
        return action.result;
    return ExecutionReturned(0x81af36u);
}

Lufia2ExecutionResult Lufia2BattleRepeatActionEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionFollowup action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!FollowupContext(cpu))
        return ExecutionHandoff(cpu, 0x81aa73u);
    SetAccumulatorWidth(cpu, 1);
    if (!FollowupCall(&action, 0xaa75u, FOLLOWUP_DISPLAY_MESSAGE, 3u) ||
        !FollowupCall(&action, 0xaa79u, FOLLOWUP_PREPARE_EFFECT, 2u) ||
        !FollowupCall(&action, 0xaa7cu, 0x81b139u, 3u) ||
        !FollowupCall(&action, 0xaa80u, 0x81b174u, 3u))
        return action.result;
    LoadAAbsolute8(memory, cpu, WRAM_BATTLE_SPRITE_BUILD_MODE & 0xffffu, 0u);
    if (!FollowupCall(&action, 0xaa87u, 0x858a39u, 3u))
        return action.result;
    LoadA8(cpu, 0xffu);
    Write8(memory, BATTLE_SPRITE_REBUILD_REQUEST, A8(cpu));
    if (!FollowupCall(&action, 0xaa91u, BATTLE_ROUTINE_FRAME_INPUT, 3u))
        return action.result;
    LoadA8(cpu, 1u);
    if (!FollowupCall(&action, 0xaa97u, FOLLOWUP_POPUPS, 3u))
        return action.result;
    LoadA8(cpu, Read8(memory, 0x7e4f0au));
    if (!cpu->zero) {
        LoadA8(cpu, 0x2au);
        if (!FollowupCall(&action, 0xaaa3u, FOLLOWUP_PLAY_EFFECT, 3u))
            return action.result;
    }
    LoadA8(cpu, 3u);
    if (!FollowupCall(&action, 0xaaa9u, FOLLOWUP_POPUPS, 3u))
        return action.result;
    LoadAAbsolute8(memory, cpu, FOLLOWUP_EFFECT, 0u);
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        if (!FollowupCall(&action, 0xaab4u, FOLLOWUP_PLAY_EFFECT, 3u))
            return action.result;
    } else if (!FollowupWait(&action, (ActionWait){60u, 0xaabcu, 0xaabdu, 0xaac7u})) {
        return action.result;
    }
    return ExecutionReturned(0x81aacfu);
}
