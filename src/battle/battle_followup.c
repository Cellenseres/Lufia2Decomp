#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    RECOVERY_FIRST_MASK = 0x00u,
    RECOVERY_SECOND_MASK = 0x01u,
    RECOVERY_ACTOR_MASK = 0x03u,
    ACTOR_STATUS = 15u,
    ENEMY_POINTERS = 0x0a6eu,
    ENEMY_GRAPHICS_GROUPS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_IDS & 0xffffu,
    ENEMY_GRAPHICS_WEIGHTS = 0x1312u,
    REQUESTED_GRAPHICS = 0x00u,
    GRAPHICS_LIMIT = 65u
};

static uint8_t BattleFollowupContext(const Lufia2CpuState *cpu, unsigned any_width) {
    return cpu->program_bank == 0x85u && !cpu->direct_page && !cpu->decimal &&
           (any_width || (cpu->accumulator_is_8_bit && !cpu->index_is_8_bit)) &&
           cpu->stack >= 0x1f20u && cpu->stack <= 0x1ffcu;
}

static uint8_t BattleFollowupCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  Lufia2PushedChildCall child, void *context,
                                  uint32_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x85u);
}

static Lufia2ExecutionResult FollowupUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult
Lufia2BattleResolveRecoverableStatuses(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    static const struct {
        uint8_t flag, phrase;
        uint32_t lookup, random, name, message, display;
    } recovery[] = {{0x20u, 1u, 0x858fc4u, 0x858fcbu, 0x858fdbu, 0x858fe1u, 0x858fe5u},
                    {0x10u, 3u, 0x858fefu, 0x858ff6u, 0x859006u, 0x85900cu, 0x859010u},
                    {0x08u, 5u, 0x85901au, 0x859021u, 0x859031u, 0x859037u, 0x85903bu}};
    if (!child || !BattleFollowupContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x858f8au);
    OpLda(memory, cpu, OpDp(cpu, RECOVERY_FIRST_MASK));
    OpOraValue(cpu, OpReadM(memory, cpu, OpDp(cpu, RECOVERY_SECOND_MASK)));
    OpSta(memory, cpu, OpDp(cpu, RECOVERY_ACTOR_MASK));
    if (!BattleFollowupCall(memory, cpu, child, context, 0x858f90u, 0x85d9c9u, 3u))
        return FollowupUnwound(0x858f90u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLdx(cpu, 30u);
    do {
        OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpDex(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, RECOVERY_ACTOR_MASK));
    if (!BattleFollowupCall(memory, cpu, child, context, 0x858fa7u, 0x81b2b5u, 3u))
        return FollowupUnwound(0x858fa7u);
    OpCpx(cpu, 0u);
    if (cpu->zero)
        return ExecutionReturned(0x85903eu);
    OpLda(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
    OpBitValue(cpu, 4u);
    if (!cpu->zero)
        return ExecutionReturned(0x85903eu);
    for (unsigned status = 0u; status < 3u; ++status) {
        OpBitValue(cpu, recovery[status].flag);
        if (cpu->zero)
            continue;
        OpTxy(cpu);
        OpLda(memory, cpu, OpDp(cpu, RECOVERY_ACTOR_MASK));
        if (!BattleFollowupCall(memory, cpu, child, context, recovery[status].lookup,
                                0x81b2dbu, 3u))
            return FollowupUnwound(recovery[status].lookup);
        OpTyx(cpu);
        OpLoadA(cpu, 7u);
        if (!BattleFollowupCall(memory, cpu, child, context, recovery[status].random,
                                0x808299u, 3u))
            return FollowupUnwound(recovery[status].random);
        OpCmpValue(cpu, 4u);
        if (!cpu->zero)
            continue;
        OpLda(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
        OpAndValue(cpu, (uint8_t)~recovery[status].flag);
        OpSta(memory, cpu, OpAbsX(cpu, ACTOR_STATUS));
        if (!BattleFollowupCall(memory, cpu, child, context, recovery[status].name,
                                0x859150u, 3u))
            return FollowupUnwound(recovery[status].name);
        OpLoadA(cpu, recovery[status].phrase);
        if (!BattleFollowupCall(memory, cpu, child, context, recovery[status].message,
                                0x859173u, 3u))
            return FollowupUnwound(recovery[status].message);
        if (!BattleFollowupCall(memory, cpu, child, context, recovery[status].display,
                                0x85903fu, 2u))
            return FollowupUnwound(recovery[status].display);
    }
    return ExecutionReturned(0x85903eu);
}

static void RestoreEnemyCapacityRegisters(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult
Lufia2BattleCheckEnemyGraphicsCapacity(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    if (!child || !BattleFollowupContext(cpu, 1u))
        return ExecutionHandoff(cpu, 0x85e0e4u);
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, REQUESTED_GRAPHICS));
    if (!BattleFollowupCall(memory, cpu, child, context, 0x85e0eeu, 0x85dfb9u, 3u))
        return FollowupUnwound(0x85e0eeu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 10u);
    bool free_actor = false;
    do {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, ENEMY_POINTERS)));
        if (cpu->zero) {
            free_actor = true;
            break;
        }
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    bool available = false;
    if (free_actor) {
        OpLdx(cpu, 2u);
        bool resident = false;
        do {
            OpLda(memory, cpu, OpAbsX(cpu, ENEMY_GRAPHICS_GROUPS));
            OpCmp(memory, cpu, OpDp(cpu, REQUESTED_GRAPHICS));
            if (cpu->zero) {
                resident = true;
                break;
            }
            OpDex(cpu);
        } while (!cpu->negative);
        bool free_graphics = resident;
        if (!resident) {
            OpLdx(cpu, 2u);
            do {
                OpLda(memory, cpu, OpAbsX(cpu, ENEMY_GRAPHICS_GROUPS));
                OpCmpValue(cpu, 0xffu);
                if (cpu->zero) {
                    free_graphics = true;
                    break;
                }
                OpDex(cpu);
            } while (!cpu->negative);
            if (free_graphics) {
                OpLda(memory, cpu, OpDp(cpu, REQUESTED_GRAPHICS));
                if (!BattleFollowupCall(memory, cpu, child, context, 0x85e122u,
                                        0x81fbc6u, 3u))
                    return FollowupUnwound(0x85e122u);
                if (!BattleFollowupCall(memory, cpu, child, context, 0x85e126u,
                                        0x81fb8eu, 3u))
                    return FollowupUnwound(0x85e126u);
                OpRepWidths(cpu, 0x20u);
                cpu->carry = 0u;
                OpAdcValue(cpu, 0x0101u);
                OpLsrA(cpu);
                OpAndValue(cpu, 0x7f7fu);
                OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
                OpSepWidths(cpu, 0x20u);
                TransferDirectToA(cpu);
                OpLdx(cpu, 2u);
                do {
                    OpLda(memory, cpu, OpAbsX(cpu, ENEMY_GRAPHICS_GROUPS));
                    OpCmpValue(cpu, 0xffu);
                    if (!cpu->zero) {
                        ExchangeAccumulatorBytes(cpu);
                        cpu->carry = 0u;
                        OpAdc(memory, cpu, OpAbsX(cpu, ENEMY_GRAPHICS_WEIGHTS));
                        ExchangeAccumulatorBytes(cpu);
                    }
                    OpDex(cpu);
                } while (!cpu->negative);
                ExchangeAccumulatorBytes(cpu);
                cpu->carry = 0u;
                OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
                OpCmpValue(cpu, GRAPHICS_LIMIT);
                free_graphics = !cpu->carry;
            }
        }
        if (free_graphics) {
            OpLda(memory, cpu, OpDp(cpu, REQUESTED_GRAPHICS));
            if (!BattleFollowupCall(memory, cpu, child, context, 0x85e158u, 0x85e02du,
                                    3u))
                return FollowupUnwound(0x85e158u);
            OpCmpValue(cpu, 0xffu);
            available = !cpu->zero;
        }
    }
    RestoreEnemyCapacityRegisters(memory, cpu);
    cpu->carry = available ? 0u : 1u;
    return ExecutionReturned(available ? 0x85e169u : 0x85e173u);
}
