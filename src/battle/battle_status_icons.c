#include "battle/battle_internal.h"

/* Each party member's status icon cycles through the ailments it has, one
 * animation frame pair at a time. */
enum {
    ICON_CYCLE_LENGTH = 10u,
    ICON_STATUS_MASK = 0x3bu,
    ICON_MASK_TABLE = 0x859ea6u,
    ICON_TILE_TABLE = 0x859eb0u,
    ICON_PERIOD_CLASS_TABLE = 0x97b418u,
    ICON_PERIOD_TABLE = 0x97ca5eu,
    ICON_SPRITE_STATE = 0x1435u,
    ICON_SPRITE_TILE = 0x1438u,
    ICON_SPRITE_SHOWN = 0x80u,
    ICON_SPRITE_HIDDEN_MASK = 0x7fu,
};

/* Moves the icon to the next ailment the member has (the cycle wraps at ten
 * entries, two frames per ailment), shows that ailment's tile and marks the
 * sprite as visible. */
static void BattleAdvanceStatusIcon(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpAbs(cpu, BATTLE_ICON_TIMER));
    OpLda(memory, cpu, OpAbs(cpu, BATTLE_ICON_CYCLE_INDEX));
    do {
        OpIncA(cpu);
        OpCmpValue(cpu, ICON_CYCLE_LENGTH);
        if (cpu->zero)
            TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, BATTLE_ICON_CYCLE_INDEX));
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ICON_MASK_TABLE));
        OpAndValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_STATUS_MASK)));
        if (!cpu->zero)
            break;
        OpTxa(cpu);
    } while (true);
    OpTxa(cpu);
    OpSta(memory, cpu, OpAbs(cpu, BATTLE_ICON_CYCLE_INDEX));
    OpLda(memory, cpu, OpLongX(cpu, ICON_TILE_TABLE));
    PushY(memory, cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_SPRITE_OFFSET)));
    OpSta(memory, cpu, OpAbsY(cpu, ICON_SPRITE_TILE));
    OpLda(memory, cpu, OpAbsY(cpu, ICON_SPRITE_STATE));
    OpOraValue(cpu, ICON_SPRITE_SHOWN);
    OpSta(memory, cpu, OpAbsY(cpu, ICON_SPRITE_STATE));
    OpPullY(memory, cpu);
}

/* Counts the icon's timer up; once it reaches the period of the member's
 * ailment class the icon advances. Returns true when the icon should be hidden:
 * the member is downed or has no icon-worthy ailment. */
static bool BattleUpdateStatusIconTimer(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    OpPushX(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, BATTLE_ICON_RECORD_TIMER));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_STATE));
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, BATTLE_ICON_RECORD_STATUS));
    OpBitValue(cpu, BATTLE_STATUS_DOWNED);
    if (!cpu->zero) {
        OpPullX(memory, cpu);
        return true;
    }
    OpAndValue(cpu, ICON_STATUS_MASK);
    if (cpu->zero) {
        OpPullX(memory, cpu);
        return true;
    }
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_STATUS_MASK));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ICON_PERIOD_CLASS_TABLE));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ICON_PERIOD_TABLE));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_PERIOD));
    OpLda(memory, cpu, OpAbs(cpu, BATTLE_ICON_TIMER));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_PERIOD));
    if (cpu->carry) {
        BattleAdvanceStatusIcon(memory, cpu);
    }
    SetAccumulatorWidth(cpu, 0);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ICON_STATE));
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
            OpSta(memory, cpu, OpAbsY(cpu, ICON_SPRITE_TILE));
            OpLda(memory, cpu, OpAbsY(cpu, ICON_SPRITE_STATE));
            OpAndValue(cpu, ICON_SPRITE_HIDDEN_MASK);
            OpSta(memory, cpu, OpAbsY(cpu, ICON_SPRITE_STATE));
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
        for (unsigned i = 0; i < BATTLE_STATUS_ICON_RECORD_SIZE; ++i)
            OpIny(cpu);
        OpCpy(cpu, WRAM_BATTLE_STATUS_ICON_RECORDS_COUNT * BATTLE_STATUS_ICON_RECORD_SIZE);
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
