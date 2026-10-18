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

enum {
    ENEMY_SORT_KEYS = WRAM_TEXT_WAIT_ACTOR,
    ENEMY_SORT_ORDER = WRAM_TEXT_PAUSE_ELAPSED,
    ENEMY_TARGET_BITS = 0x1275u,
    ENEMY_SORT_BUFFER = 0xdf00u,
    ENEMY_POINTER_TABLE = 0x859ec8u,
    ENEMY_DISPLAY_FIELDS = WRAM_BATTLE_ENEMY_SPRITE_RECORD_SLOTS,
    ENEMY_DISPLAY_ROWS = WRAM_BATTLE_ENEMY_SPRITE_Y_POSITIONS,
    ENEMY_DISPLAY_HEIGHTS = WRAM_BATTLE_ENEMY_ENCOUNTER_HEIGHTS,
    ENEMY_DISPLAY_IDS = WRAM_BATTLE_ENEMY_SPRITE_GROUP_KEYS,
    ENEMY_DISPLAY_FLAGS = WRAM_BATTLE_ENEMY_GROUP_FLAGS,
    ENEMY_AUXILIARY = WRAM_BATTLE_ENEMY_AUXILIARY_RECORDS,
    ENEMY_ACTIONS = WRAM_BATTLE_ENEMY_ACTION_RECORDS,
    ENEMY_BATTLERS = 0x161bu,
    ENEMY_SPRITE_BYTES = 15u,
    ENEMY_BATTLER_BYTES = 190u,
    ENEMY_ACTION_BYTES = 12u,
    ENEMY_AUXILIARY_BYTES = 7u,
    ENEMY_COUNT = 6u,
    DP_ENEMY_ORDER_LIMIT = 0x13u,
    DP_ENEMY_COPY_COUNT = 0x14u,
    DP_ENEMY_RECORD_BYTES = 0x15u,
    DP_INSERTED_ENEMY = 0u,
    DP_ENEMY_VARIANT_CHANCE = 1u,
    DP_ENEMY_COLUMN = 2u,
    DP_ENEMY_PIXEL_COLUMN = 3u,
    DP_INSERTED_ENEMY_SLOT = 4u,
    DP_ENEMY_SPRITES_LEFT = 5u,
    DP_INSERTED_ENEMY_POINTER = 6u,
    DP_ENEMY_TILEMAP_ADDRESS = 8u,
    DP_ENEMY_RECORD_POINTER = 0xb2u
};

static void SortEnemyColumns(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, ENEMY_DISPLAY_FIELDS));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero)
            OpLda(memory, cpu, OpAbsX(cpu, ENEMY_COLUMN_ORIGINS));
        else
            OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, ENEMY_SORT_KEYS));
        OpTxa(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, ENEMY_SORT_ORDER));
        OpInx(cpu);
        OpCpx(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
    OpLdx(cpu, ENEMY_COUNT - 1u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT), cpu->x);
    do {
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, ENEMY_SORT_KEYS));
            OpCmp(memory, cpu, OpAbsX(cpu, ENEMY_SORT_KEYS + 1u));
            if (cpu->carry) {
                OpRepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpAbsX(cpu, ENEMY_SORT_KEYS));
                ExchangeAccumulatorBytes(cpu);
                OpSta(memory, cpu, OpAbsX(cpu, ENEMY_SORT_KEYS));
                OpLda(memory, cpu, OpAbsX(cpu, ENEMY_SORT_ORDER));
                ExchangeAccumulatorBytes(cpu);
                OpSta(memory, cpu, OpAbsX(cpu, ENEMY_SORT_ORDER));
                OpSepWidths(cpu, 0x20u);
            }
            OpInx(cpu);
            OpCpx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT)));
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 1u);
    OpLdx(cpu, 0u);
    do {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, ENEMY_SORT_ORDER)));
        OpSta(memory, cpu, OpAbsY(cpu, ENEMY_TARGET_BITS));
        OpAslA(cpu);
        OpInx(cpu);
        OpCpx(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
}

static void ReorderEnemyDisplayFields(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 47u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, ENEMY_DISPLAY_FIELDS));
        OpSta(memory, cpu, OpAbsX(cpu, ENEMY_SORT_BUFFER));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdy(cpu, 0u);
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, ENEMY_SORT_ORDER)));
        for (unsigned field = 0u; field < 8u; ++field) {
            OpLda(memory, cpu,
                  OpAbsX(cpu, (uint16_t)(ENEMY_SORT_BUFFER + field * ENEMY_COUNT)));
            OpSta(memory, cpu,
                  OpAbsY(cpu, (uint16_t)(ENEMY_DISPLAY_FIELDS + field * ENEMY_COUNT)));
        }
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
}

