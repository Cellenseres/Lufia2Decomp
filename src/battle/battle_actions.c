#include "battle/battle_internal.h"

/* $81:890A: consume three-byte turn entries, then clear effect work. */
Lufia2ExecutionResult Lufia2BattleExecuteTurns(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    if (!BattleCall(&battle, 0x890au, 0x81c2e3u, 2u) ||
        !BattleCall(&battle, 0x890du, 0x81dee9u, 2u) ||
        !BattleCall(&battle, 0x8910u, 0x859dd4u, 3u) ||
        !BattleCall(&battle, 0x8914u, 0x85ec81u, 3u))
        return BattleChildUnwound(&battle);
    OpLdx(cpu, WRAM_BATTLE_TURN_QUEUE);
    OpWriteX(memory, cpu, OpDp(cpu, 0xd5u), cpu->x);
    for (;;) {
        OpLda(memory, cpu, OpAbs(cpu, Read16Direct(memory, cpu, 0xd5u)));
        if (cpu->zero) {
            if (!BattleCall(&battle, 0x8940u, 0x81c600u, 2u) ||
                !BattleCall(&battle, 0x8943u, 0x859099u, 3u) ||
                !BattleCall(&battle, 0x8947u, 0x858f67u, 3u))
                return BattleChildUnwound(&battle);
            break;
        }
        OpCmpValue(cpu, 0xffu);
        if (!cpu->zero) {
            if (!BattleCall(&battle, 0x8925u, 0x85cdaau, 3u))
                return BattleChildUnwound(&battle);
            OpLda(memory, cpu, OpAbs(cpu, Read16Direct(memory, cpu, 0xd5u)));
            if (!BattleCall(&battle, 0x892bu, 0x81a79au, 2u) ||
                !BattleCall(&battle, 0x892eu, 0x8593b7u, 3u))
                return BattleChildUnwound(&battle);
            if (cpu->carry)
                break;
        }
        OpRepWidths(cpu, 0x20u);
        for (unsigned i = 0; i < BATTLE_TURN_ENTRY_SIZE; ++i)
            OpStepMem(memory, cpu, OpDp(cpu, 0xd5u), 1);
        OpSepWidths(cpu, 0x20u);
    }
    if (!BattleCall(&battle, 0x894bu, 0x81dee9u, 2u))
        return BattleChildUnwound(&battle);
    OpLdx(cpu, 0xbfu);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, 0x7e3000u));
        OpDex(cpu);
    } while (!cpu->negative);
    if (!BattleCall(&battle, 0x8959u, 0x859dd4u, 3u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81895du);
}

/* $81:A79A: stage an action and route its original execution children. */
Lufia2ExecutionResult Lufia2BattlePrepareAction(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    if (!BattleCall(&battle, 0xa79au, 0x85ccceu, 3u))
        return BattleChildUnwound(&battle);
    OpLdx(cpu, 0x100u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x0a4bu), cpu->x);
    OpLdx(cpu, 0xffffu);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER), cpu->x);
    PushDataBank(memory, cpu);
    OpStz(memory, cpu, OpAbs(cpu, 0x0a62u));
    OpSta(memory, cpu, 0x7ff44eu);
    OpCmpValue(cpu, 0x20u);
    if (cpu->zero) {
        OpStz(memory, cpu, OpAbs(cpu, 0x0a5cu));
        OpStz(memory, cpu, OpAbs(cpu, 0x1269u));
        OpStz(memory, cpu, OpAbs(cpu, 0x0a5bu));
    } else {
        if (!BattleCall(&battle, 0xa7c1u, 0x81b2b5u, 3u))
            return BattleChildUnwound(&battle);
        TransferDirectToA(cpu);
        OpSta(memory, cpu, 0x7ffab6u);
        OpLoadA(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, 0x1262u));
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpAbs(cpu, 0x0a5cu));
        OpStz(memory, cpu, OpAbs(cpu, 0x1269u));
        OpStz(memory, cpu, OpAbs(cpu, 0x0a5bu));
        OpLda(memory, cpu, OpAbsX(cpu, BATTLE_BATTLER_STATUS));
        OpBitValue(cpu, BATTLE_STATUS_NO_TURN);
        if (!cpu->zero) {
            PullDataBank(memory, cpu);
            return ExecutionReturned(0x81a7e2u);
        }
        OpBitValue(cpu, BATTLE_STATUS_TURN_SELECTED);
        if (!cpu->zero) {
            OpLoadA(cpu, 0x0fu);
            OpSta(memory, cpu, 0x7ff454u);
            goto execute_action;
        }
    }
    OpLda(memory, cpu, 0x7ff44eu);
    if (cpu->negative) {
        PushAccumulator8(memory, cpu);
        if (!BattleCall(&battle, 0xa815u, 0x85cdd0u, 3u))
            return BattleChildUnwound(&battle);
        OpLoadA(cpu, Pull8(memory, cpu));
        OpSta(memory, cpu, 0x7ff44eu);
        OpLoadA(cpu, 0xe0u);
        OpSta(memory, cpu, OpAbs(cpu, 0x1262u));
        if (!BattleCall(&battle, 0xa823u, 0x81b1c9u, 3u))
            return BattleChildUnwound(&battle);
    } else {
        OpCmpValue(cpu, 0x10u);
        if (cpu->zero) {
            PushAccumulator8(memory, cpu);
            if (!BattleCall(&battle, 0xa800u, 0x85cdd0u, 3u))
                return BattleChildUnwound(&battle);
            OpLoadA(cpu, Pull8(memory, cpu));
            OpSta(memory, cpu, 0x7ff44eu);
            OpLoadA(cpu, 0xf0u);
            OpSta(memory, cpu, OpAbs(cpu, 0x1262u));
            if (!BattleCall(&battle, 0xa80eu, 0x81b1a3u, 3u))
                return BattleChildUnwound(&battle);
        } else if (!BattleCall(&battle, 0xa7f9u, 0x85cdd0u, 3u)) {
            return BattleChildUnwound(&battle);
        }
    }
execute_action:
    OpLda(memory, cpu, OpAbs(cpu, 0x0a5bu));
    if (cpu->zero && !BattleCall(&battle, 0xa82cu, 0x81a832u, 3u))
        return BattleChildUnwound(&battle);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81a831u);
}
