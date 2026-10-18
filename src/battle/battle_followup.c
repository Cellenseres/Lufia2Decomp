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

enum {
    TARGET_BIT = WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS & 0xffffu,
    TARGET_COUNT = WRAM_BATTLE_MESSAGE_WINDOW_ROWS & 0xffffu,
    ACTION_TARGET_COUNT = WRAM_BATTLE_ACTION_TARGET_COUNT & 0xffffu,
    DISPATCH_TARGET_COUNT = WRAM_BATTLE_DISPATCH_TARGET_COUNT & 0xffffu,
    SOURCE_ACTION = 0xb5u,
    WORKING_ACTION = 0xb8u,
    CURRENT_TARGET = 0xcau,
    SIGNED_OPERAND = 0x4eu,
    TARGET_FACTOR = 0x50u,
    SCALED_OPERAND = 0x52u,
    REQUESTED_ENEMY = 0x11u,
    REQUIRED_COLUMNS = 0x13u,
    LARGEST_FREE_RUN = 0x14u,
    COLUMN_WORKSPACE = WRAM_TEXT_WAIT_ACTOR & 0xffffu,
    ENEMY_RECORD_SLOTS = WRAM_BATTLE_ENEMY_SPRITE_RECORD_SLOTS & 0xffffu,
    ENEMY_COLUMN_ORIGINS = WRAM_BATTLE_ENEMY_SPRITE_X_POSITIONS & 0xffffu,
    ENEMY_COLUMN_WIDTHS = WRAM_BATTLE_ENEMY_ENCOUNTER_WIDTHS & 0xffffu
};

static uint8_t PlacementContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x85u && !cpu->direct_page && !cpu->decimal &&
           cpu->accumulator_is_8_bit && !cpu->index_is_8_bit && cpu->stack >= 0x1f20u &&
           cpu->stack <= 0x1ffcu;
}

static uint8_t WritableTargetBank(uint8_t bank) {
    return bank < 0x40u || (bank >= 0x7eu && bank < 0xc0u);
}

static uint8_t PlacementCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             Lufia2PushedChildCall child, void *context, uint32_t site,
                             uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, frame, 0x85u);
}

static Lufia2ExecutionResult PlacementUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void DivideTargetField(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVL));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, TARGET_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVB));
    OpRepWidths(cpu, 0x20u);
    OpPushX(memory, cpu);
    OpPullX(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDDIVL));
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, SOURCE_ACTION));
}

static uint32_t ScaleTargetFields(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                  Lufia2PushedChildCall child, void *context) {
    OpLdy(cpu, 20u);
    OpRepWidths(cpu, 0x20u);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SOURCE_ACTION));
        if (cpu->negative) {
            OpLoadA(cpu, (uint16_t)~OpA(cpu));
            OpIncA(cpu);
            OpSta(memory, cpu, OpDp(cpu, SIGNED_OPERAND));
            OpSepWidths(cpu, 0x20u);
            TransferDirectToA(cpu);
            OpLda(memory, cpu, OpAbs(cpu, TARGET_COUNT));
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, 0x859e9bu));
            OpSta(memory, cpu, OpDp(cpu, TARGET_FACTOR));
            if (!PlacementCall(memory, cpu, child, context, 0x85ced5u, 0x80834cu, 3u))
                return 0x85ced5u;
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpDp(cpu, SCALED_OPERAND));
            OpLoadA(cpu, (uint16_t)~OpA(cpu));
            OpIncA(cpu);
            OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, SOURCE_ACTION));
        } else {
            DivideTargetField(memory, cpu);
        }
        for (unsigned byte = 0u; byte < 6u; ++byte)
            OpIny(cpu);
        OpCpy(cpu, 92u);
    } while (!cpu->zero);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SOURCE_ACTION));
        DivideTargetField(memory, cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 136u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return 0u;
}

static void CopyTargetAction(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 135u);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SOURCE_ACTION));
        OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, WORKING_ACTION));
        OpDey(cpu);
    } while (!cpu->negative);
}

