#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    ACTION_SOURCE = WRAM_BATTLE_ACTION_WORK,
    ACTION_TARGET = WRAM_BATTLE_ACTION_WORK + 2u,
    DAMAGE_FACTOR = WRAM_BATTLE_ACTION_WORK + 4u,
    ACTION_KIND = WRAM_BATTLE_ACTION_WORK + 6u,
    ACTION_FLAGS = WRAM_BATTLE_ACTION_WORK + 12u,
    HIT_CHANCE = WRAM_BATTLE_ACTION_WORK + 16u,
    DAMAGE_SCALE = WRAM_BATTLE_ACTION_WORK + 18u,
    HIT_POINT_CHANGE = WRAM_BATTLE_ACTION_WORK + 20u,
    HIT_POINT_VARIANCE = WRAM_BATTLE_ACTION_WORK + 22u,
    MAGIC_POINT_CHANGE = WRAM_BATTLE_ACTION_WORK + 26u,
    MAGIC_POINT_VARIANCE = WRAM_BATTLE_ACTION_WORK + 28u,
    STATUS_CHANCE_A = WRAM_BATTLE_ACTION_WORK + 110u,
    STATUS_CHANCE_B = WRAM_BATTLE_ACTION_WORK + 114u,
    TARGET_DEFENSE = WRAM_BATTLE_ACTION_WORK + 126u,
    ACTION_INTERRUPTED = WRAM_BATTLE_ACTION_WORK + 130u,
    ACTION_RESULT = WRAM_BATTLE_DISPATCH_TARGET_COUNT,
    SCRIPT_CONTEXT = WRAM_BATTLE_SCRIPT_CONTEXT,
    SCRIPT_RUNNING = 0x0a52u,
    REMOVED_STATUS = WRAM_BATTLE_REMOVED_STATUS,
    ADDED_STATUS = WRAM_BATTLE_ADDED_STATUS,
    APPLIED_HP_CHANGE = WRAM_BATTLE_APPLIED_HP_CHANGE,
    APPLIED_MP_CHANGE = WRAM_BATTLE_APPLIED_MP_CHANGE,
    ACTION_HIT_COUNTER = 0x7ffab7u,
    ACTOR_STATUS = 0x000fu,
    ACTOR_HP = 0x0011u,
    ACTOR_MP = 0x0013u,
    ACTOR_MAX_HP = 0x0025u,
    ACTOR_MAX_MP = 0x0027u,
    ACTOR_BASE_STAT = 0x0029u,
    ACTOR_STAT_CHANGE = 0x0037u,
    ACTOR_IP = 0x00bcu,
    ACTION_RECORD_POINTER = 0xb8u,
    MULTIPLICAND = 0x4eu,
    MULTIPLIER = 0x50u,
    PRODUCT = 0x51u,
    SCALED_PRODUCT = 0x54u,
    SAVED_SCALE = 0x56u,
    STATUS_MASK = 0x54u,
    ACTOR_BIT = 0x59u,
    ACTOR_SIDE = 0x58u,
    SAVED_DELTA = 0x5au,
    DIVIDEND = 0x5du,
    DIVISOR = 0x54u,
    DEFENSE_WORK = 0xcau,
    VECTOR_LENGTH = 0x58u,
    VECTOR_RESULT = 0x63u,
    VECTOR_COMPONENT = 0x65u,
    WRMPYA = 0x4202u,
    WRMPYB = 0x4203u,
    WRDIVL = 0x4204u,
    WRDIVB = 0x4206u,
    RDDIVL = 0x4214u,
    RDMPYL = 0x4216u
};

static uint32_t ActionField(const Lufia2Memory *memory, const Lufia2CpuState *cpu) {
    return DirectLongIndirectY(memory, cpu, ACTION_RECORD_POINTER);
}

static uint8_t ActionCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          Lufia2PushedChildCall child, void *context, uint32_t site,
                          uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x85u);
}

static Lufia2ExecutionResult ActionUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static Lufia2ExecutionResult RejectAction(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu) {
    OpLda(memory, cpu, ACTION_FLAGS);
    OpOraValue(cpu, 1u);
    OpSta(memory, cpu, ACTION_FLAGS);
    cpu->carry = 1u;
    return ExecutionReturned(0x85d115u);
}

