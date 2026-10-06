#include "battle/battle_internal.h"
#include "core/wram_view.h"

enum {
    ACTION_EFFECT_ID = WRAM_PALETTE_FADE & 0xffffu,
    ACTION_TARGET_COUNT = WRAM_BATTLE_EFFECT_TARGET_COUNT,
    ACTION_FRAME_PAUSE = 60u,
    ACTION_FRAME_MARKER = WRAM_BATTLE_EFFECT_FRAME_MARKER & 0xffffu,
    ACTION_WAIT_LIMIT = 4096u,
    ACTION_ROUTINE_POPUPS = 0x81c35fu,
    ACTION_ROUTINE_EFFECT = 0x81895eu,
    ACTION_ROUTINE_PORTRAITS = 0x81bae8u,
    ACTION_ROUTINE_QUEUE_TURN = 0x859dd4u
};

typedef struct ActionEffects {
    BattleContext battle;
    Lufia2ExecutionResult result;
} ActionEffects;

static bool ActionEffectsContext(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && cpu->direct_page == 0u &&
        cpu->program_bank == 0x81u && cpu->data_bank == 0x97u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static bool ActionEffectCall(ActionEffects *effects, uint16_t site,
                             uint32_t target, bool wide) {
    Lufia2CpuState *cpu = effects->battle.cpu;

    if (!BattleCall(&effects->battle, site, target, 3u)) {
        effects->result = BattleChildUnwound(&effects->battle);
        return false;
    }
    if (cpu->direct_page != 0u || cpu->index_is_8_bit || cpu->decimal ||
        cpu->accumulator_is_8_bit == wide) {
        effects->result = ExecutionHandoff(cpu, 0x810000u | (uint16_t)(site + 4u));
        return false;
    }
    return true;
}

static bool PauseActionEffects(ActionEffects *effects) {
    const Lufia2Memory *memory = effects->battle.memory;
    Lufia2CpuState *cpu = effects->battle.cpu;

    LoadA8(cpu, ACTION_FRAME_PAUSE);
    for (unsigned frame = 0u; frame < ACTION_WAIT_LIMIT; ++frame) {
        PushAccumulator8(memory, cpu);
        if (!ActionEffectCall(effects, 0xb161u, BATTLE_ROUTINE_SPRITES, false))
            return false;
        LoadA8(cpu, 0xffu);
        Write8(memory, ACTION_FRAME_MARKER, A8(cpu));
        if (!ActionEffectCall(effects, 0xb16bu, BATTLE_ROUTINE_FRAME_INPUT, false))
            return false;
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
        if (cpu->zero)
            return true;
    }
    effects->result = ExecutionHandoff(cpu, 0x81b160u);
    return false;
}

Lufia2ExecutionResult Lufia2BattleRunActionEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionEffects effects = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!ActionEffectsContext(cpu))
        return ExecutionHandoff(cpu, 0x81b139u);
    LoadA8(cpu, 1u);
    if (!ActionEffectCall(&effects, 0xb13bu, ACTION_ROUTINE_POPUPS, false))
        return effects.result;
    LoadA8(cpu, Read8(memory, ACTION_TARGET_COUNT));
    if (!cpu->zero) {
        LoadA8(cpu, 0x9au);
        if (!ActionEffectCall(&effects, 0xb147u, ACTION_ROUTINE_EFFECT, false))
            return effects.result;
    }
    LoadA8(cpu, 3u);
    if (!ActionEffectCall(&effects, 0xb14du, ACTION_ROUTINE_POPUPS, false))
        return effects.result;
    LoadAAbsolute8(memory, cpu, ACTION_EFFECT_ID, 0u);
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        if (!ActionEffectCall(&effects, 0xb158u, ACTION_ROUTINE_EFFECT, false))
            return effects.result;
    } else if (!PauseActionEffects(&effects)) {
        return effects.result;
    }
    return ExecutionReturned(0x81b173u);
}

Lufia2ExecutionResult Lufia2BattleFinishActionEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    ActionEffects effects = {
        BattleContextCreate(memory, cpu, child, child_context, 0x81u), {0}
    };

    if (!ActionEffectsContext(cpu))
        return ExecutionHandoff(cpu, 0x81b174u);
    LoadA8(cpu, 0u);
    if (!ActionEffectCall(&effects, 0xb176u, ACTION_ROUTINE_POPUPS, false))
        return effects.result;
    LoadA8(cpu, Read8(memory, ACTION_TARGET_COUNT));
    if (!cpu->zero) {
        LoadA8(cpu, 0xdcu);
        if (!ActionEffectCall(&effects, 0xb182u, ACTION_ROUTINE_EFFECT, false))
            return effects.result;
    }
    if (!ActionEffectCall(&effects, 0xb186u, 0x85948cu, false) ||
        !ActionEffectCall(&effects, 0xb18au, ACTION_ROUTINE_PORTRAITS, false) ||
        !ActionEffectCall(&effects, 0xb18eu, ACTION_ROUTINE_QUEUE_TURN, false) ||
        !ActionEffectCall(&effects, 0xb192u, BATTLE_ROUTINE_FRAME_INPUT, false))
        return effects.result;
    SetAccumulatorWidth(cpu, 0);
    if (!ActionEffectCall(&effects, 0xb198u, BATTLE_ROUTINE_QUEUE_STATUS_SPRITES, true))
        return effects.result;
    SetAccumulatorWidth(cpu, 1);
    if (!ActionEffectCall(&effects, 0xb19eu, BATTLE_ROUTINE_FRAME_INPUT, false))
        return effects.result;
    return ExecutionReturned(0x81b1a2u);
}
