#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    ACTION_SPELL_POINTER = 0x2au,
    ACTION_SWAP_VALUE = 0x54u,
    ACTION_TARGET_POSITION = 0x00u,
    ACTION_REFRESH = 0x74u,
    ACTION_ITEM_USE_CONTEXT = WRAM_BATTLE_PARTY_IDS + 1u,
    ACTION_EFFECT_SPRITE = MENU_SPRITE_ACTIVE + 10u,
    ACTION_TARGET_FLAGS = MENU_SPRITE_ACTIVE + 12u,
    ACTION_RECORD_FLAGS = WRAM_RECORD_BUFFER + 0x0du,
    ACTION_ITEM_EFFECT = WRAM_RECORD_BUFFER + 0x0fu,
    ACTION_SPELL_FLAGS = WRAM_RECORD_BUFFER + 9u,
    ACTION_SPELL_EFFECT = WRAM_RECORD_BUFFER + 12u,
    ACTION_SPELL_COST = WRAM_RECORD_BUFFER + 13u,
    ACTION_SPELL_TARGET_POSITIONS = 0x8ed8e3u,
    ACTION_SOUND_CONFIRM = 2u,
    ACTION_SOUND_APPLY = 0x76u,
    ACTION_SELECTION_EMPTY = 0x00ffu
};

static Lufia2ExecutionResult ClearListSelection(MenuActionCall *call, bool feedback) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    if (feedback) {
        if (!MenuActionChild(call, 0x82ab5bu, 0x82ab46u, 2u, 1u) ||
            !MenuActionChild(call, 0x82ab5eu, 0x8297e3u, 2u, 1u))
            return call->result;
        return ExecutionReturned(0x82ab61u);
    }
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 8u);
    OpLdy(cpu, 9u);
    if (!MenuActionChild(call, 0x82ab4eu, 0x8289bcu, 2u, 1u))
        return call->result;
    OpLdx(cpu, ACTION_SELECTION_EMPTY);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SECOND_SELECTION), cpu->x);
    return ExecutionReturned(0x82ab5au);
}

Lufia2ExecutionResult Lufia2MenuClearListSelection(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82ab46u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    return ClearListSelection(&call, false);
}

Lufia2ExecutionResult Lufia2MenuClearSelectionWithFeedback(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu,
                                                           Lufia2PushedChildCall child,
                                                           void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82ab5bu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    return ClearListSelection(&call, true);
}

Lufia2ExecutionResult Lufia2MenuDispatchListAction(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82aaffu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MODE));
    OpCmpValue(cpu, 0u);
    unsigned mode = 0u;
    if (!cpu->zero) {
        OpCmpValue(cpu, 2u);
        if (cpu->zero) {
            mode = 2u;
        } else {
            OpCmpValue(cpu, 4u);
            if (cpu->zero) {
                mode = 4u;
            } else {
                OpCmpValue(cpu, 0xffu);
                if (cpu->zero)
                    mode = 0xffu;
            }
        }
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SECOND_SELECTION));
    uint32_t site, target;
    bool use_result = false;
    if (!cpu->zero) {
        site = mode == 2u ? 0x82ab75u : 0x82ab62u;
        target = mode == 2u ? 0x82b42du : 0x82ab89u;
    } else if (mode == 2u) {
        site = 0x82ab7au;
        target = 0x82b45bu;
        use_result = true;
    } else if (mode == 4u) {
        cpu->carry = 0u;
        site = 0x82ab82u;
        target = 0x82c683u;
        use_result = true;
    } else {
        OpLda(memory, cpu,
              OpAbs(cpu, mode == 0xffu ? WRAM_MENU_SPELL_WINDOW_STATE_2_ALT
                                       : WRAM_MENU_SPELL_WINDOW_STATE_2));
        if (mode == 0xffu)
            OpCmpValue(cpu, 4u);
        bool discard = mode == 0xffu ? cpu->zero : !cpu->zero;
        site = discard ? 0x82ab67u : 0x82ab6eu;
        target = discard ? 0x82ab9au : 0x82abb1u;
        use_result = true;
    }
    if (!MenuActionChild(&call, site, target, 2u, 1u))
        return call.result;
    return ClearListSelection(&call, use_result && cpu->carry);
}

