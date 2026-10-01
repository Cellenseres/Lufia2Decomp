#include "battle/battle_internal.h"

/* $81:B1A3: configured script, or the original default at $85:EF20. */
Lufia2ExecutionResult Lufia2BattleRunConfiguredScript(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu,
                                                      Lufia2PushedChildCall child,
                                                      void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x1129u)));
    if (!cpu->zero) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x1124u)));
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0a42u), cpu->x);
        OpLda(memory, cpu, OpAbs(cpu, 0x1126u));
    } else {
        OpLdx(cpu, 0u);
        OpLdy(cpu, 0xef20u);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0a42u), cpu->x);
        OpLoadA(cpu, 0x85u);
    }
    OpSta(memory, cpu, OpAbs(cpu, 0x0a44u));
    if (!BattleCall(&battle, 0xb1c4u, 0x81fac9u, 3u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81b1c8u);
}

/* $81:B1C9: locate the battler's script and its bank-$96 sprite record. */
Lufia2ExecutionResult Lufia2BattleRunBattlerScript(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    SetAccumulatorWidth(cpu, 0);
    OpLda(memory, cpu, 0x7ff44eu);
    SetAccumulatorWidth(cpu, 1);
    if (!BattleCall(&battle, 0xb1d1u, 0x81c5cfu, 2u))
        return BattleChildUnwound(&battle);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0x004au)));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, 0x0050u));
        if (!BattleCall(&battle, 0xb1dcu, 0x81fce2u, 3u))
            return BattleChildUnwound(&battle);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0a42u), cpu->x);
        OpLoadA(cpu, 0x96u);
    } else {
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0a42u), cpu->y);
        OpLdy(cpu, 0xef20u);
        OpLoadA(cpu, 0x85u);
    }
    OpSta(memory, cpu, OpAbs(cpu, 0x0a44u));
    if (!BattleCall(&battle, 0xb1f2u, 0x81fac9u, 3u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81b1f6u);
}

/* $81:B1F7: load the item at record+66 and select its script or $85:EF24. */
Lufia2ExecutionResult Lufia2BattleRunItemScript(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu,
                                                Lufia2PushedChildCall child,
                                                void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0x0066u)));
    OpWriteX(memory, cpu, OpAbs(cpu, 0x0a06u), cpu->y);
    OpPushX(memory, cpu);
    if (!BattleCall(&battle, 0xb1feu, 0x81f1c5u, 3u))
        return BattleChildUnwound(&battle);
    OpPullX(memory, cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x0a09u)));
    if (cpu->zero) {
        OpLdy(cpu, 0u);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0a42u), cpu->y);
        OpLoadA(cpu, 0x85u);
        OpSta(memory, cpu, OpAbs(cpu, 0x0a44u));
        OpLdy(cpu, 0xef24u);
    } else {
        OpWriteX(memory, cpu, OpAbs(cpu, 0x0a42u), cpu->y);
        OpLoadA(cpu, 0x96u);
        OpSta(memory, cpu, OpAbs(cpu, 0x0a44u));
        OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x0b93u)));
    }
    if (!BattleCall(&battle, 0xb223u, 0x81fac9u, 3u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81b227u);
}
