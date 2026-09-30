#include "battle/battle_internal.h"

static void CopyCommandTiles(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint16_t source, uint32_t destination, uint16_t end) {
    do {
        OpLda(memory, cpu, OpAbsY(cpu, source));
        OpSta(memory, cpu, OpLongX(cpu, destination));
        OpIny(cpu);
        OpInx(cpu);
        OpCpx(cpu, end);
    } while (!cpu->zero);
}

static void CommandTileOffset(Lufia2CpuState *cpu) {
    OpLdy(cpu, 0u);
    OpLdx(cpu, cpu->y);
    OpRepWidths(cpu, 0x20u);
    ExchangeAccumulatorBytes(cpu);
    OpLsrA(cpu);
    OpAndValue(cpu, 0x7f80u);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
}

/* $81:BEBC: command tiles selected by A. */
Lufia2ExecutionResult Lufia2BattleCommandTiles(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    CommandTileOffset(cpu);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x97u);
    CopyCommandTiles(memory, cpu, 0xd6b8u, 0x7e8dc0u, 0x40u);
    CopyCommandTiles(memory, cpu, 0xd6b8u, 0x7e8f80u, 0x80u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81beecu);
}

/* $81:BEED: action-menu tiles. */
Lufia2ExecutionResult Lufia2BattleActionMenuTiles(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    OpLoadA(cpu, 5u);
    CommandTileOffset(cpu);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x97u);
    CopyCommandTiles(memory, cpu, 0xd938u, 0x7e8d40u, 0x40u);
    CopyCommandTiles(memory, cpu, 0xd938u, 0x7e8f00u, 0x80u);
    OpLdx(cpu, 0u);
    CopyCommandTiles(memory, cpu, 0xd938u, 0x7e8d80u, 0x40u);
    CopyCommandTiles(memory, cpu, 0xd938u, 0x7e8f40u, 0x80u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81bf3eu);
}

/* PHB / PHK / PLB preserves A, unlike loading a bank immediate. */
static void CommandCodeBank(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
}

/* $81:DEF4: action-selection window. */
Lufia2ExecutionResult Lufia2BattleActionWindow(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    CommandCodeBank(memory, cpu);
    OpLoadA(cpu, 11u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f3u));
    OpLoadA(cpu, 28u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
    OpLdx(cpu, 0x2842u);
    if (!BattleCall(&battle, 0xdf04u, 0x81e3aeu, 3u))
        return BattleChildUnwound(&battle);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81df09u);
}

/* $81:DF0A: windows for present party members. */
Lufia2ExecutionResult Lufia2BattlePartyWindows(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu,
                                               Lufia2PushedChildCall child,
                                               void *child_context) {
    static const uint16_t blanks[] = {0x2cdeu, 0x2ce0u, 0x2d1eu, 0x2d20u, 0x2d5eu,
                                      0x2d60u, 0x2d9eu, 0x2da0u, 0x2ddeu, 0x2de0u,
                                      0x2e1eu, 0x2e20u, 0x2d5au, 0x2d5cu, 0x2d9au,
                                      0x2d9cu, 0x2d62u, 0x2d64u, 0x2da2u, 0x2da4u};
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    unsigned i;

    CommandCodeBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x2100u);
    for (i = 0; i < sizeof(blanks) / sizeof(blanks[0]); ++i)
        OpSta(memory, cpu, 0x7e0000u | blanks[i]);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    do {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0x8809u)));
        OpWriteX(memory, cpu, OpDp(cpu, 0x11u), cpu->y);
        OpPushX(memory, cpu);
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0x0a64u)));
        if (!cpu->zero) {
            PushY(memory, cpu);
            OpLoadA(cpu, 13u);
            OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
            OpLoadA(cpu, 4u);
            OpSta(memory, cpu, OpAbs(cpu, 0x09f3u));
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x11u)));
            OpLdy(cpu, 0x87b7u);
            OpWriteX(memory, cpu, OpAbs(cpu, 0x09f4u), cpu->y);
            if (!BattleCall(&battle, 0xdf85u, 0x81e503u, 2u))
                return BattleChildUnwound(&battle);
            OpPullY(memory, cpu);
            OpPullX(memory, cpu);
            OpPushX(memory, cpu);
            OpLoadA(cpu, 0x63u);
            OpSta(memory, cpu, OpDp(cpu, 0x11u));
            if (!BattleCall(&battle, 0xdf8fu, 0x81e4d1u, 2u))
                return BattleChildUnwound(&battle);
            OpWriteX(memory, cpu, OpDp(cpu, 0x19u), cpu->x);
            OpLoadA(cpu, 0x7eu);
            OpSta(memory, cpu, OpDp(cpu, 0x1bu));
        }
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 8u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81dfa1u);
}

/* $81:E16F: clear selection windows and queue uploads. */
Lufia2ExecutionResult Lufia2BattleClearActionWindow(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    OpLoadA(cpu, 32u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
    OpLoadA(cpu, 14u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f3u));
    OpLdx(cpu, 0x0100u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x09f6u), cpu->x);
    OpLdx(cpu, 0x2800u);
    if (!BattleCall(&battle, 0xe182u, 0x81e7d2u, 2u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 32u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f2u));
    OpLoadA(cpu, 19u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09f3u));
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x09f6u), cpu->x);
    OpLdx(cpu, 0x3000u);
    if (!BattleCall(&battle, 0xe198u, 0x81e7d2u, 2u))
        return BattleChildUnwound(&battle);
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0xe19du, 0x859cd7u, 3u) ||
        !BattleCall(&battle, 0xe1a1u, 0x859ceeu, 3u))
        return BattleChildUnwound(&battle);
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, 0x15d7u));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, 0x0012f3u);
    if (!BattleCall(&battle, 0xe1b0u, 0x85ec81u, 3u))
        return BattleChildUnwound(&battle);
    return ExecutionReturned(0x81e1b4u);
}
