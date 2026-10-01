#include "battle/battle_internal.h"

static void BattleAdvanceStatusIcon(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_TIMER));
    OpLda(memory, cpu, OpAbs(cpu, BATTLE_ICON_CYCLE_INDEX));
    do {
        OpIncA(cpu);
        OpCmpValue(cpu, 0x0au);
        if (cpu->zero)
            TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, BATTLE_ICON_CYCLE_INDEX));
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x859ea6u));
        OpAndValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_STATUS_MASK)));
        if (!cpu->zero)
            break;
        OpTxa(cpu);
    } while (true);
    OpTxa(cpu);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ICON_CYCLE_INDEX));
    OpLda(memory, cpu, OpLongX(cpu, 0x859eb0u));
    PushY(memory, cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_SPRITE_OFFSET)));
    OpSta(memory, cpu, OpAbsY(cpu, 0x1438u));
    OpLda(memory, cpu, OpAbsY(cpu, 0x1435u));
    OpOraValue(cpu, 0x80u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1435u));
    OpPullY(memory, cpu);
}

static bool BattleUpdateStatusIconTimer(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    OpPushX(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, BATTLE_ICON_RECORD_TIMER));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_TIMER));
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, BATTLE_ICON_RECORD_STATUS));
    OpBitValue(cpu, BATTLE_STATUS_DOWNED);
    if (!cpu->zero) {
        OpPullX(memory, cpu);
        return true;
    }
    OpAndValue(cpu, 0x3bu);
    if (cpu->zero) {
        OpPullX(memory, cpu);
        return true;
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_STATUS_MASK));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x97b418u));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x97ca5eu));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_PERIOD));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_TIMER));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_PERIOD));
    if (cpu->carry) {
        BattleAdvanceStatusIcon(memory, cpu);
    }
    SetAccumulatorWidth(cpu, 0);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_TIMER));
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsY(cpu, BATTLE_ICON_RECORD_TIMER));
    OpPullX(memory, cpu);
    return false;
}

/* $85:8850: advance the five party status-icon records and sprite flags. */
Lufia2ExecutionResult Lufia2BattleAnimateStatusIcons(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_SPRITE_OFFSET), cpu->y);
    for (;;) {
        bool hide_icon = false;
        OpLda(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_STATUS_ICON_RECORDS));
        if (cpu->zero) {
            hide_icon = true;
        } else {
            SetAccumulatorWidth(cpu, 0);
            OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS));
            if (!cpu->zero) {
                hide_icon = BattleUpdateStatusIconTimer(memory, cpu);
            }
        }
        if (hide_icon) {
            PushY(memory, cpu);
            OpLdy(cpu,
                  OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_SPRITE_OFFSET)));
            /* Hidden icons retain A, including downed-status bits. */
            OpSta(memory, cpu, OpAbsY(cpu, 0x1438u));
            OpLda(memory, cpu, OpAbsY(cpu, 0x1435u));
            OpAndValue(cpu, 0x7fu);
            OpSta(memory, cpu, OpAbsY(cpu, 0x1435u));
            OpPullY(memory, cpu);
        }
        SetAccumulatorWidth(cpu, 0);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_SPRITE_OFFSET));
        cpu->carry = 0;
        OpAdcValue(cpu, BATTLE_STATUS_SPRITE_RECORD_SIZE);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_SPRITE_OFFSET));
        SetAccumulatorWidth(cpu, 1);
        OpInx(cpu);
        OpInx(cpu);
        for (unsigned i = 0; i < 4u; ++i)
            OpIny(cpu);
        OpCpy(cpu, BATTLE_PARTY_TARGET_COUNT * BATTLE_STATUS_ICON_RECORD_SIZE);
        if (cpu->zero)
            break;
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8588f1u);
}

/* $85:919C: the original far forwarding entry used by frame upkeep. */
Lufia2ExecutionResult Lufia2BattleUpdateStatusIcons(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);
    if (!BattleCall(&battle, 0x919cu, 0x858850u, 3u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x8591a0u);
}
