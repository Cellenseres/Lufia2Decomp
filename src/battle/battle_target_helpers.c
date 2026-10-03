#include "battle/battle_internal.h"

static Lufia2ExecutionResult TargetCursor(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu, uint16_t base,
                                          bool marked) {
    OpLda(memory, cpu, OpAbsY(cpu, base));
    if (marked) {
        OpIncA(cpu);
        OpIncA(cpu);
    }
    OpSta(memory, cpu, OpLongX(cpu, 0x7e4abeu));
    OpLda(memory, cpu, OpAbsY(cpu, (uint16_t)(base + 1u)));
    if (marked) {
        OpDecA(cpu);
        OpDecA(cpu);
    }
    OpSta(memory, cpu, OpLongX(cpu, 0x7e4abfu));
    OpLoadA(cpu, 0x4eu);
    OpSta(memory, cpu, OpLongX(cpu, 0x7e4ac0u));
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, OpLongX(cpu, 0x7e4ac1u));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpLongX(cpu, 0x7e4ac2u));
    OpInx(cpu);
    OpInx(cpu);
    OpInx(cpu);
    OpInx(cpu);
    OpInx(cpu);
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_COUNT), 1);
    return ExecutionReturned(0x81d974u);
}
Lufia2ExecutionResult Lufia2BattleEnemyCursor(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    return TargetCursor(memory, cpu, WRAM_BATTLE_ENEMY_TARGETS, false);
}
Lufia2ExecutionResult Lufia2BattlePartyCursor(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    return TargetCursor(memory, cpu, WRAM_BATTLE_PARTY_TARGETS, false);
}
Lufia2ExecutionResult Lufia2BattleEnemyMarkedCursor(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    return TargetCursor(memory, cpu, WRAM_BATTLE_ENEMY_TARGETS, true);
}
Lufia2ExecutionResult Lufia2BattlePartyMarkedCursor(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    return TargetCursor(memory, cpu, WRAM_BATTLE_PARTY_TARGETS, true);
}

Lufia2ExecutionResult Lufia2BattleTargetCoordinates(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero) {
        OpAndValue(cpu, 0x7fu);
        OpCmpValue(cpu, 4u);
        if (cpu->zero) {
            OpLoadA(cpu, 16u);
            cpu->carry = false;
            OpAdc(memory, cpu, OpLongX(cpu, 5u));
            OpSta(memory, cpu, OpDp(cpu, 0x22u));
            OpLoadA(cpu, 16u);
            cpu->carry = false;
            OpAdc(memory, cpu, OpLongX(cpu, 7u));
            OpSta(memory, cpu, OpDp(cpu, 0x23u));
            return ExecutionReturned(0x81b8fbu);
        }
        OpSta(memory, cpu, 0x004202u);
        OpLoadA(cpu, 13u);
        OpSta(memory, cpu, 0x004203u);
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 0x1399u);
        cpu->carry = false;
        OpAdc(memory, cpu, 0x004216u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        PushDataBank(memory, cpu);
        OpSetDataBank(memory, cpu, 0u);
        OpLoadA(cpu, 16u);
        cpu->carry = false;
        OpAdc(memory, cpu, OpAbsX(cpu, 5u));
        OpSta(memory, cpu, OpDp(cpu, 0x22u));
        OpLoadA(cpu, 16u);
        cpu->carry = false;
        OpAdc(memory, cpu, OpAbsX(cpu, 7u));
        OpSta(memory, cpu, OpDp(cpu, 0x23u));
        PullDataBank(memory, cpu);
        return ExecutionReturned(0x81b8e8u);
    }
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0x11deu));
    if (!cpu->zero) {
        OpTxa(cpu);
        if (cpu->zero) {
            OpLdx(cpu, 0x605au);
            OpWriteX(memory, cpu, OpDp(cpu, 0x22u), cpu->x);
            OpLoadA(cpu, 0xffu);
            return ExecutionReturned(0x81b913u);
        }
        OpLdx(cpu, 0x605au);
        OpWriteX(memory, cpu, OpDp(cpu, 0x22u), cpu->x);
        TransferDirectToA(cpu);
        return ExecutionReturned(0x81b90bu);
    }
    OpTxa(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x7fu);
    PushAccumulator16(memory, cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_RECORDS));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x50u));
    if (!BattleCall(&battle, 0xb924u, 0x81fbc6u, 3u) ||
        !BattleCall(&battle, 0xb928u, 0x81fb8eu, 3u))
        return BattleChildUnwound(&battle);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAndValue(cpu, 0xf800u);
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    PullAccumulator16(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, 0x004202u);
    OpLoadA(cpu, 15u);
    OpSta(memory, cpu, 0x004203u);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x13dau);
    cpu->carry = false;
    OpAdc(memory, cpu, 0x004216u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0u);
    cpu->carry = false;
    OpLda(memory, cpu, OpAbsX(cpu, 5u));
    OpAdc(memory, cpu, OpDp(cpu, 0x22u));
    cpu->carry = true;
    OpSbcValue(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    cpu->carry = false;
    OpLda(memory, cpu, OpAbsX(cpu, 7u));
    OpAdc(memory, cpu, OpDp(cpu, 0x23u));
    cpu->carry = true;
    OpSbcValue(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, 0x23u));
    OpLda(memory, cpu, OpAbsX(cpu, 1u));
    OpAndValue(cpu, 0x80u);
    if (!cpu->zero)
        OpLoadA(cpu, 0xffu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81b973u);
}

