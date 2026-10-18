#include "lufia2/battle.h"
#include "item/menu_application_internal.h"
#include "core/snes_registers.h"
#include "system/wram.h"

enum {
    TARGET_SAVED_ACTION = 0xb5u,
    TARGET_WORKING_ACTION = 0xb8u,
    TARGET_CURRENT_ACTOR = 0xcau,
    TARGET_BIT = WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS,
    TARGET_DIVISOR = WRAM_BATTLE_MESSAGE_WINDOW_ROWS,
    TARGET_POSITIVE_CHANGES = WRAM_BATTLE_ACTION_WORK_BYTE,
    TARGET_NEGATIVE_CHANGES = WRAM_MENU_EFFECT_NEGATIVE_STATS,
    TARGET_STAT_BIT = WRAM_MENU_EFFECT_STAT_BIT,
    TARGET_REQUESTED_STATS = WRAM_PARTY_STAT_MEMBER_INDEX,
    TARGET_ALLOWED_STATS = WRAM_BATTLE_ACTION_BIAS,
    TARGET_ALLOWED_STATUSES = WRAM_PARTY_STAT_LEVEL,
    TARGET_REQUESTED_STATUSES = WRAM_MENU_EFFECT_REQUESTED_STATUSES,
    TARGET_CURRENT_HP = 0x11u,
    TARGET_CURRENT_MP = 0x13u,
    TARGET_MAXIMUM_HP = 0x25u,
    TARGET_MAXIMUM_MP = 0x27u,
    TARGET_FIRST_STAT = 0x2du,
    TARGET_AGILITY = 0x33u,
    TARGET_STATUS = 0x0fu,
    TARGET_DOWNED = 4u,
    TARGET_ACTION_KIND = 14u,
    TARGET_STATUS_REMOVE = 102u,
    TARGET_LAST_RECORD_BYTE = 329u,
    TARGET_LAST_ACTION_BYTE = 135u,
    TARGET_BATTLER = WRAM_BATTLE_ACTION_WORK + 2u,
    TARGET_SPECIAL_EFFECT_A = WRAM_BATTLE_ACTION_WORK + 32u,
    TARGET_SPECIAL_EFFECT_B = WRAM_BATTLE_ACTION_WORK + 38u
};

static void RotateMaskByte(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t address) {
    const uint32_t at = OpAbs(cpu, address);
    const uint8_t old = Read8(memory, at);
    const uint8_t value = (uint8_t)((old >> 1u) | (cpu->carry ? 0x80u : 0u));
    cpu->carry = old & 1u;
    Write8(memory, at, value);
    SetNz8(cpu, value);
}

static void DivideSavedEffectField(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, TARGET_SAVED_ACTION));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVL));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, TARGET_DIVISOR));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVB));
    OpRepWidths(cpu, 0x20u);
    OpPushX(memory, cpu);
    OpPullX(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDDIVL));
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, TARGET_SAVED_ACTION));
}

Lufia2ExecutionResult Lufia2BattleCheckAnyMenuEffectTarget(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu,
                                                           Lufia2PushedChildCall child,
                                                           void *context) {
    if (!MenuUseReady(cpu, child, 0x85u))
        return ExecutionHandoff(cpu, 0x85d711u);
    MenuUseCall call = {memory, cpu, child, context, 0x85u, {0}};
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    TransferDirectToA(cpu);
    OpLdx(cpu, TARGET_LAST_RECORD_BYTE);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_BATTLE_ACTION_RESULT_RECORDS));
        OpDex(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x10u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_BIT));
    OpLda(memory, cpu, WRAM_BATTLE_ACTION_INITIAL_TARGET_MASK);
    OpLdx(cpu, 6u);
    OpLdy(cpu, 0u);
    do {
        OpLsrA(cpu);
        if (cpu->carry)
            OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_DIVISOR));
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, TARGET_SAVED_ACTION + 2u));
    OpSta(memory, cpu, OpDp(cpu, TARGET_WORKING_ACTION + 2u));
    OpLdy(cpu, WRAM_BATTLE_ACTION_SAVED_TARGET_WORD & 0xffffu);
    OpWriteX(memory, cpu, OpDp(cpu, TARGET_SAVED_ACTION), cpu->y);
    OpLdy(cpu, WRAM_BATTLE_ACTION_WORK & 0xffffu);
    OpWriteX(memory, cpu, OpDp(cpu, TARGET_WORKING_ACTION), cpu->y);
    OpLdy(cpu, TARGET_ACTION_KIND);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, TARGET_SAVED_ACTION));
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        OpLdy(cpu, 20u);
        OpRepWidths(cpu, 0x20u);
        do {
            DivideSavedEffectField(memory, cpu);
            OpLdy(cpu, (uint16_t)(cpu->y + 4u));
            OpCpy(cpu, 92u);
        } while (!cpu->zero);
        do {
            DivideSavedEffectField(memory, cpu);
            OpLdy(cpu, (uint16_t)(cpu->y + 2u));
            OpCpy(cpu, 136u);
        } while (!cpu->zero);
        OpSepWidths(cpu, 0x20u);
    }
    do {
        OpLda(memory, cpu, WRAM_BATTLE_ACTION_INITIAL_TARGET_MASK);
        OpBit(memory, cpu, OpAbs(cpu, TARGET_BIT));
        if (!cpu->zero) {
            OpAndValue(cpu, 0x80u);
            OpOra(memory, cpu, OpAbs(cpu, TARGET_BIT));
            OpSta(memory, cpu, OpDp(cpu, TARGET_CURRENT_ACTOR));
            OpLdy(cpu, TARGET_LAST_ACTION_BYTE);
            do {
                OpLda(memory, cpu,
                      DirectLongIndirectY(memory, cpu, TARGET_SAVED_ACTION));
                OpSta(memory, cpu,
                      DirectLongIndirectY(memory, cpu, TARGET_WORKING_ACTION));
                OpDey(cpu);
            } while (!cpu->negative);
            OpLda(memory, cpu, OpDp(cpu, TARGET_CURRENT_ACTOR));
            OpSta(memory, cpu, TARGET_BATTLER);
            if (!MenuUseChild(&call, 0x85d7b8u, 0x85d7ceu, 2u, 1u))
                return call.result;
            if (cpu->carry) {
                PullDataBank(memory, cpu);
                return ExecutionReturned(0x85d7cdu);
            }
        }
        OpLda(memory, cpu, OpAbs(cpu, TARGET_BIT));
        OpAslA(cpu);
        OpCmpValue(cpu, 64u);
        if (cpu->zero)
            break;
        OpSta(memory, cpu, OpAbs(cpu, TARGET_BIT));
    } while (true);
    cpu->carry = 0u;
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85d7cdu);
}