static void ReorderEnemyAuxiliaryRecords(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu) {
    OpLdx(cpu, 41u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, ENEMY_AUXILIARY));
        OpSta(memory, cpu, OpAbsX(cpu, ENEMY_SORT_BUFFER));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLoadA(cpu, ENEMY_AUXILIARY_BYTES);
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT), cpu->y);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_SORT_ORDER));
        OpSta(memory, cpu, SNES_WRMPYB);
        PushY(memory, cpu);
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT)));
        OpLda(memory, cpu, SNES_RDMPYL);
        OpTax(cpu);
        OpLoadA(cpu, ENEMY_AUXILIARY_BYTES);
        OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_COPY_COUNT));
        do {
            OpLda(memory, cpu, OpAbsX(cpu, ENEMY_SORT_BUFFER));
            OpSta(memory, cpu, OpAbsY(cpu, ENEMY_AUXILIARY));
            OpInx(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_ENEMY_COPY_COUNT), -1);
        } while (!cpu->zero);
        OpWriteX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT), cpu->y);
        OpPullY(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
}

static void ReorderEnemyBattlers(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, ENEMY_COUNT - 1u);
    OpLdx(cpu, 10u);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_SORT_KEYS));
        OpOraValue(cpu, 0xff00u);
        OpIncA(cpu);
        if (!cpu->zero)
            OpLda(memory, cpu, OpLongX(cpu, ENEMY_POINTER_TABLE));
        OpSta(memory, cpu, OpAbsX(cpu, ENEMY_POINTERS));
        OpDex(cpu);
        OpDex(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, 0x472u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, ENEMY_BATTLERS));
        OpSta(memory, cpu, OpAbsX(cpu, ENEMY_SORT_BUFFER));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, ENEMY_BATTLER_BYTES);
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT), cpu->y);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_SORT_ORDER));
        OpSta(memory, cpu, SNES_WRMPYB);
        PushY(memory, cpu);
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT)));
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, SNES_RDMPYL);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, ENEMY_BATTLER_BYTES);
        OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_RECORD_BYTES));
        do {
            OpLda(memory, cpu, OpAbsX(cpu, ENEMY_SORT_BUFFER));
            OpSta(memory, cpu, OpAbsY(cpu, ENEMY_BATTLERS));
            OpInx(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_ENEMY_RECORD_BYTES), -1);
        } while (!cpu->zero);
        OpWriteX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT), cpu->y);
        OpPullY(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
}

static void ReorderEnemyActions(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 70u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, ENEMY_ACTIONS));
        OpSta(memory, cpu, OpAbsX(cpu, ENEMY_SORT_BUFFER));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, ENEMY_ACTION_BYTES);
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT), cpu->y);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, ENEMY_SORT_ORDER));
        OpSta(memory, cpu, SNES_WRMPYB);
        PushY(memory, cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT)));
        OpRepWidths(cpu, 0x20u);
        OpLda(memory, cpu, SNES_RDMPYL);
        OpTay(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, ENEMY_ACTION_BYTES);
        OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_RECORD_BYTES));
        do {
            OpLda(memory, cpu, OpAbsY(cpu, ENEMY_SORT_BUFFER));
            OpSta(memory, cpu, OpLongX(cpu, ENEMY_ACTIONS));
            OpInx(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, DP_ENEMY_RECORD_BYTES), -1);
        } while (!cpu->zero);
        OpWriteX(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT), cpu->x);
        OpPullY(memory, cpu);
        OpIny(cpu);
        OpCpy(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
}

static void RemapEnemyTarget(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero) {
        OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT));
        OpLoadA(cpu, 0x80u);
        OpLdy(cpu, 0u);
        do {
            OpLsrMem(memory, cpu, OpDp(cpu, DP_ENEMY_ORDER_LIMIT));
            if (cpu->carry)
                OpOra(memory, cpu, OpAbsY(cpu, ENEMY_TARGET_BITS));
            OpIny(cpu);
            OpCpy(cpu, ENEMY_COUNT);
        } while (!cpu->zero);
    }
    SimulateRtsFrame(memory, cpu);
}

