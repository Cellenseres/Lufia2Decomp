#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum {
    SUBMENU_DP_FIRST_ENTRY = 0x11u,
    SUBMENU_DP_SELECTED_ENTRY = 0x12u,
    SUBMENU_DP_CURSOR_COLUMN = 0x13u,
    SUBMENU_DP_CURSOR_ROW = 0x14u,
    SUBMENU_DP_SCROLL_STEP = 0x15u,
    SUBMENU_DP_SCROLL_PHASE = 0x16u,
    SUBMENU_DP_ENTRY_COUNT = 0x17u,
    SUBMENU_DP_PENDING_ENTRY = 0x18u,
    SUBMENU_DP_KIND = 0x1bu,
    SUBMENU_DP_TITLE = 0x1cu,
    SUBMENU_DP_MOVE_DELTA = 0xcau,
};

static bool BattleDrawSubmenuTitle(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SUBMENU_DP_TITLE)));
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
            return false;
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
    return true;
}

static Lufia2ExecutionResult BattleRunActionSubmenu(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    if (!BattleCall(battle, 0xd19au, 0x85ec81u, 3u))
        return BattleChildUnwound(battle);
    OpLoadA(cpu, 0u);
    if (!BattleCall(battle, 0xd1a0u, 0x81bebcu, 3u) ||
        !BattleCall(battle, 0xd1a4u, 0x81beedu, 3u))
        return BattleChildUnwound(battle);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd1aau, 0x859c64u, 3u))
        return BattleChildUnwound(battle);
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd1b0u, 0x81def4u, 2u))
        return BattleChildUnwound(battle);
    if (!BattleDrawSubmenuTitle(battle))
        return BattleChildUnwound(battle);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd21bu, 0x859b67u, 3u))
        return BattleChildUnwound(battle);
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd221u, 0x85ec81u, 3u))
        return BattleChildUnwound(battle);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_CGWSEL));
    OpLoadA(cpu, 0x1fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TM));
    OpLoadA(cpu, 0x11u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_TS));
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
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY)));
        OpPushX(memory, cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_COLUMN)));
        OpPushX(memory, cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_STEP)));
        OpPushX(memory, cpu);
        if (!BattleCall(battle, 0xd259u, 0x859906u, 3u) ||
            !BattleCall(battle, 0xd25du, 0x81def4u, 2u))
            return BattleChildUnwound(battle);
        OpPullX(memory, cpu);
        OpWriteX(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_STEP), cpu->x);
        OpPullX(memory, cpu);
        OpWriteX(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_COLUMN), cpu->x);
        OpPullX(memory, cpu);
        OpWriteX(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY), cpu->x);
        OpRepWidths(cpu, 0x20u);
        if (!BattleCall(battle, 0xd26bu, 0x859b67u, 3u))
            return BattleChildUnwound(battle);
        OpSepWidths(cpu, 0x20u);
        goto poll;
    }
    OpLda(memory, cpu, OpDp(cpu, 0xdeu));
    OpAndValue(cpu, 15u);
    OpSepWidths(cpu, 0x10u);
    OpTay(cpu);
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_KIND));
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
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_MOVE_DELTA));
    OpCmpValue(cpu, 2u);
    if (!cpu->zero)
        OpCmpValue(cpu, 0xfeu);
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x46u));
        OpAndValue(cpu, 0x10u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_MOVE_DELTA));
            OpAndValue(cpu, 0x20u);
            OpLoadA(cpu, cpu->zero ? 12u : 0xf4u);
            cpu->carry = false;
            OpAdc(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
            OpCmpValue(cpu, 0xe6u);
            if (cpu->carry) {
                OpAndValue(cpu, 1u);
            } else {
                OpCmp(memory, cpu, OpDp(cpu, SUBMENU_DP_ENTRY_COUNT));
                if (cpu->carry) {
                    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_ENTRY_COUNT));
                    OpDecA(cpu);
                    OpBitValue(cpu, 1u);
                    if (!cpu->zero) {
                        ExchangeAccumulatorBytes(cpu);
                        OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
                        OpLsrA(cpu);
                        ExchangeAccumulatorBytes(cpu);
                        OpSbcValue(cpu, 0u);
                    }
                }
            }
            OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_PENDING_ENTRY));
            cpu->carry = true;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY)));
            cpu->carry = false;
            OpAdc(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
            OpCmpValue(cpu, 0xe6u);
            if (cpu->carry) {
                OpStz(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
                goto draw_rows;
            }
            OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
            cpu->carry = false;
            OpAdcValue(cpu, 12u);
            OpCmp(memory, cpu, OpDp(cpu, SUBMENU_DP_ENTRY_COUNT));
            if (cpu->carry) {
                OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_ENTRY_COUNT));
                cpu->carry = true;
                OpSbcValue(cpu, 11u);
                OpAndValue(cpu, 0xfeu);
                OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
            }
            goto draw_rows;
        }
    }
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_MOVE_DELTA));
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
    OpCmpValue(cpu, 0xe6u);
    if (!cpu->carry) {
        OpCmp(memory, cpu, OpDp(cpu, SUBMENU_DP_ENTRY_COUNT));
        if (!cpu->carry)
            goto selection;
        OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
        OpAndValue(cpu, 1u);
        if (!cpu->zero) {
            OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_ENTRY_COUNT));
            OpDecA(cpu);
            goto selection;
        }
    }
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
selection:
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_PENDING_ENTRY));
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
    OpCmp(memory, cpu, OpDp(cpu, SUBMENU_DP_PENDING_ENTRY));
    if (cpu->zero)
        goto cursor;
    if (cpu->carry) {
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_PHASE));
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_STEP));
        OpStepMem(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY), -1);
        OpStepMem(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY), -1);
        goto draw_rows;
    }
    cpu->carry = false;
    OpAdcValue(cpu, 12u);
    OpCmp(memory, cpu, OpDp(cpu, SUBMENU_DP_PENDING_ENTRY));
    if (!cpu->zero && cpu->carry)
        goto cursor;
    OpLoadA(cpu, 0xfcu);
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_PHASE));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_STEP));
    OpStepMem(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY), 1);
    OpStepMem(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY), 1);
