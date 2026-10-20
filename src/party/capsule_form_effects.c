#include "party/capsule_presentation_internal.h"

enum {
    FORM_EFFECT_RECORDS = WRAM_CAPSULE_FORM_EFFECT_RECORDS & 0xffffu,
    FORM_EFFECT_ANGLE = FORM_EFFECT_RECORDS + 2u,
    FORM_EFFECT_RADIUS = FORM_EFFECT_RECORDS + 3u,
    FORM_EFFECT_FIRST_SLOT = 0x10u,
    FORM_EFFECT_SLOT_LIMIT = 0x14u,
    FORM_EFFECT_LIFETIME = 0x00f0u,
    DP_FORM_EFFECT_ANGLE = 0u,
    DP_FORM_EFFECT_RADIUS = 1u,
    DP_FORM_EFFECT_SPAWN_TIMER = 4u,
    DP_FORM_EFFECT_ACTIVE = 5u,
    DP_FORM_EFFECT_SPAWNED = 6u,
    DP_FORM_EFFECT_DRAIN_TIMER = 7u,
    FORM_EFFECT_SCALED_PRODUCT = WRAM_SYSTEM_MULTIPLY_PRODUCT + 1u
};

Lufia2ExecutionResult Lufia2CapsuleStepFormEffectAngle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, true))
        return ExecutionHandoff(cpu, 0x82d5e9u);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, FORM_EFFECT_ANGLE));
    OpIncA(cpu);
    OpIncA(cpu);
    OpCmpValue(cpu, 0xb4u);
    if (cpu->zero)
        OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbsY(cpu, FORM_EFFECT_ANGLE));
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_ANGLE));
    return ExecutionReturned(0x82d5fbu);
}

Lufia2ExecutionResult Lufia2CapsuleStepFormEffectLife(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsulePresentationReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82d5fcu);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, FORM_EFFECT_RECORDS));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, FORM_EFFECT_RECORDS));
    OpCmpValue(cpu, FORM_EFFECT_LIFETIME);
    OpSepWidths(cpu, 0x20u);
    if (cpu->zero || cpu->carry) {
        if (cpu->zero) {
            OpLoadA(cpu, 4u);
            OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
            OpLoadA(cpu, 2u);
            if (!CapsuleFeedChild(&call, 0x82d61au, 0x868cf5u, 3u))
                return call.result;
        }
        OpLda(memory, cpu, OpAbsY(cpu, FORM_EFFECT_RADIUS));
        if (cpu->zero) {
            OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
        } else {
            OpDecA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, FORM_EFFECT_RADIUS));
        }
    } else {
        OpLda(memory, cpu, OpAbsY(cpu, FORM_EFFECT_RADIUS));
    }
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_RADIUS));
    return ExecutionReturned(0x82d62eu);
}

Lufia2ExecutionResult Lufia2CapsuleSignFormProduct(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CapsulePresentationReady(cpu, false))
        return ExecutionHandoff(cpu, 0x82d68cu);
    OpLsrMem(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_FORM_EFFECT_SIGN_WORK));
    if (cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, FORM_EFFECT_SCALED_PRODUCT));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, FORM_EFFECT_SCALED_PRODUCT));
    }
    return ExecutionReturned(0x82d69bu);
}

static bool ScaleFormEffectAxis(CapsuleFeedCall *call, bool vertical) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_ANGLE));
    if (!CapsuleFeedChild(call, vertical ? 0x82d660u : 0x82d633u,
                          vertical ? 0x808450u : 0x808486u, 3u))
        return false;
    OpAslA(cpu);
    OpRolMem8(memory, cpu, OpAbs(cpu, WRAM_CAPSULE_FORM_EFFECT_SIGN_WORK));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_A));
    OpLda(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_RADIUS));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SYSTEM_MULTIPLY_B));
    return CapsuleFeedChild(call, vertical ? 0x82d678u : 0x82d64bu,
                             0x828000u, 3u) &&
        CapsuleFeedChild(call, vertical ? 0x82d67cu : 0x82d64fu,
                          0x82d68cu, 2u);
}