Lufia2ExecutionResult Lufia2BattleCommandFrame(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    if (!BattleCall(&battle, 0xd9d0u, 0x85894au, 3u) ||
        !BattleCall(&battle, 0xd9d4u, 0x859aaau, 3u) ||
        !BattleCall(&battle, 0xd9d8u, 0x858a2fu, 3u) ||
        !BattleCall(&battle, 0xd9dcu, 0x859abcu, 3u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81d9e0u);
}

Lufia2ExecutionResult Lufia2BattleConfirmCommand(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    if (!BattleCall(&battle, 0xd975u, 0x85ec81u, 3u) ||
        !BattleCall(&battle, 0xd979u, 0x81def4u, 2u))
        return BattleChildUnwound(&battle);
    OpLdx(cpu, 0xf077u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLdy(cpu, 0x3006u);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, 0x55u));
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, 0x850000u));
        if (cpu->zero)
            break;
        OpInx(cpu);
        if (!BattleCall(&battle, 0xd992u, 0x81e835u, 2u))
            return BattleChildUnwound(&battle);
        OpSta(memory, cpu, OpAbsY(cpu, 0u));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0x40u));
        OpLda(memory, cpu, OpDp(cpu, 0x55u));
        OpSta(memory, cpu, OpAbsY(cpu, 1u));
        OpSta(memory, cpu, OpAbsY(cpu, 0x41u));
        OpIny(cpu);
        OpIny(cpu);
    }
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0xd9abu, 0x859c08u, 3u) ||
        !BattleCall(&battle, 0xd9afu, 0x859b67u, 3u))
        return BattleChildUnwound(&battle);
    OpSepWidths(cpu, 0x20u);
    do {
        if (!BattleCall(&battle, 0xd9b5u, 0x81d9d0u, 2u))
            return BattleChildUnwound(&battle);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, 0x0012f3u);
        if (!BattleCall(&battle, 0xd9beu, 0x85ec81u, 3u))
            return BattleChildUnwound(&battle);
        OpLda(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED));
        OpBitValue(cpu, 0xa0u);
        if (!cpu->zero)
            break;
        OpLda(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED_HIGH));
    } while (!cpu->negative);
    if (!BattleCall(&battle, 0xd9ccu, 0x81e16fu, 2u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81d9cfu);
}