static Lufia2ExecutionResult RunTargetScripts(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              void *context) {
    OpLda(memory, cpu, ACTION_FLAGS);
    OpBitValue(cpu, 8u);
    const uint8_t use_resistance = !cpu->zero;
    if (!use_resistance) {
        OpLda(memory, cpu, ACTION_KIND);
        OpCmpValue(cpu, 13u);
        if (cpu->zero)
            return ExecutionReturned(0u);
    }
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, use_resistance ? 0x35u : 0x2bu));
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, use_resistance ? 0x43u : 0x39u)));
    OpSta(memory, cpu, TARGET_DEFENSE);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0x45u)));
    if (cpu->zero) {
        const uint32_t site = use_resistance ? 0x85d177u : 0x85d1b8u;
        if (!ActionCall(memory, cpu, child, context, site, 0x85da11u, 3u))
            return ActionUnwound(site);
    } else {
        OpLda(memory, cpu, OpAbs(cpu, SCRIPT_CONTEXT));
        if (!cpu->zero) {
            OpLoadA(cpu, 0xffu);
            OpSta(memory, cpu, OpAbs(cpu, SCRIPT_RUNNING));
            OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_POINTER), cpu->y);
            OpLda(memory, cpu, OpAbsX(cpu, 0x47u));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_NAME_BANK));
            OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0x4cu)));
            const uint32_t site = use_resistance ? 0x85d16eu : 0x85d1afu;
            if (!ActionCall(memory, cpu, child, context, site, 0x81fac9u, 3u))
                return ActionUnwound(site);
            OpStz(memory, cpu, OpAbs(cpu, SCRIPT_RUNNING));
        }
    }
    return ExecutionReturned(0u);
}

static void ShiftDamageProduct(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, PRODUCT + 2u));
    OpRepWidths(cpu, 0x20u);
    for (unsigned shift = 0u; shift < 4u; ++shift) {
        OpLsrA(cpu);
        const uint16_t old = OpReadM(memory, cpu, OpDp(cpu, PRODUCT));
        const uint16_t value = (uint16_t)((old >> 1) | (cpu->carry ? 0x8000u : 0u));
        cpu->carry = old & 1u;
        Write8(memory, OpDp(cpu, PRODUCT + 1u), (uint8_t)(value >> 8));
        Write8(memory, OpDp(cpu, PRODUCT), (uint8_t)value);
        SetNz16(cpu, value);
    }
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, PRODUCT + 2u));
    OpBitValue(cpu, 0xffu);
}

static void ReduceByDefense(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, TARGET_DEFENSE);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DEFENSE_WORK));
    OpLda(memory, cpu, OpDp(cpu, SCALED_PRODUCT));
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PRODUCT + 1u)));
    OpSta(memory, cpu, OpDp(cpu, SCALED_PRODUCT));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DEFENSE_WORK)));
    if (!cpu->carry)
        TransferDirectToA(cpu);
}