Lufia2ExecutionResult Lufia2MenuSwapInventorySelection(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82ab89u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, ACTION_SOUND_CONFIRM);
    if (!MenuActionChild(&call, 0x82ab8bu, 0x80953bu, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    if (!MenuActionChild(&call, 0x82ab91u, 0x82fc08u, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    if (!MenuActionChild(&call, 0x82ab96u, 0x82ac84u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82ab99u);
}

Lufia2ExecutionResult Lufia2MenuDiscardInventorySelection(const Lufia2Memory *memory,
                                                          Lufia2CpuState *cpu,
                                                          Lufia2PushedChildCall child,
                                                          void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82ab9au);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    if (!MenuActionChild(&call, 0x82ab9cu, 0x82fc1fu, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    if (cpu->carry)
        return ExecutionReturned(0x82abb0u);
    OpLoadA(cpu, ACTION_SOUND_CONFIRM);
    if (!MenuActionChild(&call, 0x82aba5u, 0x80953bu, 3u, 1u) ||
        !MenuActionChild(&call, 0x82aba9u, 0x829e2fu, 2u, 1u) ||
        !MenuActionChild(&call, 0x82abacu, 0x82ac84u, 2u, 1u))
        return call.result;
    cpu->carry = 0u;
    return ExecutionReturned(0x82abb0u);
}

Lufia2ExecutionResult Lufia2MenuSwapSpellSelection(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b42du);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, ACTION_SOUND_CONFIRM);
    if (!MenuActionChild(&call, 0x82b42fu, 0x80953bu, 3u, 1u))
        return call.result;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, ACTION_SPELL_POINTER));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SECOND_SELECTION));
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, ACTION_SPELL_POINTER));
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpSta(memory, cpu, OpDp(cpu, ACTION_SWAP_VALUE));
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, ACTION_SWAP_VALUE));
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    if (!MenuActionChild(&call, 0x82b457u, 0x82ac84u, 2u, 1u))
        return call.result;
    return ExecutionReturned(0x82b45au);
}

Lufia2ExecutionResult Lufia2MenuCountSpellTargets(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b564u);
    OpLdx(cpu, 4u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    do {
        cpu->carry = 0u;
        AslAbsolute8(memory, cpu, WRAM_MENU_EFFECT_TARGET_MASK);
        OpLda(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS - 1u));
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK), 1u);
        OpDex(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    OpLoadA(cpu, 0u);
    do {
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS));
        OpInx(cpu);
        OpCpx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT)));
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x82b589u);
}

Lufia2ExecutionResult Lufia2MenuFillSpellTargets(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b58au);
    OpSepWidths(cpu, 0x10u);
    OpLdx(cpu, 0u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS));
        OpInx(cpu);
        OpCpx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT)));
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x82b599u);
}

Lufia2ExecutionResult Lufia2MenuPrepareSpellTargets(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b59au);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLdx(cpu, 0u);
    OpLdy(cpu, 12u);
    OpWriteX(memory, cpu, OpDp(cpu, ACTION_TARGET_POSITION), cpu->x);
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, ACTION_TARGET_POSITION)));
        OpLda(memory, cpu, OpLongX(cpu, ACTION_SPELL_TARGET_POSITIONS));
        OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_X_LOW));
        OpLda(memory, cpu, OpLongX(cpu, ACTION_SPELL_TARGET_POSITIONS + 1u));
        OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_Y_LOW));
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_X_HIGH));
        OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_Y_HIGH));
        OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_ACTIVE));
        OpTyx(cpu);
        OpLoadA(cpu, 6u);
        if (!MenuActionChild(&call, 0x82b5c0u, 0x868cf5u, 3u, 1u))
            return call.result;
        OpStepMem(memory, cpu, OpDp(cpu, ACTION_TARGET_POSITION), 1);
        OpStepMem(memory, cpu, OpDp(cpu, ACTION_TARGET_POSITION), 1);
        OpIny(cpu);
        OpCpy(cpu, 16u);
    } while (!cpu->zero);
    return ExecutionReturned(0x82b5ceu);
}

