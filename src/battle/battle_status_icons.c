#include "battle/battle_internal.h"

/* $85:8850: advance the five party status-icon records and sprite flags. */
Lufia2ExecutionResult Lufia2BattleAnimateStatusIcons(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1491u), cpu->y);
    for (;;) {
        bool clear = false;
        OpLda(memory, cpu, OpAbsY(cpu, 0x1479u));
        if (cpu->zero) {
            clear = true;
        } else {
            SetAccumulatorWidth(cpu, 0);
            OpLda(memory, cpu, OpAbsX(cpu, 0x0a64u));
            if (!cpu->zero) {
                OpPushX(memory, cpu);
                OpLda(memory, cpu, OpAbsY(cpu, 0x147bu));
                OpSta(memory, cpu, OpAbs(cpu, 0x148fu));
                SetAccumulatorWidth(cpu, 1);
                TransferDirectToA(cpu);
                OpLda(memory, cpu, OpAbsY(cpu, 0x147au));
                OpBitValue(cpu, 4u);
                if (!cpu->zero) {
                    clear = true;
                } else {
                    OpAndValue(cpu, 0x3bu);
                    if (cpu->zero) {
                        clear = true;
                    } else {
                        OpSta(memory, cpu, OpAbs(cpu, 0x148du));
                        OpTax(cpu);
                        OpLda(memory, cpu, OpLongX(cpu, 0x97b418u));
                        OpTax(cpu);
                        OpLda(memory, cpu, OpLongX(cpu, 0x97ca5eu));
                        OpSta(memory, cpu, OpAbs(cpu, 0x148eu));
                        OpLda(memory, cpu, OpAbs(cpu, 0x148fu));
                        OpCmp(memory, cpu, OpAbs(cpu, 0x148eu));
                        if (cpu->carry) {
                            OpStz(memory, cpu, OpAbs(cpu, 0x148fu));
                            OpLda(memory, cpu, OpAbs(cpu, 0x1490u));
                            do {
                                OpIncA(cpu);
                                OpCmpValue(cpu, 0x0au);
                                if (cpu->zero)
                                    TransferDirectToA(cpu);
                                OpSta(memory, cpu, OpAbs(cpu, 0x1490u));
                                OpTax(cpu);
                                OpLda(memory, cpu, OpLongX(cpu, 0x859ea6u));
                                OpAndValue(cpu,
                                           OpReadM(memory, cpu, OpAbs(cpu, 0x148du)));
                                if (!cpu->zero)
                                    break;
                                OpTxa(cpu);
                            } while (true);
                            OpTxa(cpu);
                            OpSta(memory, cpu, OpAbs(cpu, 0x1490u));
                            OpLda(memory, cpu, OpLongX(cpu, 0x859eb0u));
                            PushY(memory, cpu);
                            OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x1491u)));
                            OpSta(memory, cpu, OpAbsY(cpu, 0x1438u));
                            OpLda(memory, cpu, OpAbsY(cpu, 0x1435u));
                            OpOraValue(cpu, 0x80u);
                            OpSta(memory, cpu, OpAbsY(cpu, 0x1435u));
                            OpPullY(memory, cpu);
                        }
                        SetAccumulatorWidth(cpu, 0);
                        OpLda(memory, cpu, OpAbs(cpu, 0x148fu));
                        OpIncA(cpu);
                        OpSta(memory, cpu, OpAbsY(cpu, 0x147bu));
                    }
                }
                OpPullX(memory, cpu);
            }
        }
        if (clear) {
            PushY(memory, cpu);
            OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x1491u)));
            /* The original stores the retained A, including downed-status bits. */
            OpSta(memory, cpu, OpAbsY(cpu, 0x1438u));
            OpLda(memory, cpu, OpAbsY(cpu, 0x1435u));
            OpAndValue(cpu, 0x7fu);
            OpSta(memory, cpu, OpAbsY(cpu, 0x1435u));
            OpPullY(memory, cpu);
        }
        SetAccumulatorWidth(cpu, 0);
        OpLda(memory, cpu, OpAbs(cpu, 0x1491u));
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0du);
        OpSta(memory, cpu, OpAbs(cpu, 0x1491u));
        SetAccumulatorWidth(cpu, 1);
        OpInx(cpu);
        OpInx(cpu);
        for (unsigned i = 0; i < 4u; ++i)
            OpIny(cpu);
        OpCpy(cpu, 0x14u);
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