static Lufia2ExecutionResult ScaleHitPointChange(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    OpPushX(memory, cpu);
    OpLda(memory, cpu, DAMAGE_SCALE);
    OpSta(memory, cpu, OpDp(cpu, SAVED_SCALE));
    OpLda(memory, cpu, HIT_POINT_CHANGE);
    OpSta(memory, cpu, OpDp(cpu, SAVED_DELTA));
    const uint8_t damage = cpu->negative;
    if (damage) {
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffffu)));
        OpIncA(cpu);
    }
    OpSta(memory, cpu, OpDp(cpu, MULTIPLICAND));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, SAVED_SCALE));
    OpSta(memory, cpu, OpDp(cpu, MULTIPLIER));
    uint32_t site = damage ? 0x85d226u : 0x85d28eu;
    if (!ActionCall(memory, cpu, child, context, site, 0x80834cu, 3u))
        return ActionUnwound(site);
    ShiftDamageProduct(memory, cpu);
    if (!cpu->zero)
        return ExecutionHandoff(cpu, damage ? 0x85d243u : 0x85d2abu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PRODUCT)));
    OpWriteX(memory, cpu, OpDp(cpu, SCALED_PRODUCT), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, MULTIPLICAND), cpu->x);
    OpLda(memory, cpu, HIT_POINT_VARIANCE);
    OpSta(memory, cpu, OpDp(cpu, MULTIPLIER));
    site = damage ? 0x85d250u : 0x85d2b8u;
    if (!ActionCall(memory, cpu, child, context, site, 0x80834cu, 3u))
        return ActionUnwound(site);
    OpRepWidths(cpu, 0x20u);
    if (damage) {
        ReduceByDefense(memory, cpu);
        OpLsrA(cpu);
        OpSta(memory, cpu, OpDp(cpu, MULTIPLICAND));
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, DAMAGE_FACTOR);
        OpSta(memory, cpu, OpDp(cpu, MULTIPLIER));
        if (!ActionCall(memory, cpu, child, context, 0x85d275u, 0x80834cu, 3u))
            return ActionUnwound(0x85d275u);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, MULTIPLICAND));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PRODUCT + 1u)));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffffu)));
        OpIncA(cpu);
    } else {
        OpLda(memory, cpu, OpDp(cpu, SCALED_PRODUCT));
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PRODUCT + 1u)));
        OpSta(memory, cpu, OpDp(cpu, SCALED_PRODUCT));
    }
    OpSta(memory, cpu, HIT_POINT_CHANGE);
    return ExecutionReturned(0u);
}

static Lufia2ExecutionResult ScaleMagicPointChange(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, MAGIC_POINT_CHANGE);
    OpSta(memory, cpu, OpDp(cpu, SAVED_DELTA));
    const uint8_t damage = cpu->negative;
    if (damage)
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffffu)));
    OpSta(memory, cpu, OpDp(cpu, SCALED_PRODUCT));
    OpSta(memory, cpu, OpDp(cpu, MULTIPLICAND));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, MAGIC_POINT_VARIANCE);
    OpSta(memory, cpu, OpDp(cpu, MULTIPLIER));
    const uint32_t site = damage ? 0x85d2e2u : 0x85d30eu;
    if (!ActionCall(memory, cpu, child, context, site, 0x80834cu, 3u))
        return ActionUnwound(site);
    OpRepWidths(cpu, 0x20u);
    if (damage) {
        ReduceByDefense(memory, cpu);
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffffu)));
        OpIncA(cpu);
    } else {
        OpLda(memory, cpu, OpDp(cpu, SCALED_PRODUCT));
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PRODUCT + 1u)));
        OpSta(memory, cpu, OpDp(cpu, SCALED_PRODUCT));
    }
    OpSta(memory, cpu, MAGIC_POINT_CHANGE);
    OpLdy(cpu, 0x28u);
    do {
        OpLda(memory, cpu, ActionField(memory, cpu));
        if (!ActionCall(memory, cpu, child, context, 0x85d324u, 0x808299u, 3u))
            return ActionUnwound(0x85d324u);
        OpDey(cpu);
        OpDey(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, ActionField(memory, cpu)));
        OpSta(memory, cpu, ActionField(memory, cpu));
        for (unsigned step = 0u; step < 4u; ++step)
            OpDey(cpu);
        OpCpy(cpu, 0x1cu);
    } while (!cpu->zero);
    OpPullX(memory, cpu);
    return ExecutionReturned(0u);
}

