#include "party/capsule_feed_internal.h"

enum {
    FIRST_FOOD_PARTICLE = 0x10u,
    FOOD_PARTICLE_LIMIT = MENU_SPRITE_SLOTS,
    DP_FOOD_PARTICLE_DELAY = 2u,
    DP_FOOD_PARTICLE_COUNT = 4u
};

Lufia2ExecutionResult Lufia2CapsuleAnimateFood(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82cb10u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpAbs(cpu, CAPSULE_FOOD_AMOUNT));
    OpCmpValue(cpu, 0u);
    if (cpu->zero)
        return ExecutionReturned(0x82cb17u);
    OpAslA(cpu);
    OpAslA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 4u);
    OpCmpValue(cpu, CAPSULE_FOOD_METER_LIMIT);
    if (cpu->carry)
        OpLoadA(cpu, CAPSULE_FOOD_METER_LIMIT);
    PushAccumulator8(memory, cpu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_MENU_EFFECT_TARGET_MASK));
    if (!CapsuleFeedChild(&call, 0x82cb29u, 0x829588u, 2u))
        return call.result;
    OpLoadA(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_FOOD_PARTICLE_COUNT));
    OpLoadA(cpu, 0x73u);
    if (!CapsuleFeedChild(&call, 0x82cb31u, 0x80953bu, 3u))
        return call.result;
    OpLdx(cpu, FIRST_FOOD_PARTICLE);
    do {
        if (!CapsuleFeedChild(&call, 0x82cb38u, 0x82cb55u, 2u))
            return call.result;
        OpPushX(memory, cpu);
        if (!CapsuleFeedChild(&call, 0x82cb3cu, 0x82cbafu, 2u) ||
            !CapsuleFeedChild(&call, 0x82cb3fu, 0x82cb8cu, 2u))
            return call.result;
        OpLdx(cpu, PullIndexValue(memory, cpu));
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_FOOD_PARTICLE_COUNT), -1);
    } while (!cpu->zero);
    if (!CapsuleFeedChild(&call, 0x82cb48u, 0x82cb8cu, 2u) ||
        !CapsuleFeedChild(&call, 0x82cb4bu, 0x82cb8cu, 2u) ||
        !CapsuleFeedChild(&call, 0x82cb4eu, 0x82cb8cu, 2u) ||
        !CapsuleFeedChild(&call, 0x82cb51u, 0x82cb8cu, 2u))
        return call.result;
    return ExecutionReturned(0x82cb54u);
}

Lufia2ExecutionResult Lufia2CapsuleSpawnFoodParticle(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82cb55u);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 0x30u);
    if (!CapsuleFeedChild(&call, 0x82cb57u, 0x808299u, 3u))
        return call.result;
    OpSta(memory, cpu, OpDp(cpu, DP_FEED_SCRATCH));
    OpLda(memory, cpu, OpAbs(cpu, MENU_SPRITE_X_LOW));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x14u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_FEED_SCRATCH));
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_X_LOW));
    OpLoadA(cpu, 0x20u);
    if (!CapsuleFeedChild(&call, 0x82cb6bu, 0x808299u, 3u))
        return call.result;
    OpSta(memory, cpu, OpDp(cpu, DP_FEED_SCRATCH));
    OpLda(memory, cpu, OpAbs(cpu, MENU_SPRITE_Y_LOW));
    cpu->carry = 1u;
    OpSbcValue(cpu, 4u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DP_FEED_SCRATCH));
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW));
    OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
    OpLoadA(cpu, 1u);
    if (!CapsuleFeedChild(&call, 0x82cb87u, 0x868cf5u, 3u))
        return call.result;
    return ExecutionReturned(0x82cb8bu);
}

Lufia2ExecutionResult Lufia2CapsuleStepFoodParticles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82cb8cu);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, DP_FOOD_PARTICLE_DELAY));
    do {
        OpLdx(cpu, FIRST_FOOD_PARTICLE);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, MENU_SPRITE_FRAME_INDEX));
            OpCmpValue(cpu, 4u);
            if (cpu->zero)
                OpStz(memory, cpu, OpAbsX(cpu, MENU_SPRITE_ACTIVE));
            OpStepMem(memory, cpu, OpAbsX(cpu, MENU_SPRITE_Y_LOW), -1);
            OpInx(cpu);
            OpCpx(cpu, FOOD_PARTICLE_LIMIT);
        } while (!cpu->zero);
        if (!CapsuleFeedChild(&call, 0x82cba6u, 0x868b55u, 3u))
            return call.result;
        OpStepMem(memory, cpu, OpDp(cpu, DP_FOOD_PARTICLE_DELAY), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x82cbaeu);
}

Lufia2ExecutionResult Lufia2CapsuleApplyFoodUnit(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!CapsuleFeedReady(cpu) || !child)
        return ExecutionHandoff(cpu, 0x82cbafu);
    CapsuleFeedCall call = {memory, cpu, child, context, {0}};
    OpLda(memory, cpu, OpDp(cpu, DP_FOOD_PARTICLE_COUNT));
    OpBitValue(cpu, 1u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, CAPSULE_FOOD_AMOUNT));
        if (!cpu->zero) {
            OpStepMem(memory, cpu, OpAbs(cpu, CAPSULE_FOOD_AMOUNT), -1);
            if (!CapsuleFeedChild(&call, 0x82cbbdu, 0x82c492u, 2u))
                return call.result;
            OpLda(memory, cpu, OpAbsX(cpu, 0u));
            OpCmpValue(cpu, CAPSULE_FOOD_METER_LIMIT);
            if (!cpu->zero)
                OpStepMem(memory, cpu, OpAbsX(cpu, 0u), 1);
            if (!CapsuleFeedChild(&call, 0x82cbcau, 0x82d150u, 2u))
                return call.result;
            OpLoadA(cpu, 8u);
            OpTestBits(memory, cpu, OpDp(cpu, DP_MENU_FRAME_FLAGS), 1u);
        }
    }
    return ExecutionReturned(0x82cbd1u);
}
