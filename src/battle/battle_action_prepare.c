#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"
#include "system/wram.h"

enum {
    ACTION_WORK_LOW = WRAM_BATTLE_ACTION_WORK & 0xffffu,
    ACTION_TARGET_MASK = WRAM_BATTLE_ACTION_WORK + 2u,
    ACTION_EFFECT_FLAGS = WRAM_BATTLE_ACTION_WORK + 12u,
    ACTION_ADJUSTMENT_FIRST = 12u,
    ACTION_ADJUSTMENT_END = 122u,
    ACTION_RESULT_STRIDE = 30u,
    ACTION_RESULT_COUNT = 11u,
    ACTION_RESULT_RECORD_BYTES = ACTION_RESULT_STRIDE * ACTION_RESULT_COUNT,
    ACTION_RESULT_RECORD_LOW = WRAM_BATTLE_ACTION_RESULT_RECORDS & 0xffffu,
    ACTION_RESULT_END = ACTION_RESULT_RECORD_LOW + ACTION_RESULT_RECORD_BYTES,
    ACTION_SOURCE_STATS = 0xb8u,
    ACTION_RESULT_VALUE = 0x0a62u,
    ACTION_TARGET_COUNT = 0x0a2du,
    ACTION_RESULT_BYTE_A = 0x0a35u,
    ACTION_RESULT_BYTE_B = 0x0a36u,
    ENEMY_STATS_POINTERS = 0x0a6eu,
    ENEMY_STATS_STATUS = 15u,
    ENEMY_INACTIVE = 4u,
    CAPSULE_STATS_POINTER = 0x0a88u,
    QUEUE_ACTOR_MASK = WRAM_PARTY_STAT_MEMBER_INDEX,
    QUEUE_RECORD_BYTES = 3u,
    QUEUE_LAST_OFFSET = (WRAM_BATTLE_TURN_QUEUE_COUNT - 1u) * QUEUE_RECORD_BYTES,
    QUEUE_END_OFFSET = WRAM_BATTLE_TURN_QUEUE_COUNT * QUEUE_RECORD_BYTES,
    QUEUE_SCAN_BUDGET = 4096u,
    ROM_ACTOR_INDEX = 0x97fe26u
};

static uint8_t ActionWorkContext(const Lufia2CpuState *cpu, uint8_t bank,
                                 unsigned mode) {
    return cpu->program_bank == bank && !cpu->direct_page && !cpu->decimal &&
           (mode == 2u || !cpu->index_is_8_bit) &&
           (mode == 1u || mode == 2u ||
            cpu->accumulator_is_8_bit == (mode == 3u ? 0u : 1u)) &&
           cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static uint8_t ActionWorkCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                              Lufia2PushedChildCall child, void *context, uint32_t site,
                              uint32_t target, uint8_t frame) {
    return CallChildWithFrame(memory, cpu, child, context, site, target, frame,
                              (uint8_t)(site >> 16));
}

static Lufia2ExecutionResult ActionWorkUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void SaveActionRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
}

static void RestoreActionRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

static void ActionDataBank(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleResetTargetMotion(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!child || !ActionWorkContext(cpu, 0x81u, 2u))
        return ExecutionHandoff(cpu, 0x81ebf4u);
    SaveActionRegisters(memory, cpu);
    if (!ActionWorkCall(memory, cpu, child, context, 0x81ebfdu, 0x81bae8u, 3u))
        return ActionWorkUnwound(0x81ebfdu);
    ActionDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    do {
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_EFFECT_TARGET_HORIZONTAL_MOTION));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_EFFECT_TARGET_VERTICAL_MOTION));
        for (unsigned word = 0u; word < 4u; ++word)
            OpIny(cpu);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 13u);
        OpTax(cpu);
        OpCpy(cpu, 20u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_BATTLE_EFFECT_FRAME_MARKER);
    OpRepWidths(cpu, 0x20u);
    if (!ActionWorkCall(memory, cpu, child, context, 0x81ec29u, 0x859bdau, 3u))
        return ActionWorkUnwound(0x81ec29u);
    RestoreActionRegisters(memory, cpu);
    return ExecutionReturned(0x81ec34u);
}