draw_rows:
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd340u, 0x81dfa2u, 2u))
        return BattleChildUnwound(battle);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(battle, 0xd345u, 0x859c08u, 3u))
        return BattleChildUnwound(battle);
    OpSepWidths(cpu, 0x20u);
scroll:
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_PHASE));
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_STEP));
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_PHASE));
    OpAndValue(cpu, 1u);
    if (!cpu->zero)
        goto draw_cursor;
cursor:
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_PENDING_ENTRY));
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
    cpu->carry = true;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY)));
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_ROW));
    OpLoadA(cpu, 0u);
    OpLoadA(cpu, cpu->carry ? 1u : 0u);
    cpu->carry = false;
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_COLUMN));
draw_cursor:
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_COLUMN));
    if (!cpu->zero)
        OpLoadA(cpu, 0x70u);
    cpu->carry = false;
    OpAdcValue(cpu, 8u);
    OpSta(memory, cpu, 0x7e4abeu);
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_ROW));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, 12u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLoadA(cpu, 0x4eu);
    OpSta(memory, cpu, 0x7e4ac0u);
    OpLoadA(cpu, 0x20u);
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, 0x7e4abfu);
    OpLoadA(cpu, 0x30u);
    OpSta(memory, cpu, 0x7e4ac1u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, 0x7e4ac2u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_ENABLED));
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_KIND));
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
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CURSOR_ENABLED));
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_KIND));
    OpCmpValue(cpu, 1u);
    if (cpu->zero) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
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
        OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
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
        return BattleChildUnwound(battle);
    OpLda(memory, cpu, OpDp(cpu, 0x46u));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
    OpSta(memory, cpu, OpDp(cpu, 0x4au));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_PHASE));
    OpSta(memory, cpu, OpAbs(cpu, 0x1b22u));
    OpStz(memory, cpu, OpAbs(cpu, 0x1b23u));
    if (!BattleCall(battle, 0xd459u, 0x81d9d0u, 2u))
        return BattleChildUnwound(battle);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!BattleCall(battle, 0xd462u, 0x85ec81u, 3u))
        return BattleChildUnwound(battle);
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_PHASE));
    if (!cpu->zero)
        goto scroll;
    OpStz(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_STEP));
    goto input;