Lufia2ExecutionResult Lufia2MenuReadSelectedSpell(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b5cfu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_SELECTION));
    OpLsrA(cpu);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu,
          AbsoluteIndexedAddress(cpu, Read16Direct(memory, cpu, ACTION_SPELL_POINTER),
                                 cpu->y));
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero) {
        cpu->carry = 1u;
        return ExecutionReturned(0x82b5ecu);
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_RECORD_ID));
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    if (!MenuActionChild(&call, 0x82b5e3u, 0x81f414u, 3u, 1u))
        return call.result;
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    cpu->carry = 0u;
    return ExecutionReturned(0x82b5eau);
}

static Lufia2ExecutionResult RejectUse(Lufia2CpuState *cpu, uint32_t exit) {
    cpu->carry = 1u;
    return ExecutionReturned(exit);
}

static Lufia2ExecutionResult FinishEffectUse(MenuActionCall *call, uint32_t site,
                                             uint32_t exit) {
    OpStz(call->memory, call->cpu, OpAbs(call->cpu, ACTION_EFFECT_SPRITE));
    OpLoadA(call->cpu, 0xffu);
    OpSta(call->memory, call->cpu, OpAbs(call->cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    if (!MenuActionChild(call, site, 0x829588u, 2u, 1u))
        return call->result;
    call->cpu->carry = 0u;
    return ExecutionReturned(exit);
}

static bool SelectItemEffectTarget(MenuActionCall *call, uint32_t site) {
    OpStz(call->memory, call->cpu, OpAbs(call->cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    OpLdx(call->cpu,
          OpReadX(call->memory, call->cpu, OpAbs(call->cpu, WRAM_MENU_LIST_INDEX)));
    return MenuActionChild(call, site, 0x82ae64u, 2u, 1u);
}

Lufia2ExecutionResult Lufia2MenuUseSelectedItem(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82abb1u);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    if (!MenuActionChild(&call, 0x82abb3u, 0x82fbeeu, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    if (cpu->carry)
        return RejectUse(cpu, 0x82abd4u);
    OpLda(memory, cpu, OpAbs(cpu, ACTION_ITEM_USE_CONTEXT));
    OpCmpValue(cpu, 2u);
    bool alternate_context = cpu->zero;
    OpLda(memory, cpu, OpAbs(cpu, ACTION_RECORD_FLAGS));
    OpBitValue(cpu, alternate_context ? 1u : 0x40u);
    if (cpu->zero)
        return RejectUse(cpu, 0x82abd4u);
    OpLoadA(cpu, ACTION_SOUND_CONFIRM);
    if (!MenuActionChild(&call, 0x82abd7u, 0x80953bu, 3u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, ACTION_ITEM_EFFECT));
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        if (!MenuActionChild(&call, 0x82abe2u, 0x82ae6fu, 2u, 1u))
            return call.result;
        if (cpu->carry)
            return RejectUse(cpu, 0x82abd4u);
        return FinishEffectUse(&call, 0x82ac7fu, 0x82ac83u);
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MODE));
    OpCmpValue(cpu, 0xffu);
    if (cpu->zero) {
        OpLoadA(cpu, 6u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
        if (!MenuActionChild(&call, 0x82abf6u, 0x829df3u, 2u, 1u))
            return call.result;
    } else {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
        if (!MenuActionChild(&call, 0x82ac00u, 0x829c7du, 2u, 1u))
            return call.result;
    }
    bool reload_item = false;
    for (;;) {
        if (reload_item) {
            OpRepWidths(cpu, 0x20u);
            if (!MenuActionChild(&call, 0x82ac07u, 0x82fbeeu, 2u, 0u))
                return call.result;
            OpSepWidths(cpu, 0x20u);
            if (cpu->carry) {
                if (!MenuActionChild(&call, 0x82ac0eu, 0x829380u, 2u, 1u))
                    return call.result;
                return FinishEffectUse(&call, 0x82ac7fu, 0x82ac83u);
            }
        }
        reload_item = false;
        if (!MenuActionChild(&call, 0x82ac13u, 0x829246u, 2u, 1u))
            return call.result;
        if (cpu->carry)
            return FinishEffectUse(&call, 0x82ac7fu, 0x82ac83u);
        if (!SelectItemEffectTarget(&call, 0x82ac1eu) ||
            !MenuActionChild(&call, 0x82ac21u, 0x81efdfu, 3u, 1u))
            return call.result;
        if (cpu->carry) {
            if (!MenuActionChild(&call, 0x82ac27u, 0x82b2c5u, 2u, 1u))
                return call.result;
            if (cpu->carry) {
                if (!MenuActionChild(&call, 0x82ac2cu, 0x8297e3u, 2u, 1u))
                    return call.result;
                continue;
            }
        }
        OpLoadA(cpu, ACTION_SOUND_APPLY);
        if (!MenuActionChild(&call, 0x82ac33u, 0x80953bu, 3u, 1u) ||
            !SelectItemEffectTarget(&call, 0x82ac3du) ||
            !MenuActionChild(&call, 0x82ac40u, 0x81ef93u, 3u, 1u) ||
            !MenuActionChild(&call, 0x82ac44u, 0x81f034u, 3u, 1u) ||
            !SelectItemEffectTarget(&call, 0x82ac4eu) ||
            !MenuActionChild(&call, 0x82ac51u, 0x82999bu, 2u, 1u))
            return call.result;
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MODE));
        OpCmpValue(cpu, 0xffu);
        OpLoadA(cpu, cpu->zero ? 4u : 1u);
        if (!MenuActionChild(&call, 0x82ac61u, 0x82a318u, 2u, 1u) ||
            !MenuActionChild(&call, 0x82ac64u, 0x829e2fu, 2u, 1u) ||
            !MenuActionChild(&call, 0x82ac67u, 0x868b55u, 3u, 1u) ||
            !MenuActionChild(&call, 0x82ac6bu, 0x82ac84u, 2u, 1u) ||
            !MenuActionChild(&call, 0x82ac6eu, 0x829588u, 2u, 1u) ||
            !MenuActionChild(&call, 0x82ac71u, 0x8295bfu, 2u, 1u))
            return call.result;
        reload_item = true;
    }
}

static Lufia2ExecutionResult CancelSpellUse(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0042u);
    OpLdx(cpu, 0x1e01u);
    if (!MenuActionChild(call, 0x82b534u, 0x8283ebu, 2u, 0u))
        return call->result;
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, ACTION_REFRESH), 1u);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0u);
    if (!MenuActionChild(call, 0x82b540u, 0x82b58au, 2u, 1u))
        return call->result;
    OpLoadA(cpu, 0u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    OpTxy(cpu);
    OpDey(cpu);
    if (!MenuActionChild(call, 0x82b54au, 0x8289fau, 2u, 1u))
        return call->result;
    OpLoadA(cpu, 3u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_CURSOR_SLOT)));
    OpTxy(cpu);
    OpDey(cpu);
    if (!MenuActionChild(call, 0x82b554u, 0x8289bcu, 2u, 1u))
        return call->result;
    return FinishEffectUse(call, 0x82b55fu, 0x82b563u);
}

