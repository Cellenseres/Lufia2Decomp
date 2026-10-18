#include "lufia2/menu.h"
#include "menu/menu_action_internal.h"
#include "menu/menu_sprite_slots.h"
#include "system/wram.h"

enum {
    EFFECT_MASK = 0x00u,
    EFFECT_SAVED_SLOT = 0x02u,
    EFFECT_PHASE = 0x04u,
    EFFECT_VALUE_POINTER = 0x08u,
    EFFECT_TEXT_BANK = 0x5fu,
    EFFECT_TARGET_BITS = 0x8ed8bbu,
    EFFECT_LABEL_ANIMATIONS = 0x829794u,
    EFFECT_REVEAL_HANDLERS = 0x82978eu,
    EFFECT_MEMBER_VALUES = WRAM_BATTLE_PREVIEW_SPRITE_ID
};

Lufia2ExecutionResult Lufia2MenuAddEffectTarget(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *context) {
    (void)context;
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82ae64u);
    OpLda(memory, cpu, OpLongX(cpu, EFFECT_TARGET_BITS));
    OpOra(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    return ExecutionReturned(0x82ae6eu);
}

Lufia2ExecutionResult Lufia2MenuResetEffectSprites(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829588u);
    OpSepWidths(cpu, 0x10u);
    OpLoadA(cpu, 24u);
    OpSta(memory, cpu, OpDp(cpu, EFFECT_PHASE));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    OpSta(memory, cpu, OpDp(cpu, EFFECT_MASK));
    do {
        OpLsrMem(memory, cpu, OpDp(cpu, EFFECT_MASK));
        if (cpu->carry) {
            OpLda(memory, cpu, OpDp(cpu, EFFECT_PHASE));
            cpu->carry = 1u;
            OpSbcValue(cpu, 8u);
            OpTax(cpu);
            OpLdy(cpu, 16u);
            do {
                OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
                OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_HIGH));
                OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_HIGH));
                if (!CallChildWithFrame(memory, cpu, child, context, 0x8295a8u,
                                        0x868cdau, 3u, 0x82u))
                    return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND,
                                                   0x8295a8u, 0u};
                if (cpu->program_bank != 0x82u || !cpu->accumulator_is_8_bit ||
                    !cpu->index_is_8_bit || cpu->direct_page || cpu->decimal)
                    return ExecutionHandoff(cpu, 0x8295acu);
                OpInx(cpu);
                OpCpx(cpu, OpReadX(memory, cpu, OpDp(cpu, EFFECT_PHASE)));
            } while (!cpu->zero);
        }
        OpLda(memory, cpu, OpDp(cpu, EFFECT_PHASE));
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, EFFECT_PHASE));
        OpCmpValue(cpu, 56u);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x10u);
    return ExecutionReturned(0x8295beu);
}

Lufia2ExecutionResult Lufia2MenuUpdateItemCapacityHint(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x829e2fu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_LIST_MODE));
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero)
        return ExecutionReturned(0x829e66u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_SELECTED_SPELL));
    OpAndValue(cpu, 0x01ffu);
    if (!MenuActionChild(&call, 0x829e3eu, 0x82fb1fu, 3u, 0u))
        return call.result;
    if (cpu->carry) {
        if (!MenuActionChild(&call, 0x829e49u, 0x82fb94u, 2u, 0u))
            return call.result;
    } else if (!MenuActionChild(&call, 0x829e44u, 0x82faf4u, 2u, 0u)) {
        return call.result;
    }
    OpLoadA(cpu, 0u);
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, EFFECT_MASK));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DRAW_MODE));
    OpLoadA(cpu, 0x8eu);
    OpSta(memory, cpu, OpDp(cpu, EFFECT_TEXT_BANK));
    OpLdy(cpu, 0xd31eu);
    if (!MenuActionChild(&call, 0x829e62u, 0x808878u, 3u, 1u))
        return call.result;
    return ExecutionReturned(0x829e66u);
}

