#include "battle/battle_internal.h"

static Lufia2ExecutionResult BattleRunActionSubmenu(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    if (!BattleCall(battle, 0xd19au, 0x85ec81u, 3u))
        goto unwound;
    OpLoadA(cpu, 0u);
    if (!BattleCall(battle, 0xd1a0u, 0x81bebcu, 3u) ||
        !BattleCall(battle, 0xd1a4u, 0x81beedu, 3u))
        goto unwound;
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd1aau, 0x859c64u, 3u))
        goto unwound;
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd1b0u, 0x81def4u, 2u))
        goto unwound;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x1cu)));
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
        if (!BattleCall(battle, 0xd1c8u, 0x81e835u, 2u))
            goto unwound;
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
    if (!BattleCall(battle, 0xd21bu, 0x859b67u, 3u))
        goto unwound;
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd221u, 0x85ec81u, 3u))
        goto unwound;
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, 0x2130u));
    OpLoadA(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, 0x212cu));
    OpLoadA(cpu, 0x11u);
    OpSta(memory, cpu, OpAbs(cpu, 0x212du));
    goto draw_rows;
input:
    OpLda(memory, cpu, OpDp(cpu, 0xddu));
    OpBitValue(cpu, 0xa0u);
    if (!cpu->zero)
        goto accept;
    OpLda(memory, cpu, OpDp(cpu, 0xdeu));
    if (cpu->negative)
        goto cancel;
    OpLda(memory, cpu, OpDp(cpu, 0xddu));
    OpBitValue(cpu, 0x40u);
    if (!cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x11u)));
        OpPushX(memory, cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x13u)));
        OpPushX(memory, cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x15u)));
        OpPushX(memory, cpu);
        if (!BattleCall(battle, 0xd259u, 0x859906u, 3u) ||
            !BattleCall(battle, 0xd25du, 0x81def4u, 2u))
            goto unwound;
        OpPullX(memory, cpu);
        OpWriteX(memory, cpu, OpDp(cpu, 0x15u), cpu->x);
        OpPullX(memory, cpu);
        OpWriteX(memory, cpu, OpDp(cpu, 0x13u), cpu->x);
        OpPullX(memory, cpu);
        OpWriteX(memory, cpu, OpDp(cpu, 0x11u), cpu->x);
        OpRepWidths(cpu, 0x20u);
        if (!BattleCall(battle, 0xd26bu, 0x859b67u, 3u))
            goto unwound;
        OpSepWidths(cpu, 0x20u);
        goto poll;
    }
    OpLda(memory, cpu, OpDp(cpu, 0xdeu));
    OpAndValue(cpu, 15u);
    OpSepWidths(cpu, 0x10u);
    OpTay(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x1bu));
    if (!cpu->zero) {
        OpCmpValue(cpu, 2u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, 0xb58au));
            goto direction;
        }
    }
    OpLda(memory, cpu, OpAbsY(cpu, 0xb59au));
