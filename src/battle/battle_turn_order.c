#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum {
    TURN_DP_ACTOR_OR_SPREAD = 0x54u,
    TURN_DP_PRIORITY = 0x56u,
    TURN_DP_RANDOM_RANGE = 0x58u,
    TURN_DP_RANDOM_SUM = 0x5au,
    TURN_DP_INSERT_OFFSET = 0xcau,
    TURN_DP_PARTY_RECORD = 0xd5u,
    TURN_RECORD_INDEX_BASE = 0x09fau,
    TURN_MASK_REMAINDER = 0x09fbu,
};

/* $85:CDFA: decode the first target bit into a Battle action record. */
static void BattleFindActionRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                   uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    OpSta(memory, cpu, OpAbs(cpu, TURN_MASK_REMAINDER));
    OpAndValue(cpu, 0x80u);
    if (!cpu->zero)
        OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, TURN_RECORD_INDEX_BASE));
    OpLoadA(cpu, 0xffu);
    do {
        const uint32_t address = OpAbs(cpu, TURN_MASK_REMAINDER);
        const uint8_t target_mask = Read8(memory, address);
        OpIncA(cpu);
        cpu->carry = (target_mask & 1u) != 0u;
        Write8(memory, address, (uint8_t)(target_mask >> 1));
        SetNz8(cpu, (uint8_t)(target_mask >> 1));
    } while (!cpu->carry);
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbs(cpu, TURN_RECORD_INDEX_BASE));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x00ffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x859eecu));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);
}

/* $85:9337: insert a three-byte target/priority entry, descending. */
static void BattleInsertTurnQueueEntry(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       uint8_t return_bank, uint16_t return_address) {
    static const uint8_t staged_bytes[BATTLE_TURN_ENTRY_SIZE] = {
        TURN_DP_ACTOR_OR_SPREAD, TURN_DP_PRIORITY, TURN_DP_PRIORITY + 1u};
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_TURN_QUEUE));
        if (cpu->zero)
            break;
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_TURN_QUEUE + 1u)));
        OpCpy(cpu, OpReadX(memory, cpu, OpDp(cpu, TURN_DP_PRIORITY)));
        if (!cpu->carry)
            break;
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
    }
    OpWriteX(memory, cpu, OpDp(cpu, TURN_DP_INSERT_OFFSET), cpu->x);
    OpLdy(cpu, (WRAM_BATTLE_TURN_QUEUE_COUNT - 1u) * BATTLE_TURN_ENTRY_SIZE);
    do {
        OpDey(cpu);
        OpDey(cpu);
        OpDey(cpu);
        for (unsigned i = 0; i < BATTLE_TURN_ENTRY_SIZE; ++i) {
            OpLda(memory, cpu, OpAbsY(cpu, (uint16_t)(WRAM_BATTLE_TURN_QUEUE + i)));
            OpSta(memory, cpu,
                  OpAbsY(cpu, (uint16_t)(WRAM_BATTLE_TURN_QUEUE +
                                         BATTLE_TURN_ENTRY_SIZE + i)));
        }
        OpCpy(cpu, OpReadX(memory, cpu, OpDp(cpu, TURN_DP_INSERT_OFFSET)));
    } while (!cpu->zero);
    for (unsigned i = 0; i < BATTLE_TURN_ENTRY_SIZE; ++i) {
        OpLda(memory, cpu, OpDp(cpu, staged_bytes[i]));
        OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(WRAM_BATTLE_TURN_QUEUE + i)));
    }
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $80:834C: 16x8 product in DP $51-$53, preserving A/X/status. */
static void BattlePriorityProduct(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x85u, 0xdd27u);
    PushAccumulator8(memory, cpu);
    OpPushX(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, 0x53u));
    OpLda(memory, cpu, OpDp(cpu, 0x50u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLda(memory, cpu, OpDp(cpu, 0x4eu));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLda(memory, cpu, OpDp(cpu, 0x4fu));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x50u));
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpWriteX(memory, cpu, OpDp(cpu, 0x51u), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, 0x52u));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, OpDp(cpu, 0x52u));
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    OpLoadA(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $85:DD19: randomize DP $56 around its staged priority. */
static void BattleRandomizePriority(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x81u, return_address);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, TURN_DP_ACTOR_OR_SPREAD));
    OpSta(memory, cpu, OpDp(cpu, 0x50u));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, TURN_DP_PRIORITY)));
    OpWriteX(memory, cpu, OpDp(cpu, 0x4eu), cpu->x);
    BattlePriorityProduct(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x52u));
    OpIncA(cpu);
    OpCmpValue(cpu, 2u);
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_RANDOM_RANGE));
    BattleCallRandomFraction(memory, cpu, 0xdd38u);
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_RANDOM_SUM));
    OpLda(memory, cpu, OpDp(cpu, TURN_DP_RANDOM_RANGE));
    BattleCallRandomFraction(memory, cpu, 0xdd40u);
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, TURN_DP_RANDOM_SUM));
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_RANDOM_SUM));
    OpLda(memory, cpu, OpDp(cpu, 0x4eu));
    cpu->carry = true;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, TURN_DP_RANDOM_RANGE)));
    if (cpu->carry) {
        cpu->carry = false;
        OpAdc(memory, cpu, OpDp(cpu, TURN_DP_RANDOM_SUM));
        if (cpu->carry)
            OpLoadA(cpu, 0xffffu);
    } else {
        cpu->carry = false;
        OpAdc(memory, cpu, OpDp(cpu, TURN_DP_RANDOM_SUM));
        if (!cpu->carry)
            TransferDirectToA(cpu);
    }
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_PRIORITY));
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

