#include "battle/battle_internal.h"
#include "core/wram_view.h"
#include "lufia2/system.h"

enum {
    TURN_DP_SLOT_BIT = 0x00u,
    TURN_DP_ACTOR_OR_SPREAD = 0x54u,
    TURN_SPREAD_FACTOR = 0x0du,
    TURN_DP_PRIORITY = 0x56u,
    TURN_DP_PRODUCT_SOURCE = 0x4eu,
    TURN_DP_PRODUCT_FACTOR = 0x50u,
    TURN_DP_PRODUCT_HIGH = 0x52u,
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

/* $85:9337: insert the staged entry by descending priority. */
static void BattleInsertTurnQueueEntry(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                       uint8_t return_bank, uint16_t return_address) {
    static const uint8_t staged_bytes[BATTLE_TURN_ENTRY_SIZE] = {
        TURN_DP_ACTOR_OR_SPREAD, TURN_DP_PRIORITY, TURN_DP_PRIORITY + 1u};
    const Lufia2Wram dp = WramViewOfCaller(memory, cpu);
    Lufia2Wram queue;
    uint16_t priority, insert_at, from;
    unsigned i;

    SimulateJslFrame(memory, cpu, return_bank, return_address);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    queue = WramViewInBank(memory, cpu, 0x85u);

    priority = WramRead16(dp, TURN_DP_PRIORITY);
    insert_at = 0;
    while (WramReadAt(queue, WRAM_BATTLE_TURN_QUEUE, insert_at) != 0 &&
           WramRead16At(queue, WRAM_BATTLE_TURN_QUEUE + 1u, insert_at) >= priority)
        insert_at = (uint16_t)(insert_at + BATTLE_TURN_ENTRY_SIZE);
    WramWrite16(dp, TURN_DP_INSERT_OFFSET, insert_at);

    /* Shift later entries up by one. */
    from = (WRAM_BATTLE_TURN_QUEUE_COUNT - 1u) * BATTLE_TURN_ENTRY_SIZE;
    do {
        from = (uint16_t)(from - BATTLE_TURN_ENTRY_SIZE);
        for (i = 0; i < BATTLE_TURN_ENTRY_SIZE; ++i)
            WramWriteAt(queue, WRAM_BATTLE_TURN_QUEUE + BATTLE_TURN_ENTRY_SIZE + i,
                        from, WramReadAt(queue, WRAM_BATTLE_TURN_QUEUE + i, from));
    } while (from != WramRead16(dp, TURN_DP_INSERT_OFFSET));
    for (i = 0; i < BATTLE_TURN_ENTRY_SIZE; ++i)
        WramWriteAt(queue, WRAM_BATTLE_TURN_QUEUE + i, insert_at,
                    WramRead(dp, staged_bytes[i]));

    /* Exit: last staged byte in A, indexes at entry. */
    cpu->x = insert_at;
    cpu->y = from;
    cpu->carry = 1;
    LoadA8(cpu, WramRead(dp, staged_bytes[BATTLE_TURN_ENTRY_SIZE - 1u]));
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $80:834C: 16x8 product into DP $51-$53. */
static void BattlePriorityProduct(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x85u, 0xdd27u);
    (void)Lufia2Multiply16By8(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $85:DD19: randomize the staged priority within its spread. */
static void BattleRandomizePriority(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint16_t return_address) {
    const Lufia2Wram dp = WramViewOfCaller(memory, cpu);
    uint16_t priority;
    int below_zero;

    SimulateJslFrame(memory, cpu, 0x81u, return_address);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);

    WramWrite(dp, TURN_DP_PRODUCT_FACTOR, WramRead(dp, TURN_DP_ACTOR_OR_SPREAD));
    priority = WramRead16(dp, TURN_DP_PRIORITY);
    LoadX16(cpu, priority);
    WramWrite16(dp, TURN_DP_PRODUCT_SOURCE, priority);
    BattlePriorityProduct(memory, cpu);

    /* Range: product's upper word plus one, more from 2. */
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, (uint16_t)(WramRead16(dp, TURN_DP_PRODUCT_HIGH) + 1u));
    Compare16(cpu, cpu->accumulator, 2u);
    Add16Value(cpu, 0u);
    WramWrite16(dp, TURN_DP_RANDOM_RANGE, cpu->accumulator);

    BattleCallRandomFraction(memory, cpu, 0xdd38u);
    WramWrite16(dp, TURN_DP_RANDOM_SUM, cpu->accumulator);
    LoadA16(cpu, WramRead16(dp, TURN_DP_RANDOM_RANGE));
    BattleCallRandomFraction(memory, cpu, 0xdd40u);
    cpu->carry = 0;
    Add16Value(cpu, WramRead16(dp, TURN_DP_RANDOM_SUM));
    WramWrite16(dp, TURN_DP_RANDOM_SUM, cpu->accumulator);

    /* priority - range + random draws, clamped at both ends. */
    LoadA16(cpu, WramRead16(dp, TURN_DP_PRODUCT_SOURCE));
    Subtract16(cpu, WramRead16(dp, TURN_DP_RANDOM_RANGE));
    below_zero = !cpu->carry;
    cpu->carry = 0;
    Add16Value(cpu, WramRead16(dp, TURN_DP_RANDOM_SUM));
    if (below_zero && !cpu->carry)
        LoadA16(cpu, cpu->direct_page);
    else if (!below_zero && cpu->carry)
        LoadA16(cpu, 0xffffu);
    WramWrite16(dp, TURN_DP_PRIORITY, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* Stage battler Y: priority plus bonus, spread 13. */
static void BattleStageRandomizedTurnPriority(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16At(wram, BATTLE_BATTLER_BASE_PRIORITY, cpu->y));
    cpu->carry = 0;
    Add16Value(cpu, WramRead16At(wram, BATTLE_BATTLER_PRIORITY_BONUS, cpu->y));
    WramWrite16(wram, TURN_DP_PRIORITY, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, TURN_SPREAD_FACTOR);
    WramWrite(wram, TURN_DP_ACTOR_OR_SPREAD, TURN_SPREAD_FACTOR);
}

/* A battler takes a turn unless its status forbids. */
static bool BattlerCanTakeTurn(const Lufia2Wram wram, Lufia2CpuState *cpu) {
    LoadA8(cpu, WramReadAt(wram, BATTLE_BATTLER_STATUS, cpu->y));
    cpu->zero = (A8(cpu) & BATTLE_STATUS_NO_TURN_MASK) == 0;
    return cpu->zero;
}

/* Queue a turn for every live enemy. */
Lufia2ExecutionResult Lufia2BattleQueueEnemyTurns(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    LoadA8(cpu, 1u);
    WramWrite(wram, TURN_DP_SLOT_BIT, A8(cpu));
    LoadX16(cpu, 0u);
    do {
        LoadY16(cpu, WramRead16At(wram, WRAM_BATTLE_ENEMY_RECORDS, cpu->x));
        if (cpu->y != 0 && BattlerCanTakeTurn(wram, cpu)) {
            OpPushX(memory, cpu);
            PushY(memory, cpu);
            BattleStageRandomizedTurnPriority(memory, cpu);
            BattleRandomizePriority(memory, cpu, 0xc27du);
            LoadA8(cpu, BATTLE_ACTOR_ENEMY_SIDE | WramRead(wram, TURN_DP_SLOT_BIT));
            WramWrite(wram, TURN_DP_ACTOR_OR_SPREAD, A8(cpu));
            BattleInsertTurnQueueEntry(memory, cpu, 0x81u, 0xc287u);
            OpPullY(memory, cpu);
            OpPullX(memory, cpu);
        }
        cpu->carry = false;
        OpRolMem8(memory, cpu, OpDp(cpu, TURN_DP_SLOT_BIT));
        LoadX16(cpu, (uint16_t)(cpu->x + BATTLE_POINTER_SIZE));
        Compare16(cpu, cpu->x, BATTLE_ENEMY_COUNT * BATTLE_POINTER_SIZE);
    } while (!cpu->zero);
    return ExecutionReturned(0x81c293u);
}

/* Queue the capsule monster's turn when able. */
Lufia2ExecutionResult Lufia2BattleQueueCapsuleTurn(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    LoadY16(cpu, WramRead16(wram, WRAM_BATTLE_CAPSULE_RECORD));
    if (cpu->y != 0 && BattlerCanTakeTurn(wram, cpu)) {
        PushY(memory, cpu);
        BattleStageRandomizedTurnPriority(memory, cpu);
        BattleRandomizePriority(memory, cpu, 0xc2b5u);
        LoadA8(cpu, BATTLE_ACTOR_CAPSULE);
        WramWrite(wram, TURN_DP_ACTOR_OR_SPREAD, A8(cpu));
        BattleInsertTurnQueueEntry(memory, cpu, 0x81u, 0xc2bdu);
        OpPullY(memory, cpu);
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