direction:
    OpRepWidths(cpu, 0x10u);
    OpSta(memory, cpu, OpDp(cpu, 0xcau));
    OpCmpValue(cpu, 2u);
    if (!cpu->zero)
        OpCmpValue(cpu, 0xfeu);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x46u));
        OpAndValue(cpu, 0x10u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, 0xcau));
            OpAndValue(cpu, 0x20u);
            OpLoadA(cpu, cpu->zero ? 12u : 0xf4u);
            cpu->carry = false;
            OpAdc(memory, cpu, OpDp(cpu, 0x12u));
            OpCmpValue(cpu, 0xe6u);
            if (cpu->carry) {
                OpAndValue(cpu, 1u);
            } else {
                OpCmp(memory, cpu, OpDp(cpu, 0x17u));
                if (cpu->carry) {
                    OpLda(memory, cpu, OpDp(cpu, 0x17u));
                    OpDecA(cpu);
                    OpBitValue(cpu, 1u);
                    if (!cpu->zero) {
                        ExchangeAccumulatorBytes(cpu);
                        OpLda(memory, cpu, OpDp(cpu, 0x12u));
                        OpLsrA(cpu);
                        ExchangeAccumulatorBytes(cpu);
                        OpSbcValue(cpu, 0u);
                    }
                }
            }
            OpSta(memory, cpu, OpDp(cpu, 0x18u));
            cpu->carry = true;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x12u)));
            cpu->carry = false;
            OpAdc(memory, cpu, OpDp(cpu, 0x11u));
            OpCmpValue(cpu, 0xe6u);
            if (cpu->carry) {
                OpStz(memory, cpu, OpDp(cpu, 0x11u));
                goto draw_rows;
            }
            OpSta(memory, cpu, OpDp(cpu, 0x11u));
            cpu->carry = false;
            OpAdcValue(cpu, 12u);
            OpCmp(memory, cpu, OpDp(cpu, 0x17u));
            if (cpu->carry) {
                OpLda(memory, cpu, OpDp(cpu, 0x17u));
                cpu->carry = true;
                OpSbcValue(cpu, 11u);
                OpAndValue(cpu, 0xfeu);
                OpSta(memory, cpu, OpDp(cpu, 0x11u));
            }
            goto draw_rows;
        }
    }
    OpLda(memory, cpu, OpDp(cpu, 0xcau));
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, 0x12u));
    OpCmpValue(cpu, 0xe6u);
    if (!cpu->carry) {
        OpCmp(memory, cpu, OpDp(cpu, 0x17u));
        if (!cpu->carry)
            goto selection;
        OpLda(memory, cpu, OpDp(cpu, 0x12u));
        OpAndValue(cpu, 1u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, 0x17u));
            OpDecA(cpu);
            goto selection;
        }
    }
    OpLda(memory, cpu, OpDp(cpu, 0x12u));
selection:
    OpSta(memory, cpu, OpDp(cpu, 0x18u));
    OpLda(memory, cpu, OpDp(cpu, 0x11u));
    OpCmp(memory, cpu, OpDp(cpu, 0x18u));
    if (cpu->zero)
        goto cursor;
    if (cpu->carry) {
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpDp(cpu, 0x16u));
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, 0x15u));
        OpStepMem(memory, cpu, OpDp(cpu, 0x11u), -1);
        OpStepMem(memory, cpu, OpDp(cpu, 0x11u), -1);
        goto draw_rows;
    }
    cpu->carry = false;
    OpAdcValue(cpu, 12u);
    OpCmp(memory, cpu, OpDp(cpu, 0x18u));
    if (!cpu->zero && cpu->carry)
        goto cursor;
    OpLoadA(cpu, 0xfcu);
    OpSta(memory, cpu, OpDp(cpu, 0x16u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0x15u));
    OpStepMem(memory, cpu, OpDp(cpu, 0x11u), 1);
    OpStepMem(memory, cpu, OpDp(cpu, 0x11u), 1);
draw_rows:
    OpLda(memory, cpu, OpDp(cpu, 0x11u));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd340u, 0x81dfa2u, 2u))
        goto unwound;
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd345u, 0x859c08u, 3u))
        goto unwound;
    OpSepWidths(cpu, 0x20u);
scroll:
    OpLda(memory, cpu, OpDp(cpu, 0x16u));
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, 0x15u));
    OpSta(memory, cpu, OpDp(cpu, 0x16u));
    OpAndValue(cpu, 1u);
    if (!cpu->zero)
        goto draw_cursor;
cursor:
    OpLda(memory, cpu, OpDp(cpu, 0x18u));
    OpSta(memory, cpu, OpDp(cpu, 0x12u));
    cpu->carry = true;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x11u)));
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x14u));
    OpLoadA(cpu, 0u);
    OpLoadA(cpu, cpu->carry ? 1u : 0u);
    cpu->carry = false;
    OpSta(memory, cpu, OpDp(cpu, 0x13u));