Lufia2ExecutionResult Lufia2BattlePrepareTargetedAction(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !PlacementContext(cpu) || !WritableTargetBank(cpu->data_bank))
        return ExecutionHandoff(cpu, 0x85ce6fu);
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
    OpWriteX(memory, cpu, OpAbs(cpu, TARGET_COUNT), cpu->y);
    OpWriteX(memory, cpu, OpAbs(cpu, ACTION_TARGET_COUNT), cpu->y);
    OpRepWidths(cpu, 0x10u);
    OpLdy(cpu, 12u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, SOURCE_ACTION));
    OpBitValue(cpu, 0x0100u);
    OpSepWidths(cpu, 0x20u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, TARGET_COUNT));
        OpCmpValue(cpu, 1u);
        if (!cpu->zero) {
            const uint32_t unwound = ScaleTargetFields(memory, cpu, child, context);
            if (unwound)
                return PlacementUnwound(unwound);
        }
    }
    for (;;) {
        OpLda(memory, cpu, WRAM_BATTLE_ACTION_INITIAL_TARGET_MASK);
        OpBit(memory, cpu, OpAbs(cpu, TARGET_BIT));
        if (!cpu->zero) {
            OpAndValue(cpu, 0x80u);
            OpOra(memory, cpu, OpAbs(cpu, TARGET_BIT));
            OpSta(memory, cpu, OpDp(cpu, CURRENT_TARGET));
            CopyTargetAction(memory, cpu);
            OpLda(memory, cpu, OpDp(cpu, CURRENT_TARGET));
            OpSta(memory, cpu, WRAM_BATTLE_ACTION_WORK + 2u);
            if (!PlacementCall(memory, cpu, child, context, 0x85cf2eu, 0x85d025u, 2u))
                return PlacementUnwound(0x85cf2eu);
            OpLda(memory, cpu, OpAbs(cpu, TARGET_COUNT));
            OpSta(memory, cpu, OpAbs(cpu, DISPATCH_TARGET_COUNT));
            if (!PlacementCall(memory, cpu, child, context, 0x85cf37u, 0x85d0cdu, 2u))
                return PlacementUnwound(0x85cf37u);
            if (cpu->carry) {
                if (!PlacementCall(memory, cpu, child, context, 0x85cf3cu, 0x85d94fu,
                                   2u))
                    return PlacementUnwound(0x85cf3cu);
                OpRepWidths(cpu, 0x20u);
                TransferDirectToA(cpu);
                OpSta(memory, cpu, WRAM_BATTLE_ACTION_WORK + 130u);
                OpSepWidths(cpu, 0x20u);
                OpLda(memory, cpu, WRAM_BATTLE_ACTION_WORK);
                OpSta(memory, cpu, WRAM_BATTLE_ACTION_WORK + 2u);
                if (!PlacementCall(memory, cpu, child, context, 0x85cf50u, 0x85d0cdu,
                                   2u))
                    return PlacementUnwound(0x85cf50u);
                if (cpu->carry)
                    return ExecutionHandoff(cpu, 0x85cf55u);
            }
            if (!PlacementCall(memory, cpu, child, context, 0x85cf56u, 0x85d94fu, 2u))
                return PlacementUnwound(0x85cf56u);
        }
        OpLda(memory, cpu, OpAbs(cpu, TARGET_BIT));
        OpAslA(cpu);
        OpCmpValue(cpu, 0x40u);
        if (cpu->zero)
            return ExecutionReturned(0x85cf67u);
        OpSta(memory, cpu, OpAbs(cpu, TARGET_BIT));
    }
}

static void RoundEnemyWidth(Lufia2CpuState *cpu) {
    OpIncA(cpu);
    OpAndValue(cpu, 0xfeu);
    OpIncA(cpu);
}

static void MarkEnemyColumns(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 31u);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, COLUMN_WORKSPACE));
        OpDex(cpu);
    } while (!cpu->negative);
    TransferDirectToA(cpu);
    OpLdy(cpu, 5u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_RECORD_SLOTS));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, ENEMY_COLUMN_ORIGINS));
            for (unsigned bit = 0u; bit < 3u; ++bit)
                OpLsrA(cpu);
            OpTax(cpu);
            OpLda(memory, cpu, OpAbsY(cpu, ENEMY_COLUMN_WIDTHS));
            RoundEnemyWidth(cpu);
            OpSta(memory, cpu, OpDp(cpu, REQUIRED_COLUMNS));
            OpLoadA(cpu, 0xffu);
            do {
                OpSta(memory, cpu, OpAbsX(cpu, COLUMN_WORKSPACE));
                OpInx(cpu);
                OpStepMem(memory, cpu, OpDp(cpu, REQUIRED_COLUMNS), -1);
            } while (!cpu->zero);
        }
        OpDey(cpu);
    } while (!cpu->negative);
}