Lufia2ExecutionResult
Lufia2BattleRefreshInactiveEnemySprites(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                        Lufia2PushedChildCall child, void *context) {
    if (!child || !ActionWorkContext(cpu, 0x85u, 0u))
        return ExecutionHandoff(cpu, 0x85948cu);
    OpLdy(cpu, 0u);
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, ENEMY_STATS_POINTERS)));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, ENEMY_STATS_STATUS));
            OpAndValue(cpu, ENEMY_INACTIVE);
            if (!cpu->zero) {
                OpTya(cpu);
                OpLsrA(cpu);
                OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
                OpLoadA(cpu, 15u);
                OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
                OpRepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
                OpTax(cpu);
                OpSepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_SPRITE_FLAGS));
                OpAndValue(cpu, 0x7fu);
                OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_SPRITE_FLAGS));
            }
        }
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 12u);
    } while (!cpu->zero);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, CAPSULE_STATS_POINTER)));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, ENEMY_STATS_STATUS));
        OpAndValue(cpu, ENEMY_INACTIVE);
        if (!cpu->zero) {
            OpStz(memory, cpu, OpAbs(cpu, 0x154cu));
            OpLdx(cpu, 0xdf00u);
            OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
            OpStz(memory, cpu, OpAbs(cpu, SNES_WMADDH));
            OpLdx(cpu, 0x800u);
            do {
                OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
                OpDex(cpu);
            } while (!cpu->zero);
            OpRepWidths(cpu, 0x20u);
            if (!ActionWorkCall(memory, cpu, child, context, 0x8594e0u, 0x859d1cu, 3u))
                return ActionWorkUnwound(0x8594e0u);
            OpSepWidths(cpu, 0x20u);
        }
    }
    return ExecutionReturned(0x8594e6u);
}

Lufia2ExecutionResult Lufia2BattlePrepareAlternateAction(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    static const struct {
        uint32_t site, target;
    } followup[] = {{0x85ce56u, 0x85cf68u},
                    {0x85ce59u, 0x85d025u},
                    {0x85ce61u, 0x85d0cdu},
                    {0x85ce64u, 0x85d94fu}};
    if (!child || !ActionWorkContext(cpu, 0x85u, 2u))
        return ExecutionHandoff(cpu, 0x85ce21u);
    SaveActionRegisters(memory, cpu);
    ActionDataBank(memory, cpu);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_BATTLE_EFFECT_SCRIPT_ENABLED);
    OpLdx(cpu, ACTION_RESULT_RECORD_BYTES - 1u);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_BATTLE_ACTION_RESULT_RECORDS));
        OpDex(cpu);
    } while (!cpu->negative);
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_WORK_BYTE));
    if (!ActionWorkCall(memory, cpu, child, context, 0x85ce40u, 0x85ce6fu, 2u))
        return ActionWorkUnwound(0x85ce40u);
    OpLda(memory, cpu, WRAM_BATTLE_ACTION_RESULT_FLAGS);
    OpBitValue(cpu, 2u);
    if (!cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0xc3u);
        OpSta(memory, cpu, WRAM_BATTLE_ACTION_RESULT_FLAGS);
        OpSepWidths(cpu, 0x20u);
        for (unsigned call = 0u; call < 4u; ++call) {
            if (call == 2u) {
                OpLoadA(cpu, 4u);
                OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_WORK_BYTE));
            }
            if (!ActionWorkCall(memory, cpu, child, context, followup[call].site,
                                followup[call].target, 2u))
                return ActionWorkUnwound(followup[call].site);
        }
    }
    RestoreActionRegisters(memory, cpu);
    return ExecutionReturned(0x85ce6eu);
}

Lufia2ExecutionResult Lufia2BattleAccumulateActionAdjustment(const Lufia2Memory *memory,
                                                             Lufia2CpuState *cpu) {
    if (!ActionWorkContext(cpu, 0x85u, 3u))
        return ExecutionHandoff(cpu, 0x85cfabu);
    OpLda(memory, cpu, OpAbsX(cpu, 0u));
    OpAndValue(cpu, 0xffu);
    OpCmpValue(cpu, 0x80u);
    if (cpu->zero) {
        for (unsigned word = 0u; word < 11u; ++word) {
            const uint16_t destination = (uint16_t)(ACTION_WORK_LOW + 20u + word * 6u);
            OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(4u + word * 2u)));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpAbs(cpu, destination));
            OpSta(memory, cpu, OpAbs(cpu, destination));
        }
    }
    return ExecutionReturned(0x85d024u);
}