Lufia2ExecutionResult Lufia2CapsulePlaceFormEffect(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsulePresentationReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82d62fu);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    if (!ScaleFormEffectAxis(&call, false))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, MENU_SPRITE_X_LOW));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, FORM_EFFECT_SCALED_PRODUCT));
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
    if (!ScaleFormEffectAxis(&call, true))
        return call.result;
    OpLda(memory, cpu, OpAbs(cpu, MENU_SPRITE_Y_LOW));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, FORM_EFFECT_SCALED_PRODUCT)));
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x82d68bu);
}

static bool StepFormEffectSlots(CapsuleFeedCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLdx(cpu, FORM_EFFECT_FIRST_SLOT);
    OpLdy(cpu, 0u);
    OpStz(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_ACTIVE));
    do {
        bool step = true;
        OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
        if (cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_SPAWNED));
            OpCmpValue(cpu, 4u);
            if (cpu->zero) {
                step = false;
            } else {
                OpLda(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_SPAWN_TIMER));
                OpCmpValue(cpu, 0x16u);
                if (!cpu->zero) {
                    step = false;
                } else {
                    OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_SPAWNED), 1);
                    OpStz(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_SPAWN_TIMER));
                    OpLoadA(cpu, 1u);
                    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
                }
            }
        }
        if (step) {
            OpSta(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_ACTIVE));
            PushDataBank(memory, cpu);
            OpLoadA(cpu, 0x7eu);
            PushAccumulator8(memory, cpu);
            PullDataBank(memory, cpu);
            if (!CapsuleFeedChild(call, 0x82d5a8u, 0x82d5e9u, 2u) ||
                !CapsuleFeedChild(call, 0x82d5abu, 0x82d5fcu, 2u))
                return false;
            PullDataBank(memory, cpu);
            if (!CapsuleFeedChild(call, 0x82d5afu, 0x82d62fu, 2u))
                return false;
        }
        OpIny(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpInx(cpu);
        OpCpx(cpu, FORM_EFFECT_SLOT_LIMIT);
    } while (!cpu->zero);
    return true;
}

static bool DrainFormFoodMeter(CapsuleFeedCall *call) {
    const Lufia2Memory *memory = call->memory;
    Lufia2CpuState *cpu = call->cpu;
    OpLda(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_SPAWNED));
    OpCmpValue(cpu, 4u);
    if (!cpu->zero)
        return true;
    OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_DRAIN_TIMER), 1);
    OpLda(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_DRAIN_TIMER));
    OpCmpValue(cpu, 4u);
    if (!cpu->zero)
        return true;
    OpStz(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_DRAIN_TIMER));
    if (!CapsuleFeedChild(call, 0x82d5ccu, 0x82c492u, 2u))
        return false;
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    if (!cpu->zero)
        OpStepMem(memory, cpu, OpAbsX(cpu, 0u), -1);
    return true;
}

Lufia2ExecutionResult Lufia2CapsuleRunFormEffects(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsulePresentationReady(cpu, true) || !child)
        return ExecutionHandoff(cpu, 0x82d577u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0x16u);
    OpSta(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_SPAWN_TIMER));
    OpStz(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_SPAWNED));
    OpStz(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_DRAIN_TIMER));
    do {
        if (!StepFormEffectSlots(&call) || !DrainFormFoodMeter(&call))
            return call.result;
        OpStepMem(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_SPAWN_TIMER), 1);
        if (!CapsuleFeedChild(&call, 0x82d5d9u, 0x82d150u, 2u))
            return call.result;
        OpLoadA(cpu, 8u);
        OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
        if (!CapsuleFeedChild(&call, 0x82d5e0u, 0x868b55u, 3u))
            return call.result;
        OpLda(memory, cpu, OpDp(cpu, DP_FORM_EFFECT_ACTIVE));
    } while (!cpu->zero);
    return ExecutionReturned(0x82d5e8u);
}