static void MeasureFreeColumns(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, LARGEST_FREE_RUN));
    OpLdx(cpu, 31u);
    OpLoadA(cpu, 1u);
    ExchangeAccumulatorBytes(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, COLUMN_WORKSPACE));
        if (cpu->zero) {
            ExchangeAccumulatorBytes(cpu);
            OpSta(memory, cpu, OpAbsX(cpu, COLUMN_WORKSPACE));
            OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, LARGEST_FREE_RUN)));
            if (cpu->carry)
                OpSta(memory, cpu, OpDp(cpu, LARGEST_FREE_RUN));
            OpIncA(cpu);
            ExchangeAccumulatorBytes(cpu);
        } else {
            OpLoadA(cpu, 1u);
            ExchangeAccumulatorBytes(cpu);
        }
        OpDex(cpu);
        OpCpx(cpu, 7u);
    } while (!cpu->zero);
}

static void RetainPlacementColumns(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 8u);
    OpLoadA(cpu, 0xffu);
    ExchangeAccumulatorBytes(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, COLUMN_WORKSPACE));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, REQUIRED_COLUMNS)));
            if (cpu->zero) {
                OpStz(memory, cpu, OpAbsX(cpu, COLUMN_WORKSPACE));
            } else if (cpu->carry) {
                ExchangeAccumulatorBytes(cpu);
                OpCmpValue(cpu, 0xffu);
                if (cpu->zero)
                    OpStz(memory, cpu, OpAbsX(cpu, COLUMN_WORKSPACE));
            }
        }
        OpLda(memory, cpu, OpAbsX(cpu, COLUMN_WORKSPACE));
        ExchangeAccumulatorBytes(cpu);
        OpInx(cpu);
        OpCpx(cpu, 32u);
    } while (!cpu->zero);
}

Lufia2ExecutionResult
Lufia2BattleFindEnemyGraphicsPlacement(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       Lufia2PushedChildCall child, void *context) {
    if (!child || !PlacementContext(cpu))
        return ExecutionHandoff(cpu, 0x85e02du);
    OpSta(memory, cpu, OpDp(cpu, REQUESTED_ENEMY));
    MarkEnemyColumns(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, REQUESTED_ENEMY));
    if (!PlacementCall(memory, cpu, child, context, 0x85e063u, 0x81fbc6u, 3u))
        return PlacementUnwound(0x85e063u);
    if (!PlacementCall(memory, cpu, child, context, 0x85e067u, 0x81fb8eu, 3u))
        return PlacementUnwound(0x85e067u);
    RoundEnemyWidth(cpu);
    OpSta(memory, cpu, OpDp(cpu, REQUIRED_COLUMNS));
    MeasureFreeColumns(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, LARGEST_FREE_RUN));
    OpCmpValue(cpu, OpReadM(memory, cpu, OpDp(cpu, REQUIRED_COLUMNS)));
    if (!cpu->carry) {
        OpLoadA(cpu, 0xffu);
        return ExecutionReturned(0x85e09du);
    }
    RetainPlacementColumns(memory, cpu);
    OpLdx(cpu, 20u);
    OpLdy(cpu, 19u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, COLUMN_WORKSPACE));
        if (cpu->zero) {
            OpTxa(cpu);
            return ExecutionReturned(0x85e0e3u);
        }
        OpLda(memory, cpu, OpAbsY(cpu, COLUMN_WORKSPACE));
        if (cpu->zero) {
            OpTya(cpu);
            return ExecutionReturned(0x85e0e3u);
        }
        OpDey(cpu);
        OpInx(cpu);
        OpCpx(cpu, 32u);
    } while (!cpu->zero);
    return ExecutionHandoff(cpu, 0x85e0e0u);
}
