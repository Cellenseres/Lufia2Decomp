#include "battle/battle_internal.h"
#include "core/wram_view.h"
#include "lufia2/system.h"

enum {
    TURN_DP_SLOT_BIT = 0x00u,
    TURN_DP_ACTOR_OR_SPREAD = 0x54u,
    TURN_SPREAD_FACTOR = 0x0du,
    TURN_DP_PRIORITY = 0x56u,
    TURN_DP_PARTY_RECORD = 0xd5u,
};

static void BattleInsertTurnQueueEntry(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t return_bank, uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    (void)BattleInsertTurnBody(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

static uint8_t RandomizeTurnChild(
    void *context, Lufia2CpuState *cpu, uint32_t target,
    uint32_t site, uint8_t frame_size) {
    const Lufia2Memory *memory = context;
    (void)site;
    (void)frame_size;
    if (target == 0x80834cu)
        (void)Lufia2Multiply16By8(memory, cpu);
    else
        (void)Lufia2BattleRandomFraction(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return 1u;
}

static void BattleRandomizePriority(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address) {
    const uint8_t caller_bank = cpu->program_bank;
    SimulateJslFrame(memory, cpu, 0x81u, return_address);
    cpu->program_bank = 0x85u;
    (void)Lufia2BattleRandomizeTurnPriority(
        memory, cpu, RandomizeTurnChild, (void *)memory);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = caller_bank;
}

static void BattleStageRandomizedTurnPriority(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_BASE_PRIORITY));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_PRIORITY_BONUS));
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_PRIORITY));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, TURN_SPREAD_FACTOR);
    OpSta(memory, cpu, OpDp(cpu, TURN_DP_ACTOR_OR_SPREAD));
}

static bool BattlerCanTakeTurn(Lufia2Wram wram, Lufia2CpuState *cpu) {
    OpLda(wram.memory, cpu, OpAbsY(cpu, BATTLE_BATTLER_STATUS));
    OpBitValue(cpu, BATTLE_STATUS_NO_TURN_MASK);
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

static uint8_t PublishPartyActionChild(
    void *context, Lufia2CpuState *cpu, uint32_t target,
    uint32_t site, uint8_t frame_size) {
    const Lufia2Memory *memory = context;
    (void)site;
    if (target == 0x85cdfau)
        (void)Lufia2BattleActionRecordPointer(memory, cpu);
    else
        (void)BattleInsertTurnBody(memory, cpu);
    if (frame_size == 2u)
        SimulateRtsFrame(memory, cpu);
    else
        SimulateRtlFrame(memory, cpu);
    return 1u;
}

static void BattlePublishPartyTurn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x85u, 0x92bcu);
    (void)Lufia2BattlePublishPartyAction(
        memory, cpu, PublishPartyActionChild, (void *)memory);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x92c0u);
    (void)Lufia2BattleQueueStagedPartyAction(
        memory, cpu, PublishPartyActionChild, (void *)memory);
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