Lufia2ExecutionResult Lufia2BattleCheckMenuEffectTarget(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!MenuUseReady(cpu, child, 0x85u))
        return ExecutionHandoff(cpu, 0x85d7ceu);
    MenuUseCall call = {memory, cpu, child, context, 0x85u, {0}};
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, TARGET_REQUESTED_STATS), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, TARGET_ALLOWED_STATS), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, TARGET_ALLOWED_STATUSES), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, TARGET_REQUESTED_STATUSES), cpu->x);
    OpLda(memory, cpu, TARGET_BATTLER);
    if (!MenuUseChild(&call, 0x85d7e1u, 0x85c168u, 2u, 1u) ||
        !MenuUseChild(&call, 0x85d7e4u, 0x85d8b9u, 2u, 1u))
        return call.result;
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_STATUS));
    OpBitValue(cpu, TARGET_DOWNED);
    if (!cpu->zero) {
        OpLdy(cpu, TARGET_STATUS_REMOVE);
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, TARGET_WORKING_ACTION));
        cpu->carry = !cpu->zero;
    } else {
        if (!MenuUseChild(&call, 0x85d7f7u, 0x85d82cu, 2u, 1u))
            return call.result;
        const uint16_t pairs[][2] = {
            {TARGET_REQUESTED_STATS, TARGET_ALLOWED_STATS},
            {TARGET_REQUESTED_STATS + 1u, TARGET_ALLOWED_STATS + 1u},
            {TARGET_REQUESTED_STATUSES, TARGET_ALLOWED_STATUSES},
            {TARGET_REQUESTED_STATUSES + 1u, TARGET_ALLOWED_STATUSES + 1u}};
        for (unsigned pair = 0u; pair < 4u; ++pair) {
            OpLda(memory, cpu, OpAbs(cpu, pairs[pair][0]));
            OpAndValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, pairs[pair][1])));
            if (!cpu->zero) {
                cpu->carry = 1u;
                return ExecutionReturned(0x85d82bu);
            }
        }
        OpLda(memory, cpu, TARGET_SPECIAL_EFFECT_A);
        if (cpu->zero)
            OpLda(memory, cpu, TARGET_SPECIAL_EFFECT_B);
        cpu->carry = !cpu->zero;
    }
    return ExecutionReturned(cpu->carry ? 0x85d82bu : 0x85d827u);
}

Lufia2ExecutionResult Lufia2BattleAddRequestedStatChange(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    (void)context;
    if (!child || cpu->program_bank != 0x85u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x85d8a1u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, TARGET_WORKING_ACTION));
    if (!cpu->zero) {
        const uint16_t changes =
            cpu->negative ? TARGET_NEGATIVE_CHANGES : TARGET_POSITIVE_CHANGES;
        OpLda(memory, cpu, OpAbs(cpu, TARGET_STAT_BIT));
        OpTestBits(memory, cpu, OpAbs(cpu, changes), 1u);
    }
    const uint32_t at = OpAbs(cpu, TARGET_STAT_BIT);
    const uint16_t previous = OpRead16(memory, at);
    const uint16_t next = (uint16_t)(previous << 1u);
    cpu->carry = (previous & 0x8000u) != 0u;
    Write8(memory, OpNextByte(at), (uint8_t)(next >> 8u));
    Write8(memory, at, (uint8_t)next);
    SetNz16(cpu, next);
    return ExecutionReturned(0x85d8b8u);
}