Lufia2ExecutionResult Lufia2MenuUseSelectedSpell(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82b45bu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    if (!MenuActionChild(&call, 0x82b45bu, 0x82b5cfu, 2u, 1u))
        return call.result;
    if (cpu->carry)
        return RejectUse(cpu, 0x82b480u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, ACTION_SPELL_COST));
    OpAndValue(cpu, 0x00ffu);
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_SPELL_MEMBER_MP));
    OpSepWidths(cpu, 0x20u);
    if (!cpu->zero && cpu->carry)
        return RejectUse(cpu, 0x82b480u);
    if (!MenuActionChild(&call, 0x82b473u, 0x82b59au, 2u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, ACTION_SPELL_FLAGS));
    OpBitValue(cpu, 0x40u);
    if (cpu->zero)
        return RejectUse(cpu, 0x82b480u);
    OpLoadA(cpu, ACTION_SOUND_CONFIRM);
    if (!MenuActionChild(&call, 0x82b483u, 0x80953bu, 3u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, ACTION_SPELL_EFFECT));
    OpCmpValue(cpu, 0u);
    if (cpu->zero) {
        if (!MenuActionChild(&call, 0x82b48eu, 0x82af00u, 2u, 1u))
            return call.result;
        if (cpu->carry)
            return RejectUse(cpu, 0x82b480u);
        return FinishEffectUse(&call, 0x82b55fu, 0x82b563u);
    }
    if (!MenuActionChild(&call, 0x82b496u, 0x829c7du, 2u, 1u))
        return call.result;
    bool reload_spell = true;
    for (;;) {
        if (reload_spell && !MenuActionChild(&call, 0x82b499u, 0x82b5cfu, 2u, 1u))
            return call.result;
        reload_spell = false;
        OpLoadA(cpu, 0x18u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E14BF));
        if (!MenuActionChild(&call, 0x82b4a1u, 0x829246u, 2u, 1u))
            return call.result;
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_INDEX)));
        if (cpu->carry) {
            OpLda(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS));
            if (cpu->zero)
                return CancelSpellUse(&call);
            OpStz(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS));
            continue;
        }
        OpCmpValue(cpu, 0u);
        if (!cpu->zero) {
            OpCmpValue(cpu, 1u);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS));
                OpLoadA(cpu, OpA(cpu) ^ 1u);
                if (!MenuActionChild(&call, 0x82b4c1u, 0x82b58au, 2u, 1u))
                    return call.result;
                continue;
            }
            OpCmpValue(cpu, 2u);
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS));
                if (cpu->zero) {
                    OpLoadA(cpu, 1u);
                    OpSta(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS));
                    continue;
                }
            } else {
                OpLoadA(cpu, 1u);
                OpSta(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS));
            }
        } else {
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, OpAbsX(cpu, ACTION_TARGET_FLAGS));
        }
        if (!MenuActionChild(&call, 0x82b4dcu, 0x82b564u, 2u, 1u) ||
            !MenuActionChild(&call, 0x82b4dfu, 0x81f34du, 3u, 1u))
            return call.result;
        if (cpu->carry) {
            OpLoadA(cpu, 0u);
            if (!MenuActionChild(&call, 0x82b4e7u, 0x82b58au, 2u, 1u) ||
                !MenuActionChild(&call, 0x82b4eau, 0x8297e3u, 2u, 1u))
                return call.result;
            continue;
        }
        OpLoadA(cpu, ACTION_SOUND_APPLY);
        if (!MenuActionChild(&call, 0x82b4f1u, 0x80953bu, 3u, 1u) ||
            !MenuActionChild(&call, 0x82b4f5u, 0x82b564u, 2u, 1u) ||
            !MenuActionChild(&call, 0x82b4f8u, 0x81f3bcu, 3u, 1u) ||
            !MenuActionChild(&call, 0x82b4fcu, 0x81f2e7u, 3u, 1u) ||
            !MenuActionChild(&call, 0x82b500u, 0x82b564u, 2u, 1u))
            return call.result;
        OpCmpValue(cpu, 1u);
        bool clear_targets = cpu->zero;
        if (!clear_targets) {
            OpCmp(memory, cpu, OpAbs(cpu, WRAM_MENU_PARTY_MEMBER_COUNT));
            clear_targets = !cpu->zero;
        }
        if (clear_targets) {
            OpLoadA(cpu, 0u);
            if (!MenuActionChild(&call, 0x82b50eu, 0x82b58au, 2u, 1u))
                return call.result;
        }
        if (!MenuActionChild(&call, 0x82b511u, 0x82999bu, 2u, 1u))
            return call.result;
        OpLoadA(cpu, 1u);
        if (!MenuActionChild(&call, 0x82b516u, 0x82a318u, 2u, 1u) ||
            !MenuActionChild(&call, 0x82b519u, 0x868b55u, 3u, 1u) ||
            !MenuActionChild(&call, 0x82b51du, 0x82b5edu, 2u, 1u) ||
            !MenuActionChild(&call, 0x82b520u, 0x82ac84u, 2u, 1u) ||
            !MenuActionChild(&call, 0x82b523u, 0x829588u, 2u, 1u) ||
            !MenuActionChild(&call, 0x82b526u, 0x8295bfu, 2u, 1u))
            return call.result;
        reload_spell = true;
    }
}