static void RemapEnemyActions(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const struct {
        uint32_t address;
        uint16_t site;
    } targets[] = {{ENEMY_ACTIONS, 0xe31fu},       {ENEMY_ACTIONS + 12u, 0xe32au},
                   {ENEMY_ACTIONS + 24u, 0xe335u}, {ENEMY_ACTIONS + 36u, 0xe340u},
                   {ENEMY_ACTIONS + 48u, 0xe34bu}, {ENEMY_ACTIONS + 60u, 0xe356u},
                   {0x7ff560u, 0xe361u},           {0x7ff56cu, 0xe36cu},
                   {0x7ff578u, 0xe377u},           {0x7ff584u, 0xe382u},
                   {0x7ff590u, 0xe38du},           {ENEMY_ACTIONS + 2u, 0xe398u},
                   {ENEMY_ACTIONS + 14u, 0xe3a3u}, {ENEMY_ACTIONS + 26u, 0xe3aeu},
                   {ENEMY_ACTIONS + 38u, 0xe3b9u}, {ENEMY_ACTIONS + 50u, 0xe3c4u},
                   {ENEMY_ACTIONS + 62u, 0xe3cfu}, {WRAM_BATTLE_ACTION_WORK, 0xe3dau}};
    for (unsigned target = 0u; target < sizeof targets / sizeof targets[0]; ++target) {
        OpLda(memory, cpu, targets[target].address);
        RemapEnemyTarget(memory, cpu, targets[target].site);
        OpSta(memory, cpu, targets[target].address);
    }
    OpLdx(cpu, 0xfffdu);
    for (;;) {
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_TURN_QUEUE));
        if (cpu->zero)
            return;
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            RemapEnemyTarget(memory, cpu, 0xe3f0u);
            OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_TURN_QUEUE));
        }
    }
}

Lufia2ExecutionResult Lufia2BattleReorderEnemies(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    if (!BattleFollowupContext(cpu, 0u))
        return ExecutionHandoff(cpu, 0x85e174u);
    OpSepWidths(cpu, 0x10u);
    OpStz(memory, cpu, OpDp(cpu, REQUESTED_ENEMY));
    OpLdx(cpu, 0u);
    bool ordered = true;
    do {
        OpLda(memory, cpu, OpAbsX(cpu, ENEMY_DISPLAY_FIELDS));
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, ENEMY_COLUMN_ORIGINS));
            OpCmp(memory, cpu, OpDp(cpu, REQUESTED_ENEMY));
            if (!cpu->carry) {
                ordered = false;
                break;
            }
            OpSta(memory, cpu, OpDp(cpu, REQUESTED_ENEMY));
        }
        OpInx(cpu);
        OpCpx(cpu, ENEMY_COUNT);
    } while (!cpu->zero);
    if (ordered) {
        OpRepWidths(cpu, 0x10u);
        return ExecutionReturned(0x85e191u);
    }
    SortEnemyColumns(memory, cpu);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    ReorderEnemyDisplayFields(memory, cpu);
    ReorderEnemyAuxiliaryRecords(memory, cpu);
    ReorderEnemyBattlers(memory, cpu);
    ReorderEnemyActions(memory, cpu);
    PullDataBank(memory, cpu);
    RemapEnemyActions(memory, cpu);
    return ExecutionReturned(0x85e3f8u);
}

typedef struct EnemyInsertion {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
} EnemyInsertion;

static bool EnemyInsertionCall(EnemyInsertion *insertion, uint32_t site,
                               uint32_t target, bool byte_accumulator) {
    Lufia2CpuState *cpu = insertion->cpu;
    if (!BattleFollowupCall(insertion->memory, cpu, insertion->child,
                            insertion->context, site, target, 3u)) {
        insertion->result = FollowupUnwound(site);
        return false;
    }
    if (cpu->program_bank != 0x85u || cpu->direct_page || cpu->decimal ||
        cpu->index_is_8_bit || cpu->accumulator_is_8_bit != byte_accumulator) {
        insertion->result = ExecutionHandoff(cpu, site + 4u);
        return false;
    }
    return true;
}

static bool EnemyInsertionEntry(const Lufia2CpuState *cpu, bool any_index_width) {
    return BattleFollowupContext(cpu, 1u) && (any_index_width || !cpu->index_is_8_bit);
}