static void BattleStageRandomizedTurnPriority(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_BASE_PRIORITY));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_PRIORITY_BONUS));
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_PRIORITY));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0du);
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_ACTOR_OR_SPREAD));
}

Lufia2ExecutionResult Lufia2BattleQueueEnemyTurns(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0u);
    do {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_RECORDS)));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_STATUS));
            OpBitValue(cpu, BATTLE_STATUS_NO_TURN);
            if (cpu->zero) {
                OpPushX(memory, cpu);
                PushY(memory, cpu);
                BattleStageRandomizedTurnPriority(memory, cpu);
                BattleRandomizePriority(memory, cpu, 0xc27du);
                OpLoadA(cpu, BATTLE_TARGET_ENEMY_SIDE);
                OpOra(memory, cpu, OpDp(cpu, 0u));
                OpSta(memory, cpu, OpDp(cpu, TURN_DP_ACTOR_OR_SPREAD));
                BattleInsertTurnQueueEntry(memory, cpu, 0x81u, 0xc287u);
                OpPullY(memory, cpu);
                OpPullX(memory, cpu);
            }
        }
        cpu->carry = false;
        OpRolMem8(memory, cpu, OpDp(cpu, 0u));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, BATTLE_ENEMY_COUNT * BATTLE_POINTER_SIZE);
    } while (!cpu->zero);
    return ExecutionReturned(0x81c293u);
}

Lufia2ExecutionResult Lufia2BattleQueueCapsuleTurn(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CAPSULE_RECORD)));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_STATUS));
        OpBitValue(cpu, BATTLE_STATUS_NO_TURN);
        if (cpu->zero) {
            PushY(memory, cpu);
            BattleStageRandomizedTurnPriority(memory, cpu);
            BattleRandomizePriority(memory, cpu, 0xc2b5u);
            OpLoadA(cpu, BATTLE_TARGET_CAPSULE);
            OpSta(memory, cpu, OpDp(cpu, TURN_DP_ACTOR_OR_SPREAD));
            BattleInsertTurnQueueEntry(memory, cpu, 0x81u, 0xc2bdu);
            OpPullY(memory, cpu);
        }
    }
    return ExecutionReturned(0x81c2bfu);
}

/* $85:92CE/$85:92FF: publish the descriptor, then its queued priority. */
static void BattlePublishPartyTurn(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t descriptor_offsets[4] = {0u, 2u, 6u, 8u};
    SimulateJslFrame(memory, cpu, 0x85u, 0x92bcu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_STAGED_ACTION));
    BattleFindActionRecord(memory, cpu, 0x92dau);
    OpRepWidths(cpu, 0x20u);
    for (unsigned i = 0; i < 4u; ++i) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(WRAM_BATTLE_STAGED_ACTION + 2u * i)));
        OpSta(memory, cpu, OpLongX(cpu, 0x7f0000u + descriptor_offsets[i]));
    }
    OpRepWidths(cpu, 0x30u);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);

    SimulateJslFrame(memory, cpu, 0x85u, 0x92c0u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_STAGED_ACTION));
    BattleFindActionRecord(memory, cpu, 0x930cu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_STAGED_ACTION));
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_ACTOR_OR_SPREAD));
    OpLda(memory, cpu, OpAbs(cpu, BATTLE_ACTION_PRIORITY));
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_PRIORITY));
    OpSepWidths(cpu, 0x20u);
    BattleInsertTurnQueueEntry(memory, cpu, 0x85u, 0x931eu);
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

static bool BattleLoadSelectedPartyTurn(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    OpSta(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
    OpAslA(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_RECORDS));
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_PARTY_RECORD));
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_STATUS));
    OpSepWidths(cpu, 0x20u);
    OpBitValue(cpu, BATTLE_STATUS_TURN_SELECTED);
    return !cpu->zero;
}

static void BattleQueueSelectedPartyTurn(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TARGET_MASK));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x96ffecu));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_STAGED_ACTION));
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_ACTOR_OR_SPREAD));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, TURN_DP_PARTY_RECORD)));
    OpLoadA(cpu, BATTLE_ACTION_SELECTED_TURN);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_TYPE));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_BASE_PRIORITY));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_PRIORITY_BONUS));
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ACTION_PRIORITY));
    BattlePublishPartyTurn(memory, cpu);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2BattleQueuePartyTurns(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    do {
        OpPushX(memory, cpu);
        if (BattleLoadSelectedPartyTurn(memory, cpu)) {
            BattleQueueSelectedPartyTurn(memory, cpu);
        }
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpTxa(cpu);
        OpCmp(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_COUNT));
    } while (!cpu->zero);
    return ExecutionReturned(0x8592cdu);
}
