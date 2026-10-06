#include "battle/battle_internal.h"

enum {
    SPECIAL_ACTOR = WRAM_BATTLE_ACTION_WORK,
    SPECIAL_PARAMETER = WRAM_BATTLE_ACTION_WORK + 8u,
    SPECIAL_EFFECT = WRAM_PALETTE_FADE & 0xffffu,
    SPECIAL_MESSAGE_COUNT = WRAM_BATTLE_MESSAGE_COUNT & 0xffffu,
    SPECIAL_MESSAGE_NAME = WRAM_BATTLE_MESSAGE_NAME_POINTER & 0xffffu,
    SPECIAL_MESSAGE_BANK = WRAM_BATTLE_MESSAGE_NAME_BANK & 0xffffu,
    SPECIAL_ITEM_ID = WRAM_ITEM_RECORD_ID & 0xffffu,
    SPECIAL_ITEM_EFFECT = (WRAM_RECORD_BUFFER & 0xffffu) + 0x30u,
    SPECIAL_TARGET_COUNT = 0x7e4f0au,
    SPECIAL_TARGET_INDEX = 0x0a62u,
    SPECIAL_WAIT_LIMIT = 4096u,
    SPECIAL_MESSAGE = 0x8595feu,
    SPECIAL_PLAY_EFFECT = 0x81895eu,
    SPECIAL_POPUPS = 0x81c35fu,
    SPECIAL_RUN_ACTION = 0x81b057u
};

typedef struct SpecialAction {
    BattleContext battle;
    Lufia2ExecutionResult result;
} SpecialAction;

typedef struct AttackFinish {
    uint16_t popups;
    uint16_t input;
    uint16_t low_effect;
    uint16_t high_effect;
    uint16_t return_pc;
} AttackFinish;

static bool SpecialContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->direct_page == 0u &&
        cpu->program_bank == 0x81u && cpu->data_bank == 0x97u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool SpecialCall(SpecialAction *action, uint16_t site,
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

static bool IpErrorWait(SpecialAction *action) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadA8(cpu, 30u);
    for (unsigned frame = 0u; frame < SPECIAL_WAIT_LIMIT; ++frame) {
        PushAccumulator8(memory, cpu);
        if (!SpecialCall(action, 0xab81u, BATTLE_ROUTINE_SPRITES, true))
            return false;
        LoadA8(cpu, 0xffu);
        Write8(memory, BATTLE_SPRITE_REBUILD_REQUEST, A8(cpu));
        if (!SpecialCall(action, 0xab8bu, BATTLE_ROUTINE_FRAME_INPUT, true))
            return false;
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
        if (cpu->zero)
            return true;
    }
    action->result = ExecutionHandoff(cpu, 0x81ab80u);
    return false;
}

Lufia2ExecutionResult Lufia2BattleIpAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    SpecialAction action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!SpecialContext(cpu))
        return ExecutionHandoff(cpu, 0x81ab0cu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, SPECIAL_PARAMETER));
    SetAccumulatorWidth(cpu, 1);
    if (cpu->zero)
        return ExecutionHandoff(cpu, 0x81ab6au);
    if (!SpecialCall(&action, 0xab19u, 0x859578u, true))
        return action.result;
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, SPECIAL_PARAMETER));
    if (!SpecialCall(&action, 0xab23u, 0x81f45eu, false))
        return action.result;
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x84u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, Read8(memory, SPECIAL_ACTOR));
    if (!SpecialCall(&action, 0xab32u, 0x81b2b5u, true))
        return action.result;
    LoadAAbsolute8(memory, cpu, 0x00bcu, cpu->x);
    cpu->carry = true;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 5u, cpu->y));
    if (cpu->carry) {
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x00bcu, cpu->x), A8(cpu));
        LoadAAbsolute8(memory, cpu, 2u, cpu->y);
        Write8(memory, AbsoluteIndexedAddress(cpu, SPECIAL_EFFECT, 0u), A8(cpu));
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0u, cpu->y));
        if (!SpecialCall(&action, 0xab50u, 0x81f46bu, false))
            return action.result;
        Write16Absolute(memory, cpu, SPECIAL_MESSAGE_NAME, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, 0x95u);
        Write8(memory, AbsoluteIndexedAddress(cpu, SPECIAL_MESSAGE_BANK, 0u), A8(cpu));
        LoadY16(cpu, 1u);
        if (!SpecialCall(&action, 0xab61u, 0x81fac9u, true) ||
            !SpecialCall(&action, 0xab65u, SPECIAL_RUN_ACTION, true))
            return action.result;
        return ExecutionReturned(0x81ab69u);
    }
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, 0xf21bu);
    if (!SpecialCall(&action, 0xab70u, 0x8594e7u, true))
        return action.result;
    LoadX16(cpu, 1u);
    Write16Absolute(memory, cpu, SPECIAL_MESSAGE_COUNT, cpu->x);
    if (!SpecialCall(&action, 0xab7au, SPECIAL_MESSAGE, true) || !IpErrorWait(&action))
        return action.result;
    return ExecutionReturned(0x81ab93u);
}