static void AllocateInsertedEnemy(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_COLUMN));
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_PIXEL_COLUMN));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpTxy(cpu);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, ENEMY_POINTERS));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
    }
    OpLda(memory, cpu, OpLongX(cpu, ENEMY_POINTER_TABLE));
    OpSta(memory, cpu, OpAbsX(cpu, ENEMY_POINTERS));
    OpTax(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, DP_INSERTED_ENEMY_POINTER), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, DP_ENEMY_RECORD_POINTER), cpu->x);
    OpSepWidths(cpu, 0x20u);
    OpTya(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_DISPLAY_FIELDS));
    OpWriteX(memory, cpu, OpDp(cpu, DP_INSERTED_ENEMY_SLOT), cpu->y);
    OpLda(memory, cpu, OpDp(cpu, DP_INSERTED_ENEMY));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_MESSAGE_WINDOW_COLUMNS));
}

Lufia2ExecutionResult Lufia2BattleInsertEnemy(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              void *context) {
    if (!child || !EnemyInsertionEntry(cpu, true))
        return ExecutionHandoff(cpu, 0x85e412u);
    EnemyInsertion insertion = {memory, cpu, child, context, {0}};
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    PushAccumulator8(memory, cpu);
    if (!EnemyInsertionCall(&insertion, 0x85e41bu, 0x85e4e5u, true))
        return insertion.result;
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_INSERTED_ENEMY));
    if (!EnemyInsertionCall(&insertion, 0x85e422u, 0x85e02du, true))
        return insertion.result;
    AllocateInsertedEnemy(memory, cpu);
    if (!EnemyInsertionCall(&insertion, 0x85e456u, 0x81fc0bu, true))
        return insertion.result;
    OpLda(memory, cpu, OpDp(cpu, DP_INSERTED_ENEMY));
    if (!EnemyInsertionCall(&insertion, 0x85e45cu, 0x81fb79u, true))
        return insertion.result;
    OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_VARIANT_CHANCE));
    OpLoadA(cpu, 100u);
    if (!EnemyInsertionCall(&insertion, 0x85e464u, 0x808299u, true))
        return insertion.result;
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, DP_ENEMY_VARIANT_CHANCE)));
    TransferDirectToA(cpu);
    RolA8(cpu);
    LoadA8(cpu, A8(cpu) ^ 1u);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_INSERTED_ENEMY_SLOT)));
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_DISPLAY_FLAGS));
    OpLda(memory, cpu, OpDp(cpu, DP_ENEMY_PIXEL_COLUMN));
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_COLUMN_ORIGINS));
    OpLda(memory, cpu, OpDp(cpu, DP_INSERTED_ENEMY));
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_DISPLAY_IDS));
    if (!EnemyInsertionCall(&insertion, 0x85e47eu, 0x81fbc6u, true) ||
        !EnemyInsertionCall(&insertion, 0x85e482u, 0x81fb8eu, true))
        return insertion.result;
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_COLUMN_WIDTHS));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_DISPLAY_HEIGHTS));
    OpLoadA(cpu, 12u);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpAbsY(cpu, ENEMY_DISPLAY_HEIGHTS)));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsY(cpu, ENEMY_DISPLAY_FLAGS));
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, ENEMY_DISPLAY_ROWS));
    static const struct {
        uint32_t site, target;
        bool byte_accumulator;
    } rebuild[] = {{0x85e49du, 0x85e174u, true},  {0x85e4a1u, 0x85df44u, true},
                   {0x85e4a5u, 0x85eb91u, true},  {0x85e4a9u, 0x85de9du, true},
                   {0x85e4afu, 0x859d33u, false}, {0x85e4b5u, 0x85ec81u, true},
                   {0x85e4bbu, 0x859d4au, false}, {0x85e4c1u, 0x85ec81u, true},
                   {0x85e4c5u, 0x858af4u, true},  {0x85e4c9u, 0x81b9afu, true}};
    for (unsigned step = 0u; step < sizeof rebuild / sizeof rebuild[0]; ++step) {
        if (!rebuild[step].byte_accumulator)
            OpRepWidths(cpu, 0x20u);
        if (!EnemyInsertionCall(&insertion, rebuild[step].site, rebuild[step].target,
                                rebuild[step].byte_accumulator))
            return insertion.result;
        if (!rebuild[step].byte_accumulator)
            OpSepWidths(cpu, 0x20u);
    }
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SPRITE_BUILD_MODE));
    if (!EnemyInsertionCall(&insertion, 0x85e4d2u, 0x858a2fu, true))
        return insertion.result;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    RestoreEnemyCapacityRegisters(memory, cpu);
    return ExecutionReturned(0x85e4e4u);
}