static Lufia2ExecutionResult ScaleAttributeChanges(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    OpPushX(memory, cpu);
    OpLdy(cpu, 0x30u);
    do {
        OpLda(memory, cpu, ActionField(memory, cpu));
        if (!cpu->zero) {
            OpSepWidths(cpu, 0x20u);
            OpBitValue(cpu, 0x80u);
            const uint8_t decrease = !cpu->zero;
            PushY(memory, cpu);
            if (decrease) {
                OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffu)));
                OpIncA(cpu);
            }
            OpSta(memory, cpu, OpDp(cpu, MULTIPLIER));
            OpRepWidths(cpu, 0x20u);
            if (decrease) {
                OpLda(memory, cpu, OpAbsX(cpu, ACTOR_STAT_CHANGE));
                if (cpu->negative)
                    TransferDirectToA(cpu);
                cpu->carry = 0u;
                OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_BASE_STAT)));
                OpSta(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_MEMBER_INDEX));
            } else {
                OpLda(memory, cpu, OpAbsX(cpu, ACTOR_BASE_STAT));
            }
            OpSta(memory, cpu, OpDp(cpu, MULTIPLICAND));
            uint32_t site = decrease ? 0x85d361u : 0x85d3b2u;
            if (!ActionCall(memory, cpu, child, context, site, 0x80834cu, 3u))
                return ActionUnwound(site);
            OpLda(memory, cpu, OpDp(cpu, PRODUCT));
            OpSta(memory, cpu, OpDp(cpu, DIVIDEND));
            OpSepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpDp(cpu, PRODUCT + 2u));
            OpSta(memory, cpu, OpDp(cpu, DIVIDEND + 2u));
            OpLoadA(cpu, 100u);
            OpSta(memory, cpu, OpDp(cpu, DIVISOR));
            OpPushX(memory, cpu);
            site = decrease ? 0x85d374u : 0x85d3c5u;
            if (!ActionCall(memory, cpu, child, context, site, 0x85dc6fu, 3u))
                return ActionUnwound(site);
            OpPullX(memory, cpu);
            OpRepWidths(cpu, 0x20u);
            if (decrease) {
                OpLda(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_MEMBER_INDEX));
                cpu->carry = 1u;
                OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DIVIDEND)));
                OpSta(memory, cpu, OpDp(cpu, DEFENSE_WORK));
                OpLda(memory, cpu, OpAbsX(cpu, ACTOR_BASE_STAT));
                cpu->carry = 0u;
                OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_STAT_CHANGE)));
                cpu->carry = 1u;
                OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DEFENSE_WORK)));
                if (!cpu->carry)
                    TransferDirectToA(cpu);
            } else {
                OpLda(memory, cpu, OpDp(cpu, DIVIDEND));
                cpu->carry = 1u;
                OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_STAT_CHANGE)));
                if (cpu->negative != cpu->overflow)
                    TransferDirectToA(cpu);
            }
            OpLsrA(cpu);
            OpAdcValue(cpu, 0u);
            OpPullY(memory, cpu);
            PushY(memory, cpu);
            for (unsigned step = 0u; step < 4u; ++step)
                OpDey(cpu);
            if (decrease) {
                OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffffu)));
                OpIncA(cpu);
            }
            OpSta(memory, cpu, ActionField(memory, cpu));
            OpIny(cpu);
            OpIny(cpu);
            TransferDirectToA(cpu);
            OpSta(memory, cpu, ActionField(memory, cpu));
            OpPullY(memory, cpu);
        }
        OpInx(cpu);
        OpInx(cpu);
        for (unsigned step = 0u; step < 6u; ++step)
            OpIny(cpu);
        OpCpy(cpu, 0x5au);
    } while (!cpu->zero);
    OpPullX(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0u);
}

static Lufia2ExecutionResult RollStatusMask(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child, void *context,
                                            uint8_t adding) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, STATUS_MASK));
    OpLdy(cpu, adding ? 0x78u : 0x7au);
    const uint32_t site = adding ? 0x85d43bu : 0x85d409u;
    do {
        OpLoadA(cpu, 100u);
        if (!ActionCall(memory, cpu, child, context, site, 0x808299u, 3u))
            return ActionUnwound(site);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, ActionField(memory, cpu)));
        for (unsigned step = 0u; step < 4u; ++step)
            OpDey(cpu);
        OpRolMem8(memory, cpu, OpDp(cpu, STATUS_MASK));
    } while (!cpu->carry);
    OpLda(memory, cpu, OpDp(cpu, STATUS_MASK));
    if (adding) {
        OpOraValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_STATUS)));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffu)));
    } else {
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffu)));
        OpAndValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_STATUS)));
    }
    OpSta(memory, cpu, OpAbs(cpu, adding ? ADDED_STATUS : REMOVED_STATUS));
    OpLoadA(cpu,
            (uint16_t)(OpA(cpu) ^ (OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_STATUS)))));
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
    return ExecutionReturned(0u);
}