poll:
    if (!BattleCall(battle, 0xd472u, 0x81d9d0u, 2u))
        return BattleChildUnwound(battle);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!BattleCall(battle, 0xd47bu, 0x85ec81u, 3u))
        return BattleChildUnwound(battle);
    goto input;
accept:
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_KIND));
    OpCmpValue(cpu, 2u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
        OpRepWidths(cpu, 0x20u);
        OpAslA(cpu);
        OpAslA(cpu);
        OpAslA(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
    } else {
        OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
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
        return BattleChildUnwound(battle);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_IDS));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_KIND));
    OpAslA(cpu);
    OpRepWidths(cpu, 0x20u);
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
    OpSta(memory, cpu, OpAbsX(cpu, 0x1363u));
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    return ExecutionReturned(0x81d4d6u);
cancel:
    OpLoadA(cpu, 1u);
    if (!BattleCall(battle, 0xd4d9u, 0x80953bu, 3u))
        return BattleChildUnwound(battle);
    OpLoadA(cpu, 1u);
    return ExecutionReturned(0x81d4dfu);
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
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_KIND));
    OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_KIND));
    if (cpu->zero) {
        if (!BattleCall(&battle, 0xd135u, 0x81bf3fu, 2u))
            return BattleChildUnwound(&battle);
        OpLdx(cpu, 0xf030u);
    } else {
        OpCmpValue(cpu, 1u);
        if (cpu->zero) {
            if (!BattleCall(&battle, 0xd141u, 0x81c031u, 2u))
                return BattleChildUnwound(&battle);
            OpLdx(cpu, 0xf047u);
        } else {
            if (!BattleCall(&battle, 0xd149u, 0x81c129u, 2u))
                return BattleChildUnwound(&battle);
            OpLdx(cpu, 0xf05eu);
        }
    }
    OpWriteX(memory, cpu, OpDp(cpu, SUBMENU_DP_TITLE), cpu->x);
    OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_ENTRY_COUNT));
    OpStz(memory, cpu, OpDp(cpu, SUBMENU_DP_PENDING_ENTRY));
    OpStz(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
    OpStz(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
    OpStz(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_COLUMN));
    OpStz(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_ROW));
    OpStz(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_STEP));
    OpStz(memory, cpu, OpDp(cpu, SUBMENU_DP_SCROLL_PHASE));
    OpLda(memory, cpu, OpAbs(cpu, 0x0b53u));
    if (!cpu->zero) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, WRAM_BATTLE_PARTY_SLOT);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_IDS));
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
        OpLoadA(cpu, 7u);
        OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, SUBMENU_DP_KIND));
        OpAslA(cpu);
        OpRepWidths(cpu, 0x20u);
        OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0x1363u));
        OpSepWidths(cpu, 0x20u);
        OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_FIRST_ENTRY));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY));
        OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_PENDING_ENTRY));
        ExchangeAccumulatorBytes(cpu);
        cpu->carry = true;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, SUBMENU_DP_SELECTED_ENTRY)));
        OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_COLUMN));
        OpLsrA(cpu);
        OpSta(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_ROW));
        OpLoadA(cpu, 0xfeu);
        OpTestBits(memory, cpu, OpDp(cpu, SUBMENU_DP_CURSOR_COLUMN), 0u);
    }
    return BattleRunActionSubmenu(&battle);
}