static void PrepareExistingEnemyTiles(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_SPRITE_FLAGS + 12u));
    OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_COLUMN));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_SPRITE_FLAGS + 6u));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_SPRITE_FLAGS + 4u));
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0100u);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpAndValue(cpu, 0x1f1fu);
    OpSepWidths(cpu, 0x20u);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLoadA(cpu, 64u);
    OpSta(memory, cpu, SNES_WRMPYB);
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpRepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    OpAdcValue(cpu, OpReadM(memory, cpu, SNES_RDMPYL));
    OpAdcValue(cpu, 0x2800u);
    OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_TILEMAP_ADDRESS));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_SPRITE_FLAGS + 2u));
    cpu->carry = 0u;
    OpAdcValue(cpu, 5u);
    OpAndValue(cpu, 7u);
    OpAslA(cpu);
    OpAslA(cpu);
    OpOraValue(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_INSERTED_ENEMY_SLOT));
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_SPRITE_FLAGS + 3u));
    OpSta(memory, cpu, OpDp(cpu, DP_INSERTED_ENEMY));
}

Lufia2ExecutionResult Lufia2BattlePrepareEnemyInsertion(const Lufia2Memory *memory,
                                                        Lufia2CpuState *cpu,
                                                        Lufia2PushedChildCall child,
                                                        void *context) {
    if (!child || !EnemyInsertionEntry(cpu, false))
        return ExecutionHandoff(cpu, 0x85e4e5u);
    EnemyInsertion insertion = {memory, cpu, child, context, {0}};
    static const uint32_t queue_sites[] = {0x85e4e7u, 0x85e4f3u, 0x85e4ffu};
    static const uint32_t frame_sites[] = {0x85e4edu, 0x85e4f9u, 0x85e505u};
    static const uint32_t queues[] = {0x859d8fu, 0x859da6u, 0x859dbdu};
    for (unsigned queue = 0u; queue < 3u; ++queue) {
        OpRepWidths(cpu, 0x20u);
        if (!EnemyInsertionCall(&insertion, queue_sites[queue], queues[queue], false))
            return insertion.result;
        OpSepWidths(cpu, 0x20u);
        if (!EnemyInsertionCall(&insertion, frame_sites[queue], 0x85ec81u, true))
            return insertion.result;
    }
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0x2800u);
    OpLdy(cpu, 0x0200u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLoadA(cpu, 1u);
    do {
        OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpDey(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ENEMY_INITIAL_COUNT));
    if (!cpu->zero) {
        OpLoadA(cpu, ENEMY_COUNT);
        OpSta(memory, cpu, OpDp(cpu, DP_ENEMY_SPRITES_LEFT));
        OpLoadA(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        OpLdy(cpu, 0u);
        do {
            OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_SPRITE_FLAGS));
            if (cpu->negative) {
                PushY(memory, cpu);
                PrepareExistingEnemyTiles(memory, cpu);
                if (!EnemyInsertionCall(&insertion, 0x85e587u, 0x81be54u, true))
                    return insertion.result;
                OpPullY(memory, cpu);
            }
            OpRepWidths(cpu, 0x20u);
            OpTya(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, ENEMY_SPRITE_BYTES);
            OpTay(cpu);
            OpSepWidths(cpu, 0x20u);
            OpStepMem(memory, cpu, OpDp(cpu, DP_ENEMY_SPRITES_LEFT), -1);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);
    if (!EnemyInsertionCall(&insertion, 0x85e59bu, 0x858b22u, true) ||
        !EnemyInsertionCall(&insertion, 0x85e59fu, 0x81b9afu, true))
        return insertion.result;
    OpRepWidths(cpu, 0x20u);
    if (!EnemyInsertionCall(&insertion, 0x85e5a5u, 0x859bf1u, false))
        return insertion.result;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SPRITE_BUILD_MODE));
    if (!EnemyInsertionCall(&insertion, 0x85e5b0u, 0x858a2fu, true))
        return insertion.result;
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_SPRITE_REBUILD_STATE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!EnemyInsertionCall(&insertion, 0x85e5bdu, 0x85ec81u, true))
        return insertion.result;
    return ExecutionReturned(0x85e5c1u);
}