Lufia2ExecutionResult Lufia2BattleBuildRequestedEffectMasks(const Lufia2Memory *memory,
                                                            Lufia2CpuState *cpu,
                                                            Lufia2PushedChildCall child,
                                                            void *context) {
    if (!MenuUseReady(cpu, child, 0x85u))
        return ExecutionHandoff(cpu, 0x85d82cu);
    MenuUseCall call = {memory, cpu, child, context, 0x85u, {0}};
    OpPushX(memory, cpu);
    OpLdy(cpu, 92u);
    TransferDirectToA(cpu);
    do {
        OpCmpValue(cpu,
                   OpReadM(memory, cpu,
                           DirectLongIndirectY(memory, cpu, TARGET_WORKING_ACTION)));
        RotateMaskByte(memory, cpu, TARGET_REQUESTED_STATUSES);
        OpLdy(cpu, (uint16_t)(cpu->y + 4u));
        OpCpy(cpu, 124u);
    } while (!cpu->zero);
    OpLdy(cpu, 94u);
    do {
        OpCmpValue(cpu,
                   OpReadM(memory, cpu,
                           DirectLongIndirectY(memory, cpu, TARGET_WORKING_ACTION)));
        RotateMaskByte(memory, cpu, TARGET_REQUESTED_STATUSES + 1u);
        OpLdy(cpu, (uint16_t)(cpu->y + 4u));
        OpCpy(cpu, 126u);
    } while (!cpu->zero);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, TARGET_REQUESTED_STATUSES));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
    OpSta(memory, cpu, OpAbs(cpu, TARGET_REQUESTED_STATUSES));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_STAT_BIT));
    OpStz(memory, cpu, OpAbs(cpu, TARGET_POSITIVE_CHANGES));
    OpStz(memory, cpu, OpAbs(cpu, TARGET_NEGATIVE_CHANGES));
    const uint16_t changes[] = {20u, 26u, 56u, 62u, 68u, 74u, 80u};
    for (unsigned change = 0u; change < 7u; ++change) {
        OpLdy(cpu, changes[change]);
        if (!MenuUseChild(&call, 0x85d86au + change * 6u, 0x85d8a1u, 2u, 0u))
            return call.result;
    }
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, TARGET_POSITIVE_CHANGES));
    OpSta(memory, cpu, OpAbs(cpu, TARGET_REQUESTED_STATS));
    OpLda(memory, cpu, OpAbs(cpu, TARGET_NEGATIVE_CHANGES));
    OpSta(memory, cpu, OpAbs(cpu, TARGET_REQUESTED_STATS + 1u));
    OpPullX(memory, cpu);
    return ExecutionReturned(0x85d8a0u);
}

Lufia2ExecutionResult Lufia2BattleBuildTargetEffectMasks(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    (void)context;
    if (!MenuUseReady(cpu, child, 0x85u))
        return ExecutionHandoff(cpu, 0x85d8b9u);
    OpPushX(memory, cpu);
    TransferDirectToA(cpu);
    OpRepWidths(cpu, 0x20u);
    const uint16_t current[] = {TARGET_CURRENT_HP,      TARGET_CURRENT_MP,
                                TARGET_FIRST_STAT,      TARGET_FIRST_STAT + 2u,
                                TARGET_FIRST_STAT + 4u, TARGET_AGILITY,
                                TARGET_FIRST_STAT + 8u};
    for (unsigned field = 0u; field < 7u; ++field) {
        OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, current[field])));
        RorAbsolute16(memory, cpu, TARGET_REQUESTED_STATS);
    }
    OpLsrMem(memory, cpu, OpAbs(cpu, TARGET_REQUESTED_STATS));
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_CURRENT_HP));
    OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, TARGET_MAXIMUM_HP)));
    RorAbsolute16(memory, cpu, TARGET_REQUESTED_STATS);
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_CURRENT_MP));
    OpCmpValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, TARGET_MAXIMUM_MP)));
    RorAbsolute16(memory, cpu, TARGET_REQUESTED_STATS);
    for (uint16_t field = TARGET_FIRST_STAT; field < TARGET_FIRST_STAT + 10u;
         field += 2u) {
        OpLda(memory, cpu, OpAbsX(cpu, field));
        OpCmpValue(cpu, field == TARGET_AGILITY ? 199u : 999u);
        RorAbsolute16(memory, cpu, TARGET_REQUESTED_STATS);
    }
    RorAbsolute16(memory, cpu, TARGET_REQUESTED_STATS);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, TARGET_REQUESTED_STATS));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_ALLOWED_STATS + 1u));
    OpLda(memory, cpu, OpAbs(cpu, TARGET_REQUESTED_STATS + 1u));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_ALLOWED_STATS));
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_STATUS));
    OpSta(memory, cpu, OpAbs(cpu, TARGET_ALLOWED_STATUSES + 1u));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffu));
    OpSta(memory, cpu, OpAbs(cpu, TARGET_ALLOWED_STATUSES));
    OpPullX(memory, cpu);
    return ExecutionReturned(0x85d94eu);
}