Lufia2ExecutionResult Lufia2MenuPositionEffectSprites(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x8295fau);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
    cpu->carry = 1u;
    OpSbcValue(cpu, 16u);
    for (uint16_t part = 3u; part < 8u; ++part)
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(MENU_SPRITE_X_LOW + part)));
    OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    cpu->carry = 1u;
    OpSbcValue(cpu, 4u);
    for (uint16_t part = 3u; part < 8u; ++part)
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(MENU_SPRITE_Y_LOW + part)));
    OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
    OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_X_LOW + 1u));
    cpu->carry = 1u;
    OpSbcValue(cpu, 8u);
    OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_X_LOW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 16u);
    OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_X_LOW + 2u));
    OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 8u);
    for (uint16_t part = 0u; part < 3u; ++part)
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(MENU_SPRITE_Y_LOW + part)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND_COPY));
    OpCmpValue(cpu, 8u);
    if (cpu->zero) {
        OpLoadA(cpu, 10u);
        for (uint16_t digit = 0u; digit < 3u; ++digit)
            OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(MENU_SPRITE_ANIMATION + digit)));
        return ExecutionReturned(0x829690u);
    }
    OpCmpValue(cpu, 18u);
    if (cpu->carry) {
        OpLda(memory, cpu, OpLongX(cpu, EFFECT_TARGET_BITS));
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK), 0u);
        return ExecutionReturned(0x829698u);
    }
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    OpSta(memory, cpu, OpDp(cpu, EFFECT_VALUE_POINTER));
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, EFFECT_VALUE_POINTER));
    cpu->carry = 0u;
    OpAdcValue(cpu, EFFECT_MEMBER_VALUES);
    OpSta(memory, cpu, OpDp(cpu, EFFECT_VALUE_POINTER));
    OpLda(memory, cpu, OpAbs(cpu, Read16Direct(memory, cpu, EFFECT_VALUE_POINTER)));
    if (!MenuActionChild(&call, 0x829661u, 0x8291abu, 2u, 0u))
        return call.result;
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, EFFECT_MASK));
    if (cpu->zero)
        OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_ANIMATION));
    OpLda(memory, cpu, OpDp(cpu, EFFECT_SAVED_SLOT));
    if (cpu->zero) {
        OpLda(memory, cpu, OpAbsY(cpu, MENU_SPRITE_ANIMATION));
        OpCmpValue(cpu, 10u);
        if (!cpu->zero)
            OpLoadA(cpu, 0u);
    }
    OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_ANIMATION + 1u));
    OpLda(memory, cpu, OpDp(cpu, EFFECT_PHASE));
    OpSta(memory, cpu, OpAbsY(cpu, MENU_SPRITE_ANIMATION + 2u));
    return ExecutionReturned(0x829684u);
}

Lufia2ExecutionResult Lufia2MenuLoadEffectDigitAnimation(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x82970bu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ANIMATION));
    OpAslA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpLoadA(cpu, 0u);
    if (!MenuActionChild(&call, 0x829717u, 0x868cf5u, 3u, 1u))
        return call.result;
    return ExecutionReturned(0x82971bu);
}

static Lufia2ExecutionResult RevealEffectPart(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              uint32_t entry, uint16_t label,
                                              uint16_t digit, uint32_t finish) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, entry);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EFFECT_SAVED_SLOT)));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(MENU_SPRITE_ACTIVE + label)));
    if (digit != 0xffffu)
        OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(MENU_SPRITE_ACTIVE + digit)));
    return ExecutionReturned(finish);
}

Lufia2ExecutionResult Lufia2MenuRevealEffectLabel(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    (void)context;
    return RevealEffectPart(memory, cpu, child, 0x82975du, 3u, 0xffffu, 0x829764u);
}

Lufia2ExecutionResult Lufia2MenuRevealFirstEffectDigit(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    (void)context;
    return RevealEffectPart(memory, cpu, child, 0x829765u, 4u, 0u, 0x82976fu);
}

Lufia2ExecutionResult Lufia2MenuRevealSecondEffectDigit(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    (void)context;
    return RevealEffectPart(memory, cpu, child, 0x829770u, 5u, 1u, 0x82977au);
}

Lufia2ExecutionResult Lufia2MenuRevealThirdEffectDigit(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu,
                                                       Lufia2PushedChildCall child,
                                                       void *context) {
    (void)context;
    return RevealEffectPart(memory, cpu, child, 0x82977bu, 6u, 2u, 0x829785u);
}

Lufia2ExecutionResult Lufia2MenuRevealEffectSuffix(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    (void)context;
    return RevealEffectPart(memory, cpu, child, 0x829786u, 7u, 0xffffu, 0x82978du);
}