static void ResetActorStatusAnimation(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpPushX(memory, cpu);
    OpLda(memory, cpu, ACTION_TARGET);
    OpSta(memory, cpu, OpDp(cpu, ACTOR_BIT));
    OpBitValue(cpu, 0x3fu);
    if (!cpu->zero) {
        OpAndValue(cpu, 0x80u);
        if (!cpu->zero)
            OpLoadA(cpu, 5u);
        OpSta(memory, cpu, OpDp(cpu, ACTOR_SIDE));
        OpLoadA(cpu, 0xffu);
        do {
            OpIncA(cpu);
            OpLsrMem(memory, cpu, OpDp(cpu, ACTOR_BIT));
        } while (!cpu->carry);
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, ACTOR_SIDE)));
    }
    OpSta(memory, cpu, OpAbs(cpu, WRMPYA));
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, WRMPYB));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x1499u);
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, RDMPYL)));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 3u);
    OpSta(memory, cpu, OpAbsX(cpu, 6u));
    OpPullX(memory, cpu);
}

static Lufia2ExecutionResult AddDamageIp(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu,
                                         Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffffu)));
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, VECTOR_COMPONENT));
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_HP)));
    OpPushX(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, VECTOR_LENGTH));
    OpStz(memory, cpu, OpDp(cpu, VECTOR_RESULT));
    if (!ActionCall(memory, cpu, child, context, 0x85d538u, 0x85db6du, 3u))
        return ActionUnwound(0x85d538u);
    OpLda(memory, cpu, OpDp(cpu, VECTOR_RESULT));
    OpSta(memory, cpu, OpDp(cpu, MULTIPLICAND));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x40u);
    OpSta(memory, cpu, OpDp(cpu, MULTIPLIER));
    if (!ActionCall(memory, cpu, child, context, 0x85d546u, 0x80834cu, 3u))
        return ActionUnwound(0x85d546u);
    OpLda(memory, cpu, OpDp(cpu, PRODUCT));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x80u);
    TransferDirectToA(cpu);
    OpAdcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, PRODUCT + 1u)));
    OpSta(memory, cpu, OpDp(cpu, SCALED_PRODUCT));
    if (!ActionCall(memory, cpu, child, context, 0x85d554u, 0x85de1eu, 3u))
        return ActionUnwound(0x85d554u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x100u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, VECTOR_RESULT)));
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, MULTIPLICAND));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, ACTION_TARGET);
    if (!ActionCall(memory, cpu, child, context, 0x85d569u, 0x85da5fu, 3u))
        return ActionUnwound(0x85d569u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, (uint16_t)(cpu->stack + 1u));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x33u));
    OpSepWidths(cpu, 0x20u);
    ExchangeAccumulatorBytes(cpu);
    if (!cpu->zero) {
        OpCmpValue(cpu, 200u);
        if (cpu->carry)
            OpLoadA(cpu, 199u);
        else
            ExchangeAccumulatorBytes(cpu);
    } else {
        ExchangeAccumulatorBytes(cpu);
    }
    OpSta(memory, cpu, OpAbs(cpu, WRMPYA));
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRMPYB));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, RDMPYL)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRDIVL), cpu->x);
    OpLoadA(cpu, 100u);
    OpSta(memory, cpu, OpAbs(cpu, WRDIVB));
    PushAccumulator8(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    OpPushX(memory, cpu);
    OpPullX(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, RDDIVL));
    OpBitValue(cpu, 0xff00u);
    if (!cpu->zero)
        OpLoadA(cpu, 0xffu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, MULTIPLIER));
    if (!ActionCall(memory, cpu, child, context, 0x85d5aeu, 0x80834cu, 3u))
        return ActionUnwound(0x85d5aeu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, PRODUCT + 1u));
    cpu->carry = 0u;
    OpAdcValue(cpu, 12u);
    OpCmpValue(cpu, 0x100u);
    if (cpu->carry)
        OpLoadA(cpu, 0xffu);
    OpSepWidths(cpu, 0x20u);
    OpPullX(memory, cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_IP)));
    if (cpu->carry)
        OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_IP));
    OpRepWidths(cpu, 0x20u);
    return ExecutionReturned(0u);
}