draw_cursor:
    OpLda(memory, cpu, OpDp(cpu, 0x13u));
    if (!cpu->zero)
        OpLoadA(cpu, 0x70u);
    cpu->carry = false;
    OpAdcValue(cpu, 8u);
    OpSta(memory, cpu, 0x7e4abeu);
    OpLda(memory, cpu, OpDp(cpu, 0x14u));
    OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
    OpLoadA(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
    OpLoadA(cpu, 0x4eu);
    OpSta(memory, cpu, 0x7e4ac0u);
    OpLoadA(cpu, 0x20u);
    OpAdc(memory, cpu, OpAbs(cpu, 0x4216u));
    OpSta(memory, cpu, 0x7e4abfu);
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, 0x7e4ac1u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, 0x7e4ac2u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x15dau));
    OpSta(memory, cpu, OpAbs(cpu, 0x15d7u));
    OpLda(memory, cpu, OpDp(cpu, 0x1bu));
    OpCmpValue(cpu, 2u);
    if (cpu->zero)
        goto draw_frame;
    OpLoadA(cpu, 0xecu);
    OpSta(memory, cpu, 0x7e4ac3u);
    OpLoadA(cpu, 0x4au);
    OpSta(memory, cpu, 0x7e4ac5u);
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, 0x7e4ac6u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, 0x7e4ac7u);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, 0x15dau));
    OpSta(memory, cpu, OpAbs(cpu, 0x15d7u));
    OpLda(memory, cpu, OpDp(cpu, 0x1bu));
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0x11u));
        OpTax(cpu);
        if (!cpu->zero) {
            OpCmpValue(cpu, 0x18u);
            if (!cpu->zero) {
                OpLoadA(cpu, 0x4cu);
                OpSta(memory, cpu, 0x7e4ac5u);
            }
        }
        OpLda(memory, cpu, OpLongX(cpu, 0xa5db00u));
    } else {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0x11u));
        OpTax(cpu);
        if (!cpu->zero) {
            OpCmpValue(cpu, 0xb4u);
            if (!cpu->zero) {
                OpLoadA(cpu, 0x4cu);
                OpSta(memory, cpu, 0x7e4ac5u);
            }
        }
        OpLda(memory, cpu, OpLongX(cpu, 0xa5d700u));
    }
    OpSta(memory, cpu, 0x7e4ac4u);
draw_frame:
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd444u, 0x859ca9u, 3u))
        goto unwound;
    OpLda(memory, cpu, OpDp(cpu, 0x46u));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
    OpSta(memory, cpu, OpDp(cpu, 0x4au));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x16u));
    OpSta(memory, cpu, OpAbs(cpu, 0x1b22u));
    OpStz(memory, cpu, OpAbs(cpu, 0x1b23u));
    if (!BattleCall(battle, 0xd459u, 0x81d9d0u, 2u))
        goto unwound;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!BattleCall(battle, 0xd462u, 0x85ec81u, 3u))
        goto unwound;
    OpLda(memory, cpu, OpDp(cpu, 0x16u));
    if (!cpu->zero)
        goto scroll;
    OpStz(memory, cpu, OpDp(cpu, 0x15u));
    goto input;
poll:
    if (!BattleCall(battle, 0xd472u, 0x81d9d0u, 2u))
        goto unwound;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!BattleCall(battle, 0xd47bu, 0x85ec81u, 3u))
        goto unwound;
    goto input;