static bool LoadEffectLabels(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, EFFECT_PHASE), cpu->x);
    do {
        OpLdx(cpu, 19u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
        OpSta(memory, cpu, OpDp(cpu, EFFECT_MASK));
        do {
            OpLsrMem(memory, cpu, OpDp(cpu, EFFECT_MASK));
            if (cpu->carry) {
                OpWriteX(memory, cpu, OpDp(cpu, EFFECT_SAVED_SLOT), cpu->x);
                OpRepWidths(cpu, 0x20u);
                OpTxa(cpu);
                cpu->carry = 0u;
                OpAdc(memory, cpu, OpDp(cpu, EFFECT_PHASE));
                OpTax(cpu);
                OpSepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpDp(cpu, EFFECT_PHASE));
                OpAslA(cpu);
                OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
                OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
                OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_LABEL_ANIMATION));
                if (!MenuActionChild(call, 0x8296c1u, 0x868cf5u, 3u, 1u))
                    return false;
                OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ANIMATION));
                OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EFFECT_SAVED_SLOT)));
            }
            OpRepWidths(cpu, 0x20u);
            OpTxa(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 8u);
            OpTax(cpu);
            OpSepWidths(cpu, 0x20u);
            OpCpx(cpu, 51u);
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpDp(cpu, EFFECT_PHASE), 1);
        OpLda(memory, cpu, OpDp(cpu, EFFECT_PHASE));
        OpCmpValue(cpu, 5u);
    } while (!cpu->zero);
    return true;
}

static bool LoadEffectDigits(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLdx(cpu, 16u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    OpSta(memory, cpu, OpDp(cpu, EFFECT_MASK));
    do {
        OpLsrMem(memory, cpu, OpDp(cpu, EFFECT_MASK));
        if (cpu->carry) {
            if (!MenuActionChild(call, 0x8296edu, 0x82970bu, 2u, 1u))
                return false;
            OpInx(cpu);
            if (!MenuActionChild(call, 0x8296f1u, 0x82970bu, 2u, 1u))
                return false;
            OpInx(cpu);
            if (!MenuActionChild(call, 0x8296f5u, 0x82970bu, 2u, 1u))
                return false;
            OpDex(cpu);
            OpDex(cpu);
        }
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpCpx(cpu, 48u);
    } while (!cpu->zero);
    return true;
}

static bool RevealEffectParts(MenuActionCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, EFFECT_PHASE), cpu->x);
    do {
        OpLdx(cpu, 16u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
        OpSta(memory, cpu, OpDp(cpu, EFFECT_MASK));
        do {
            OpLsrMem(memory, cpu, OpDp(cpu, EFFECT_MASK));
            if (cpu->carry) {
                OpLoadA(cpu, 1u);
                OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_DISPLAY_REQUESTS + 2u));
                OpWriteX(memory, cpu, OpDp(cpu, EFFECT_SAVED_SLOT), cpu->x);
                OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EFFECT_PHASE)));
                uint16_t table = (uint16_t)(EFFECT_REVEAL_HANDLERS + cpu->x);
                uint32_t target = Read8(memory, 0x820000u | table);
                target |= (uint32_t)Read8(memory, 0x820000u | (uint16_t)(table + 1u))
                          << 8;
                target |= 0x820000u;
                if (!MenuActionChild(call, 0x829736u, target, 2u, 1u))
                    return false;
                OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, EFFECT_SAVED_SLOT)));
            }
            OpRepWidths(cpu, 0x20u);
            OpTxa(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 8u);
            OpTax(cpu);
            OpSepWidths(cpu, 0x20u);
            OpCpx(cpu, 48u);
        } while (!cpu->zero);
        if (!MenuActionChild(call, 0x82974au, 0x868b55u, 3u, 1u) ||
            !MenuActionChild(call, 0x82974eu, 0x868b55u, 3u, 1u))
            return false;
        OpStepMem(memory, cpu, OpDp(cpu, EFFECT_PHASE), 1);
        OpStepMem(memory, cpu, OpDp(cpu, EFFECT_PHASE), 1);
        OpLda(memory, cpu, OpDp(cpu, EFFECT_PHASE));
        OpCmpValue(cpu, 10u);
    } while (!cpu->zero);
    return true;
}

Lufia2ExecutionResult Lufia2MenuShowUseEffect(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              void *context) {
    if (!MenuActionReady(cpu, child, 1u))
        return ExecutionHandoff(cpu, 0x8295bfu);
    MenuActionCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND_COPY));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_KIND_COPY + 1u));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, EFFECT_LABEL_ANIMATIONS));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_LABEL_ANIMATION));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_LABEL_ANIMATION + 1u));
    OpLdx(cpu, 0u);
    OpLdy(cpu, 16u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    do {
        OpLsrA(cpu);
        PushAccumulator8(memory, cpu);
        if (cpu->carry && !MenuActionChild(&call, 0x8295e3u, 0x8295fau, 2u, 1u))
            return call.result;
        OpRepWidths(cpu, 0x20u);
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        LoadA8(cpu, Pull8(memory, cpu));
        OpInx(cpu);
        OpCpx(cpu, 4u);
    } while (!cpu->zero);
    if (!LoadEffectLabels(&call) || !LoadEffectDigits(&call) ||
        !RevealEffectParts(&call))
        return call.result;
    return ExecutionReturned(0x82975cu);
}
