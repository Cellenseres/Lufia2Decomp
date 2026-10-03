#include "battle/battle_internal.h"

/* $81:CB77: draw and poll the four-way Battle command selection. */
Lufia2ExecutionResult Lufia2BattleChooseCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    if (!BattleCall(&battle, 0xcb77u, 0x81df0au, 2u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0x97u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    OpLdy(cpu, 0xfde6u);
    OpLoadA(cpu, 0u);
    if (!BattleCall(&battle, 0xcb83u, 0x81b974u, 2u) ||
        !BattleCall(&battle, 0xcb86u, 0x81b9afu, 3u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 3u));
    OpSta(memory, cpu, OpDp(cpu, 1u));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    for (;;) {
        OpLdx(cpu, 0u);
        do {
            OpLda(memory, cpu, OpLongX(cpu, 0x97b55eu));
            OpSta(memory, cpu, OpDp(cpu, 0u));
            OpLda(memory, cpu, OpLongX(cpu, 0x97b55fu));
            OpSta(memory, cpu, OpDp(cpu, 8u));
            OpLda(memory, cpu, OpLongX(cpu, 0x97b560u));
            OpSta(memory, cpu, OpDp(cpu, 9u));
            OpInx(cpu); OpInx(cpu); OpInx(cpu);
            OpPushX(memory, cpu);
            if (!BattleCall(&battle, 0xcbafu, 0x81be58u, 2u))
                return BattleChildUnwound(&battle);
            OpPullX(memory, cpu);
            OpCpx(cpu, 9u);
        } while (!cpu->zero);
        OpSepWidths(cpu, 0x10u);
        OpLda(memory, cpu, OpDp(cpu, 0x47u));
        OpLsrA(cpu); OpLsrA(cpu);
        OpAndValue(cpu, 3u);
        OpTay(cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, 0xb576u)));
        OpRepWidths(cpu, 0x10u);
        OpWriteX(memory, cpu, OpDp(cpu, 0x26u), cpu->x);
        OpLda(memory, cpu, OpLongX(cpu, 0x97b55eu));
        cpu->carry = true;
        OpSbcValue(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLda(memory, cpu, OpLongX(cpu, 0x97b55fu));
        OpSta(memory, cpu, OpDp(cpu, 8u));
        OpLda(memory, cpu, OpLongX(cpu, 0x97b560u));
        OpSta(memory, cpu, OpDp(cpu, 9u));
        if (!BattleCall(&battle, 0xcbddu, 0x81be58u, 2u) ||
            !BattleCall(&battle, 0xcbe0u, 0x859dd4u, 3u) ||
            !BattleCall(&battle, 0xcbe4u, 0x81d9d0u, 2u))
            return BattleChildUnwound(&battle);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, 0x0012f3u);
        if (!BattleCall(&battle, 0xcbedu, 0x85ec81u, 3u))
            return BattleChildUnwound(&battle);
        OpLda(memory, cpu, OpDp(cpu, 0x46u));
        OpAslA(cpu);
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x47u)));
        OpAndValue(cpu, 0x20u);
        OpAndValue(cpu, OpReadM(memory, cpu, OpAbs(cpu, 0x057cu)));
        if (!cpu->zero) {
            OpLoadA(cpu, 0x1bu);
            if (!BattleCall(&battle, 0xcbffu, 0x80953bu, 3u) ||
                !BattleCall(&battle, 0xcc03u, 0x849b3eu, 3u))
                return BattleChildUnwound(&battle);
        }
        OpLda(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED));
        OpBitValue(cpu, 0xa0u);
        if (!cpu->zero) {
            OpLoadA(cpu, 2u);
            if (!BattleCall(&battle, 0xcc16u, 0x80953bu, 3u))
                return BattleChildUnwound(&battle);
            OpLda(memory, cpu, OpDp(cpu, 0x26u));
            OpDecA(cpu);
            OpAndValue(cpu, 3u);
            return ExecutionReturned(0x81cc1fu);
        }
        OpBitValue(cpu, 0x40u);
        if (!cpu->zero) {
            OpLoadA(cpu, 2u);
            if (!BattleCall(&battle, 0xcc22u, 0x80953bu, 3u))
                return BattleChildUnwound(&battle);
            OpLda(memory, cpu, OpDp(cpu, 0x26u));
            OpDecA(cpu);
            OpAndValue(cpu, 3u);
            OpOraValue(cpu, 4u);
            return ExecutionReturned(0x81cc2du);
        }
    }
}
