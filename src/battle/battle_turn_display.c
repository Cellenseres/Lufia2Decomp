#include "battle/battle_internal.h"

/* $81:DEE9: refresh the panel and four party status rows. */
Lufia2ExecutionResult Lufia2BattleRefreshTurnDisplay(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, context, 0x81u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    if (!BattleCall(&battle, 0xdeecu, 0x81e593u, 2u) ||
        !BattleCall(&battle, 0xdeefu, 0x81e63au, 2u))
        return BattleChildUnwound(&battle);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81def3u);
}

/* $81:E63A: draw party slots in reverse order. */
Lufia2ExecutionResult Lufia2BattlePartyStatusRows(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, context, 0x81u);
    OpLdx(cpu, 6u);
    do {
        if (!BattleCall(&battle, 0xe63du, 0x81e645u, 2u))
            return BattleChildUnwound(&battle);
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x81e644u);
}

static bool StatusGauge(BattleContext *battle, uint16_t current, uint16_t maximum,
                        uint8_t tile, uint8_t offset, uint16_t site) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x40u);
    OpTay(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, current));
    OpSta(memory, cpu, OpDp(cpu, 0x11u));
    OpLda(memory, cpu, OpAbsX(cpu, maximum));
    OpSta(memory, cpu, OpDp(cpu, 0x13u));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, tile);
    OpSta(memory, cpu, OpDp(cpu, 0x16u));
    OpLoadA(cpu, offset);
    OpSta(memory, cpu, OpDp(cpu, 0x15u));
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    if (!BattleCall(battle, site, 0x81e2afu, 2u))
        return false;
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return true;
}

/* Body of $81:E645 between its register saves and the shared exit; false when
 * a child call unwinds. */
static bool PartyStatusRowCore(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x818819u));
    PushAccumulator16(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS)));
    OpTyx(cpu);
    OpPullY(memory, cpu);
    OpCpx(cpu, 0u);
    if (cpu->zero) {
        OpTya(cpu);
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x40u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, 6u);
        OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpAbs(cpu, 0x09f3u));
        OpLdy(cpu, 0x2150u);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x09f6u), cpu->y);
        if (!BattleCall(battle, 0xe674u, 0x81e7d2u, 2u))
            return false;
        return true;
    }
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, 0x12u));
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0u));
        if (!BattleCall(battle, 0xe687u, 0x81e835u, 2u))
            return false;
        OpBitValue(cpu, 0xffu);
        if (!cpu->zero) {
            cpu->carry = 1;
            OpSbcValue(cpu, 0x65u);
            OpSta(memory, cpu, OpAbsY(cpu, 0u));
        }
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x12u), -1);
    } while (!cpu->zero);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x40u);
    OpTay(cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    /* Saved slot lies below the temporary record and tile pointers. */
    OpLda(memory, cpu, OpStack(cpu, 5u));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x1be0u));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, 0x1bu));
    do {
        OpTxa(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0u));
        OpLoadA(cpu, 0x21u);
        OpSta(memory, cpu, OpAbsY(cpu, 1u));
        OpInx(cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x1bu), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 0x50u);
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpLoadA(cpu, 0x21u);
    OpSta(memory, cpu, OpAbsY(cpu, 1u));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (!StatusGauge(battle, 0x11u, 0x25u, 0x4au, 1u, 0xe6eeu) ||
        !StatusGauge(battle, 0x13u, 0x27u, 0x4cu, 0x16u, 0xe711u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x40u);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0xbcu));
    OpSta(memory, cpu, OpDp(cpu, 0x11u));
    OpStz(memory, cpu, OpDp(cpu, 0x12u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0x13u));
    OpStz(memory, cpu, OpDp(cpu, 0x14u));
    OpLoadA(cpu, 0x4eu);
    OpSta(memory, cpu, OpDp(cpu, 0x16u));
    OpLoadA(cpu, 0x2bu);
    OpSta(memory, cpu, OpDp(cpu, 0x15u));
    if (!BattleCall(battle, 0xe735u, 0x81e2afu, 2u))
        return false;
    return true;
}

/* $81:E645: name, symbols, HP, MP and IP for party slot X=0/2/4/6. */
Lufia2ExecutionResult Lufia2BattlePartyStatusRow(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, context, 0x81u);
    PushDataBank(memory, cpu);
    OpPushX(memory, cpu);
    if (!PartyStatusRowCore(&battle))
        return BattleChildUnwound(&battle);
    OpPullX(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81e73au);
}

/* $85:9DD4: queue the two turn-display regions. */
Lufia2ExecutionResult Lufia2BattleQueueTurnDisplay(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, context, 0x85u);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0x9dd6u, 0x85ecdbu, 3u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0x2c00u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a91u));
    OpLoadA(cpu, 0x5e00u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a93u));
    OpLoadA(cpu, 0x300u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a8fu));
    if (!BattleCall(&battle, 0x9decu, 0x85ecdbu, 3u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0x3400u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a91u));
    OpLoadA(cpu, 0xa00u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a93u));
    OpLoadA(cpu, 0x400u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a8fu));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x859e04u);
}