Lufia2ExecutionResult Lufia2BattleBuildActionAdjustments(const Lufia2Memory *memory,
                                                         Lufia2CpuState *cpu,
                                                         Lufia2PushedChildCall child,
                                                         void *context) {
    if (!child || !ActionWorkContext(cpu, 0x85u, 0u))
        return ExecutionHandoff(cpu, 0x85cf68u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_SAVED_TARGET_WORD & 0xffffu));
    OpSta(memory, cpu, OpAbs(cpu, ACTION_TARGET_MASK & 0xffffu));
    OpLdx(cpu, ACTION_ADJUSTMENT_END);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, ACTION_WORK_LOW));
        OpDex(cpu);
        OpDex(cpu);
        OpCpx(cpu, ACTION_ADJUSTMENT_FIRST - 2u);
    } while (!cpu->zero);
    OpLdx(cpu, ACTION_RESULT_RECORD_LOW);
    do {
        if (!ActionWorkCall(memory, cpu, child, context, 0x85cf85u, 0x85cfabu, 2u))
            return ActionWorkUnwound(0x85cf85u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, ACTION_RESULT_STRIDE);
        OpTax(cpu);
        OpCpx(cpu, ACTION_RESULT_END);
    } while (!cpu->zero);
    OpLdx(cpu, ACTION_ADJUSTMENT_END);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, ACTION_WORK_LOW));
        OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, ACTION_WORK_LOW));
        OpDex(cpu);
        OpDex(cpu);
        OpCpx(cpu, ACTION_ADJUSTMENT_FIRST - 2u);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x85cfaau);
}

Lufia2ExecutionResult Lufia2BattleReadActorBiasFlag(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    if (!ActionWorkContext(cpu, 0x85u, 0u))
        return ExecutionHandoff(cpu, 0x85d080u);
    OpBitValue(cpu, 0x80u);
    if (cpu->zero) {
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xffu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ROM_ACTOR_INDEX));
        OpAndValue(cpu, 0xffu);
        OpSepWidths(cpu, 0x20u);
        OpLsrA(cpu);
        OpAndValue(cpu, 1u);
        return ExecutionReturned(0x85d096u);
    }
    PushAccumulator8(memory, cpu);
    OpLdy(cpu, 10u);
    for (;;) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, ENEMY_STATS_POINTERS)));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, ENEMY_STATS_STATUS));
            OpBitValue(cpu, ENEMY_INACTIVE);
            if (cpu->zero) {
                OpTya(cpu);
                OpLsrA(cpu);
                OpTax(cpu);
                OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_GROUP_FLAGS));
                if (!cpu->zero)
                    break;
            }
        }
        OpDey(cpu);
        OpDey(cpu);
        if (cpu->negative) {
            LoadA8(cpu, Pull8(memory, cpu));
            TransferDirectToA(cpu);
            return ExecutionReturned(0x85d0b5u);
        }
    }
    LoadA8(cpu, Pull8(memory, cpu));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x7fu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_ACTOR_INDEX));
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_GROUP_FLAGS));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 1u));
    return ExecutionReturned(0x85d0ccu);
}

Lufia2ExecutionResult Lufia2BattleCalculateActionBias(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *context) {
    if (!child || !ActionWorkContext(cpu, 0x85u, 0u))
        return ExecutionHandoff(cpu, 0x85d025u);
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_BIAS));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SCRIPT_CONTEXT));
    if (!cpu->zero) {
        OpLda(memory, cpu, ACTION_EFFECT_FLAGS);
        OpBitValue(cpu, 0x40u);
        if (cpu->zero) {
            OpLda(memory, cpu, WRAM_BATTLE_ACTION_WORK);
            if (!ActionWorkCall(memory, cpu, child, context, 0x85d039u, 0x85d080u, 2u))
                return ActionWorkUnwound(0x85d039u);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_BIAS));
            OpLda(memory, cpu, ACTION_TARGET_MASK);
            if (!ActionWorkCall(memory, cpu, child, context, 0x85d043u, 0x85d080u, 2u))
                return ActionWorkUnwound(0x85d043u);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_BIAS));
            for (unsigned bit = 0u; bit < 6u; ++bit)
                OpAslA(cpu);
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_BIAS));
        }
    }
    OpLda(memory, cpu, ACTION_TARGET_MASK);
    if (!ActionWorkCall(memory, cpu, child, context, 0x85d057u, 0x85c168u, 2u))
        return ActionWorkUnwound(0x85d057u);
    OpLda(memory, cpu, OpAbsX(cpu, 16u));
    OpAndValue(cpu, 1u);
    uint8_t add_bias = !cpu->zero;
    if (add_bias) {
        OpLda(memory, cpu, ACTION_EFFECT_FLAGS);
        OpBitValue(cpu, 0x80u);
        add_bias = cpu->zero;
    }
    if (add_bias) {
        OpLoadA(cpu, 0x60u);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_BIAS));
    } else {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ACTION_BIAS));
    }
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_BATTLE_ACTION_WORK + 4u);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x85d07fu);
}