static Lufia2ExecutionResult ApplyHitPointChange(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context, uint8_t *defeated) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, ACTOR_HP));
    OpSta(memory, cpu, OpDp(cpu, ACTOR_SIDE));
    OpLda(memory, cpu, HIT_POINT_CHANGE);
    if (cpu->zero)
        return ExecutionReturned(0u);
    OpSta(memory, cpu, OpAbs(cpu, APPLIED_HP_CHANGE));
    if (!cpu->negative) {
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_HP)));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_MAX_HP)));
        if (cpu->carry) {
            OpLda(memory, cpu, OpAbsX(cpu, ACTOR_MAX_HP));
            cpu->carry = 1u;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_HP)));
        } else {
            OpLda(memory, cpu, HIT_POINT_CHANGE);
        }
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_HP)));
        OpSta(memory, cpu, OpAbsX(cpu, ACTOR_HP));
        return ExecutionReturned(0u);
    }
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_HP)));
    if (!cpu->carry) {
        OpLda(memory, cpu, OpAbsX(cpu, ACTOR_HP));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffffu)));
        OpIncA(cpu);
    } else {
        OpLda(memory, cpu, HIT_POINT_CHANGE);
    }
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_HP)));
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_HP));
    if (cpu->zero) {
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
        OpOraValue(cpu, 4u);
        OpSta(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
        OpLoadA(cpu, 4u);
        OpTestBits(memory, cpu, OpAbs(cpu, ADDED_STATUS), 1u);
        *defeated = 1u;
        return ExecutionReturned(0u);
    }
    OpLda(memory, cpu, ACTION_SOURCE);
    OpAndValue(cpu, 0x80u);
    if (!cpu->zero) {
        OpLda(memory, cpu, ACTION_TARGET);
        OpBitValue(cpu, 0x80u);
        if (cpu->zero) {
            OpLda(memory, cpu, HIT_POINT_CHANGE);
            if (cpu->negative)
                return AddDamageIp(memory, cpu, child, context);
        }
    }
    return ExecutionReturned(0u);
}

static void ApplyMagicPointChange(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, MAGIC_POINT_CHANGE);
    if (cpu->zero)
        return;
    OpSta(memory, cpu, OpAbs(cpu, APPLIED_MP_CHANGE));
    if (!cpu->negative) {
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_MP)));
        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_MAX_MP)));
        if (cpu->carry) {
            OpLda(memory, cpu, OpAbsX(cpu, ACTOR_MAX_MP));
            cpu->carry = 1u;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_MP)));
            OpSta(memory, cpu, MAGIC_POINT_CHANGE);
        } else {
            OpLda(memory, cpu, MAGIC_POINT_CHANGE);
        }
    } else {
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_MP)));
        if (!cpu->carry) {
            OpLda(memory, cpu, OpAbsX(cpu, ACTOR_MP));
            OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (0xffffu)));
            OpIncA(cpu);
            OpSta(memory, cpu, MAGIC_POINT_CHANGE);
        } else {
            OpLda(memory, cpu, OpAbsX(cpu, ACTOR_MP));
            if (cpu->zero)
                OpSta(memory, cpu, MAGIC_POINT_CHANGE);
            else
                OpLda(memory, cpu, MAGIC_POINT_CHANGE);
        }
    }
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, ACTOR_MP)));
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_MP));
}