Lufia2ExecutionResult Lufia2BattleCapsuleAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    SpecialAction action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!SpecialContext(cpu))
        return ExecutionHandoff(cpu, 0x81ad3bu);
    LoadA8(cpu, Read8(memory, SPECIAL_ACTOR));
    if (cpu->negative)
        return ExecutionHandoff(cpu, 0x81ad8cu);
    Compare8(cpu, A8(cpu), BATTLE_ACTOR_CAPSULE);
    if (!cpu->zero)
        return ExecutionHandoff(cpu, 0x81ad8cu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, SPECIAL_PARAMETER));
    if (!SpecialCall(&action, 0xad51u, 0x81f476u, false))
        return action.result;
    TransferAToY(cpu);
    Write16Absolute(memory, cpu, SPECIAL_MESSAGE_NAME, cpu->accumulator);
    Write16Direct(memory, cpu, 0x0eu, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x97u);
    Write8(memory, AbsoluteIndexedAddress(cpu, SPECIAL_MESSAGE_BANK, 0u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x10u), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0x0eu)));
    PushAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    if (!SpecialCall(&action, 0xad6cu, 0x8595c6u, true))
        return action.result;
    LoadA8(cpu, Pull8(memory, cpu));
    if (cpu->zero) {
        LoadA8(cpu, 0xffu);
    } else {
        Compare8(cpu, A8(cpu), 0u);
        if (cpu->zero)
            LoadA8(cpu, 0xf0u);
    }
    Write8(memory, AbsoluteIndexedAddress(cpu, SPECIAL_EFFECT, 0u), A8(cpu));
    LoadY16(cpu, 2u);
    if (!SpecialCall(&action, 0xad83u, 0x81fac9u, true) ||
        !SpecialCall(&action, 0xad87u, SPECIAL_RUN_ACTION, true))
        return action.result;
    return ExecutionReturned(0x81ad8bu);
}

static Lufia2ExecutionResult FinishAlternateAttack(SpecialAction *action, AttackFinish finish) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    LoadA8(cpu, 0u);
    if (!SpecialCall(action, finish.popups, SPECIAL_POPUPS, true))
        return action->result;
    LoadA8(cpu, Read8(memory, SPECIAL_TARGET_COUNT));
    if (cpu->zero)
        return ExecutionReturned(0x81ae76u);
    if (finish.input && !SpecialCall(action, finish.input, BATTLE_ROUTINE_FRAME_INPUT, true))
        return action->result;
    LoadAAbsolute8(memory, cpu, SPECIAL_TARGET_INDEX, 0u);
    Compare8(cpu, A8(cpu), 12u);
    const bool high_target = cpu->carry;
    LoadA8(cpu, high_target ? 0xddu : 0xdcu);
    if (!SpecialCall(action, high_target ? finish.high_effect : finish.low_effect,
        SPECIAL_PLAY_EFFECT, true))
        return action->result;
    return ExecutionReturned(0x810000u | finish.return_pc);
}

static Lufia2ExecutionResult AlternateNonPartyAttack(SpecialAction *action, bool capsule) {
    const Lufia2Memory *memory = action->battle.memory;
    Lufia2CpuState *cpu = action->battle.cpu;

    if (!SpecialCall(action, capsule ? 0xaeacu : 0xae77u, 0x859532u, true))
        return action->result;
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SPECIAL_MESSAGE_COUNT, 0u));
    if (!cpu->zero && !SpecialCall(action, capsule ? 0xaeb5u : 0xae80u, SPECIAL_MESSAGE, true))
        return action->result;
    LoadA8(cpu, capsule ? 0xf0u : 0xe0u);
    if (!SpecialCall(action, capsule ? 0xaebbu : 0xae86u, SPECIAL_PLAY_EFFECT, true))
        return action->result;
    return FinishAlternateAttack(action, capsule
        ? (AttackFinish){0xaec1u, 0u, 0xaed4u, 0xaedcu, 0xaee0u}
        : (AttackFinish){0xae8cu, 0u, 0xae9fu, 0xaea7u, 0xaeabu});
}

Lufia2ExecutionResult Lufia2BattleAlternateAttackAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    SpecialAction action = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!SpecialContext(cpu))
        return ExecutionHandoff(cpu, 0x81ae0fu);
    if (!SpecialCall(&action, 0xae0fu, 0x85cd8cu, true) ||
        !SpecialCall(&action, 0xae13u, 0x85ce21u, true))
        return action.result;
    LoadA8(cpu, 3u);
    if (!SpecialCall(&action, 0xae19u, SPECIAL_POPUPS, true))
        return action.result;
    LoadA8(cpu, Read8(memory, SPECIAL_ACTOR));
    if (cpu->negative)
        return AlternateNonPartyAttack(&action, false);
    Compare8(cpu, A8(cpu), BATTLE_ACTOR_CAPSULE);
    if (cpu->zero)
        return AlternateNonPartyAttack(&action, true);
    if (!SpecialCall(&action, 0xae2au, 0x859532u, true) ||
        !SpecialCall(&action, 0xae2eu, SPECIAL_MESSAGE, true))
        return action.result;
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, SPECIAL_PARAMETER));
    if (!cpu->zero) {
        Write16Absolute(memory, cpu, SPECIAL_ITEM_ID, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        if (!SpecialCall(&action, 0xae3fu, 0x81f1c5u, true))
            return action.result;
        LoadAAbsolute8(memory, cpu, SPECIAL_ITEM_EFFECT, 0u);
        AslA8(cpu);
    } else {
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, 0x0eu);
    }
    if (!SpecialCall(&action, 0xae4du, SPECIAL_PLAY_EFFECT, true))
        return action.result;
    return FinishAlternateAttack(&action,
        (AttackFinish){0xae53u, 0xae5du, 0xae6au, 0xae72u, 0xae76u});
}