Lufia2ExecutionResult Lufia2BattleStoreActionResult(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *context) {
    if (!child || !ActionWorkContext(cpu, 0x85u, 0u))
        return ExecutionHandoff(cpu, 0x85d94fu);
    OpLda(memory, cpu, ACTION_TARGET_MASK);
    OpSta(memory, cpu, OpAbs(cpu, QUEUE_ACTOR_MASK));
    OpLoadA(cpu, 0u);
    OpLdx(cpu, 7u);
    do {
        OpLsrMem(memory, cpu, OpAbs(cpu, QUEUE_ACTOR_MASK));
        OpAdcValue(cpu, 0u);
        OpDex(cpu);
    } while (!cpu->zero);
    OpCmpValue(cpu, 1u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, ACTION_RESULT_VALUE));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_BATTLE_ACTION_RESULT_BUFFER + 2u);
    OpLda(memory, cpu, OpAbs(cpu, ACTION_TARGET_COUNT));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_BATTLE_ACTION_RESULT_BUFFER);
    OpLdy(cpu, 20u);
    OpLdx(cpu, 4u);
    do {
        OpLda(memory, cpu,
              LongIndexedAddress(DirectLongPointer(memory, cpu, ACTION_SOURCE_STATS),
                                 cpu->y));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_BATTLE_ACTION_RESULT_BUFFER));
        for (unsigned source_byte = 0u; source_byte < 6u; ++source_byte)
            OpIny(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpy(cpu, 0x5cu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, ACTION_RESULT_BYTE_A));
    OpSta(memory, cpu, WRAM_BATTLE_ACTION_RESULT_BUFFER + 28u);
    OpLda(memory, cpu, OpAbs(cpu, ACTION_RESULT_BYTE_B));
    OpSta(memory, cpu, WRAM_BATTLE_ACTION_RESULT_BUFFER + 26u);
    OpLda(memory, cpu, ACTION_TARGET_MASK);
    if (!cpu->zero) {
        if (!ActionWorkCall(memory, cpu, child, context, 0x85d9acu, 0x85d9c9u, 3u))
            return ActionWorkUnwound(0x85d9acu);
        OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
        OpLoadA(cpu, 0x7fu);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, OpLongX(cpu, WRAM_BATTLE_ACTION_RESULT_BUFFER));
            OpSta(memory, cpu, OpAbs(cpu, SNES_WMDATA));
            OpInx(cpu);
            OpCpx(cpu, ACTION_RESULT_STRIDE);
        } while (!cpu->zero);
    }
    return ExecutionReturned(0x85d9c8u);
}

Lufia2ExecutionResult Lufia2BattleInvalidateTurnQueueActor(const Lufia2Memory *memory,
                                                           Lufia2CpuState *cpu) {
    if (!ActionWorkContext(cpu, 0x85u, 0u))
        return ExecutionHandoff(cpu, 0x85d9f0u);
    PushAccumulator8(memory, cpu);
    PushY(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SCRIPT_CONTEXT));
    if (!cpu->zero) {
        OpLdy(cpu, 0xfffdu);
        for (unsigned record = 0u; record < QUEUE_SCAN_BUDGET; ++record) {
            OpIny(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_TURN_QUEUE));
            if (cpu->zero)
                break;
            OpCmp(memory, cpu, OpAbs(cpu, QUEUE_ACTOR_MASK));
            if (cpu->zero) {
                OpLoadA(cpu, 0xffu);
                OpSta(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_TURN_QUEUE));
            }
            if (record + 1u == QUEUE_SCAN_BUDGET)
                return ExecutionHandoff(cpu, 0x85d9fau);
        }
    }
    OpPullY(memory, cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x85da10u);
}

Lufia2ExecutionResult Lufia2BattleRemoveTurnQueueActor(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu) {
    if (!ActionWorkContext(cpu, 0x85u, 0u))
        return ExecutionHandoff(cpu, 0x85937du);
    PushDataBank(memory, cpu);
    ActionDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    for (;;) {
        OpCmp(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_TURN_QUEUE));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, QUEUE_END_OFFSET);
        if (cpu->zero) {
            PullDataBank(memory, cpu);
            return ExecutionReturned(0x8593b6u);
        }
    }
    OpCpx(cpu, QUEUE_LAST_OFFSET);
    while (!cpu->zero) {
        for (unsigned byte = 0u; byte < QUEUE_RECORD_BYTES; ++byte) {
            OpLda(memory, cpu,
                  OpAbsX(cpu, (uint16_t)(WRAM_BATTLE_TURN_QUEUE + 3u + byte)));
            OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(WRAM_BATTLE_TURN_QUEUE + byte)));
        }
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, QUEUE_LAST_OFFSET);
    }
    for (unsigned byte = 0u; byte < QUEUE_RECORD_BYTES; ++byte)
        OpStz(memory, cpu, OpAbsX(cpu, (uint16_t)(WRAM_BATTLE_TURN_QUEUE + byte)));
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8593b6u);
}