static void ApplyAttributeChanges(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, SCRIPT_CONTEXT));
    const uint8_t scripted = !cpu->zero;
    OpRepWidths(cpu, 0x20u);
    OpPushX(memory, cpu);
    if (scripted) {
        TransferDirectToA(cpu);
        OpLdy(cpu, 0x20u);
        do {
            OpSta(memory, cpu, ActionField(memory, cpu));
            OpIny(cpu);
            OpIny(cpu);
            OpCpy(cpu, 0x2cu);
        } while (!cpu->zero);
    } else {
        OpLdy(cpu, 0x20u);
        do {
            OpLda(memory, cpu, ActionField(memory, cpu));
            OpSta(memory, cpu, OpDp(cpu, SCALED_PRODUCT));
            cpu->carry = 0u;
            OpAdcValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, 0x74u)));
            OpSta(memory, cpu, OpAbsX(cpu, 0x74u));
            OpInx(cpu);
            OpInx(cpu);
            for (unsigned step = 0u; step < 6u; ++step)
                OpIny(cpu);
            OpCpy(cpu, 0x2cu);
        } while (!cpu->zero);
    }
    do {
        OpLda(memory, cpu, ActionField(memory, cpu));
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu,
                                OpAbsX(cpu, scripted ? ACTOR_STAT_CHANGE : 0x74u)));
        OpSta(memory, cpu, OpAbsX(cpu, scripted ? ACTOR_STAT_CHANGE : 0x74u));
        OpInx(cpu);
        OpInx(cpu);
        for (unsigned step = 0u; step < 6u; ++step)
            OpIny(cpu);
        OpCpy(cpu, 0x56u);
    } while (!cpu->zero);
    OpPullX(memory, cpu);
    OpSepWidths(cpu, 0x20u);
}

static Lufia2ExecutionResult DefeatTarget(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu,
                                          Lufia2PushedChildCall child, void *context) {
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbsX(cpu, ACTOR_HP));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, ACTION_TARGET);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_MEMBER_INDEX));
    if (!ActionCall(memory, cpu, child, context, 0x85d6acu, 0x85d9f0u, 3u))
        return ActionUnwound(0x85d6acu);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, ACTION_RESULT));
    OpLda(memory, cpu, ACTION_TARGET);
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, 0x50u));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_MEMBER_INDEX));
        if (!ActionCall(memory, cpu, child, context, 0x85d6c3u, 0x81fbdbu, 3u))
            return ActionUnwound(0x85d6c3u);
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_BIAS));
        cpu->carry = 0u;
        OpAdcValue(cpu,
                   OpReadM(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EXPERIENCE_REWARD)));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EXPERIENCE_REWARD));
        if (cpu->carry)
            OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EXPERIENCE_REWARD + 2u), 1);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_LEVEL));
        cpu->carry = 0u;
        OpAdcValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_BATTLE_GOLD_REWARD)));
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_GOLD_REWARD));
        if (cpu->carry)
            OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_GOLD_REWARD + 2u), 1);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ITEM_REWARD));
        if (cpu->zero) {
            OpSepWidths(cpu, 0x20u);
            if (!ActionCall(memory, cpu, child, context, 0x85d6eeu, 0x81fb1au, 3u))
                return ActionUnwound(0x85d6eeu);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, WRAM_PARTY_STAT_LEVEL));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ITEM_REWARD));
        }
    } else {
        OpRepWidths(cpu, 0x20u);
        OpStz(memory, cpu, OpAbsX(cpu, ACTOR_IP));
        OpStepMem(memory, cpu, OpAbsX(cpu, 0xbau), 1);
    }
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    return ExecutionReturned(0x85d709u);
}