accept:
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x1bu));
    OpCmpValue(cpu, 2u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x12u));
        OpRepWidths(cpu, 0x20u);
        OpAslA(cpu);
        OpAslA(cpu);
        OpAslA(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
    } else {
        OpLda(memory, cpu, OpDp(cpu, 0x12u));
        OpRepWidths(cpu, 0x20u);
        OpAslA(cpu);
        OpAslA(cpu);
        OpAslA(cpu);
        PushAccumulator16(memory, cpu);
        OpAslA(cpu);
        OpAdc(memory, cpu, OpStack(cpu, 1u));
        OpTax(cpu);
        PullAccumulator16(memory, cpu);
        OpSepWidths(cpu, 0x20u);
    }
    OpLda(memory, cpu, OpLongX(cpu, 0x7edf00u));
    OpCmpValue(cpu, 0u);
    if (!cpu->zero)
        goto poll;
    OpLoadA(cpu, 2u);
    if (!BattleCall(battle, 0xd4afu, 0x80953bu, 3u))
        goto unwound;
    TransferDirectToA(cpu);
    OpLda(memory, cpu, 0x001be8u);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x153du));
    OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x1bu));
    OpAslA(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAdc(memory, cpu, OpAbs(cpu, 0x4216u));
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x11u));
    OpSta(memory, cpu, OpAbsX(cpu, 0x1363u));
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    return ExecutionReturned(0x81d4d6u);
cancel:
    OpLoadA(cpu, 1u);
    if (!BattleCall(battle, 0xd4d9u, 0x80953bu, 3u))
        goto unwound;
    OpLoadA(cpu, 1u);
    return ExecutionReturned(0x81d4dfu);
unwound:
    return BattleChildUnwound(battle);
}

Lufia2ExecutionResult Lufia2BattleActionSubmenuResume(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    return BattleRunActionSubmenu(&battle);
}

Lufia2ExecutionResult Lufia2BattleActionSubmenuStart(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    OpSta(memory, cpu, OpDp(cpu, 0x1bu));
    OpLda(memory, cpu, OpDp(cpu, 0x1bu));
    if (cpu->zero) {
        if (!BattleCall(&battle, 0xd135u, 0x81bf3fu, 2u))
            goto unwound;
        OpLdx(cpu, 0xf030u);
    } else {
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            if (!BattleCall(&battle, 0xd141u, 0x81c031u, 2u))
                goto unwound;
            OpLdx(cpu, 0xf047u);
        } else {
            if (!BattleCall(&battle, 0xd149u, 0x81c129u, 2u))
                goto unwound;
            OpLdx(cpu, 0xf05eu);
        }
    }
    OpWriteX(memory, cpu, OpDp(cpu, 0x1cu), cpu->x);
    OpSta(memory, cpu, OpDp(cpu, 0x17u));
    OpStz(memory, cpu, OpDp(cpu, 0x18u));
    OpStz(memory, cpu, OpDp(cpu, 0x11u));
    OpStz(memory, cpu, OpDp(cpu, 0x12u));
    OpStz(memory, cpu, OpDp(cpu, 0x13u));
    OpStz(memory, cpu, OpDp(cpu, 0x14u));
    OpStz(memory, cpu, OpDp(cpu, 0x15u));
    OpStz(memory, cpu, OpDp(cpu, 0x16u));
    OpLda(memory, cpu, OpAbs(cpu, 0x0b53u));
    if (!cpu->zero) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, 0x001be8u);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0x153du));
        OpSta(memory, cpu, OpAbs(cpu, 0x4202u));
        OpLoadA(cpu, 7u);
        OpSta(memory, cpu, OpAbs(cpu, 0x4203u));
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0x1bu));
        OpAslA(cpu);
        OpRepWidths(cpu, 0x20u);
        OpAdc(memory, cpu, OpAbs(cpu, 0x4216u));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0x1363u));
        OpSepWidths(cpu, 0x20u);
        OpSta(memory, cpu, OpDp(cpu, 0x11u));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0x12u));
        OpSta(memory, cpu, OpDp(cpu, 0x18u));
        ExchangeAccumulatorBytes(cpu);
        cpu->carry = true;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x12u)));
        OpSta(memory, cpu, OpDp(cpu, 0x13u));
        OpLsrA(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0x14u));
        OpLoadA(cpu, 0xfeu);
        OpTestBits(memory, cpu, OpDp(cpu, 0x13u), 0u);
    }
    return BattleRunActionSubmenu(&battle);
unwound:
    return BattleChildUnwound(&battle);
}