Lufia2ExecutionResult Lufia2BattleApplyTargetAction(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    const uint8_t bank = cpu->data_bank;
    if (!child || cpu->program_bank != 0x85u || cpu->direct_page || cpu->decimal ||
        !cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->stack < 0x1f20u ||
        cpu->stack > 0x1ffcu || !(bank < 0x40u || (bank >= 0x7eu && bank < 0xc0u)))
        return ExecutionHandoff(cpu, 0x85d0cdu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_WORK_BYTE));
    OpSta(memory, cpu, OpAbs(cpu, ACTION_RESULT));
    OpLda(memory, cpu, ACTION_TARGET);
    if (!ActionCall(memory, cpu, child, context, 0x85d0d7u, 0x85c168u, 2u))
        return ActionUnwound(0x85d0d7u);
    OpLda(memory, cpu, ACTION_FLAGS);
    OpBitValue(cpu, 1u);
    if (cpu->zero) {
        OpBitValue(cpu, 0x10u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
            OpBitValue(cpu, 0x80u);
            if (!cpu->zero) {
                OpLoadA(cpu, 3u);
                OpSta(memory, cpu, OpAbs(cpu, ACTION_RESULT));
                return RejectAction(memory, cpu);
            }
        }
        OpLda(memory, cpu, ACTION_FLAGS);
        OpBitValue(cpu, 0x20u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
            OpBitValue(cpu, 0x40u);
            if (!cpu->zero) {
                OpLoadA(cpu, 2u);
                OpSta(memory, cpu, OpAbs(cpu, ACTION_RESULT));
                return RejectAction(memory, cpu);
            }
        }
    }
    OpLda(memory, cpu, HIT_POINT_CHANGE + 1u);
    if (cpu->negative) {
        OpLda(memory, cpu, ACTION_SOURCE);
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ (OpReadM(memory, cpu, ACTION_TARGET))));
        OpLoadA(cpu, cpu->negative ? 20u : 100u);
        OpSta(memory, cpu, STATUS_CHANCE_A);
        OpSta(memory, cpu, STATUS_CHANCE_B);
    }
    Lufia2ExecutionResult result = RunTargetScripts(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, ACTION_INTERRUPTED);
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        OpLoadA(cpu, 3u);
        OpSta(memory, cpu, OpAbs(cpu, ACTION_RESULT));
        return RejectAction(memory, cpu);
    }
    OpLda(memory, cpu, HIT_POINT_VARIANCE);
    if (!ActionCall(memory, cpu, child, context, 0x85d1d0u, 0x808299u, 3u))
        return ActionUnwound(0x85d1d0u);
    OpSta(memory, cpu, HIT_POINT_VARIANCE);
    OpLda(memory, cpu, MAGIC_POINT_VARIANCE);
    if (!ActionCall(memory, cpu, child, context, 0x85d1dcu, 0x808299u, 3u))
        return ActionUnwound(0x85d1dcu);
    OpSta(memory, cpu, MAGIC_POINT_VARIANCE);
    OpLoadA(cpu, 100u);
    if (!ActionCall(memory, cpu, child, context, 0x85d1e6u, 0x808299u, 3u))
        return ActionUnwound(0x85d1e6u);
    OpCmpValue(cpu, OpReadM(memory, cpu, HIT_CHANCE));
    if (cpu->carry) {
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 16u);
        OpSta(memory, cpu, DAMAGE_SCALE);
    } else {
        OpLda(memory, cpu, ACTION_HIT_COUNTER);
        OpIncA(cpu);
        OpSta(memory, cpu, ACTION_HIT_COUNTER);
        OpRepWidths(cpu, 0x20u);
        TransferDirectToA(cpu);
        OpSta(memory, cpu, TARGET_DEFENSE);
    }
    result = ScaleHitPointChange(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = ScaleMagicPointChange(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = ScaleAttributeChanges(memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    result = RollStatusMask(memory, cpu, child, context, 0u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpLda(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
    OpBitValue(cpu, 4u);
    if (!cpu->zero) {
        OpLoadA(cpu, 0x80u);
        OpSta(memory, cpu, OpAbs(cpu, ACTION_RESULT));
        cpu->carry = 0u;
        return ExecutionReturned(0x85d710u);
    }
    result = RollStatusMask(memory, cpu, child, context, 1u);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    OpBitValue(cpu, 4u);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        TransferDirectToA(cpu);
        OpSta(memory, cpu, HIT_POINT_CHANGE);
        OpSepWidths(cpu, 0x20u);
        return DefeatTarget(memory, cpu, child, context);
    }
    OpLda(memory, cpu, OpAbs(cpu, ADDED_STATUS));
    OpBitValue(cpu, 0x80u);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, STATUS_MASK));
        OpBitValue(cpu, 0x80u);
        if (cpu->zero)
            ResetActorStatusAnimation(memory, cpu);
    } else {
        ResetActorStatusAnimation(memory, cpu);
    }
    uint8_t defeated = 0u;
    result = ApplyHitPointChange(memory, cpu, child, context, &defeated);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    if (defeated)
        return DefeatTarget(memory, cpu, child, context);
    ApplyMagicPointChange(memory, cpu);
    ApplyAttributeChanges(memory, cpu);
    if (!ActionCall(memory, cpu, child, context, 0x85d68au, 0x81f4d5u, 3u))
        return ActionUnwound(0x85d68au);
    cpu->carry = 0u;
    return ExecutionReturned(0x85d68fu);
}
