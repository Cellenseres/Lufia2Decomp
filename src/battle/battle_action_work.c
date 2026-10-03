#include "battle/battle_internal.h"

/* Fill an action work area with the DP value. */
static Lufia2ExecutionResult ClearActionWork(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu, uint32_t base,
                                             uint16_t last, bool bytes,
                                             uint32_t return_pc) {
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    if (bytes)
        SetAccumulatorWidth(cpu, 1);
    OpLdx(cpu, last);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, base));
        OpDex(cpu);
        if (!bytes)
            OpDex(cpu);
    } while (!cpu->negative);
    if (bytes)
        SetAccumulatorWidth(cpu, 0);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(return_pc);
}

/* Clears the 0x84-byte action work area at $7F:F452. */
Lufia2ExecutionResult Lufia2BattleClearActionWork(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    return ClearActionWork(memory, cpu, 0x7ff452u, 0x82u, false, 0x85cce2u);
}

/* Clear the saved action work copy at $7F:F4DA. */
Lufia2ExecutionResult Lufia2BattleClearSavedActionWork(const Lufia2Memory *memory,
                                                       Lufia2CpuState *cpu) {
    return ClearActionWork(memory, cpu, 0x7ff4dau, 0x82u, false, 0x85ccf7u);
}

/* Clears the 0x14A-byte action record area at $7F:F60C. */
Lufia2ExecutionResult Lufia2BattleClearActionRecords(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu) {
    return ClearActionWork(memory, cpu, 0x7ff60cu, 0x149u, true, 0x85cd0fu);
}

/* Copy the action work area to its save, or back. */
static Lufia2ExecutionResult CopyActionWork(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu, bool restore) {
    OpLdx(cpu, 0x87u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, restore ? 0x7ff4d6u : 0x7ff44eu));
        OpSta(memory, cpu, OpLongX(cpu, restore ? 0x7ff44eu : 0x7ff4d6u));
        OpDex(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(restore ? 0x85cda9u : 0x85cd9au);
}

/* Saves the action work area. */
Lufia2ExecutionResult Lufia2BattleSaveActionWork(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    return CopyActionWork(memory, cpu, false);
}

/* Restores the action work area from the saved copy. */
Lufia2ExecutionResult Lufia2BattleRestoreActionWork(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    return CopyActionWork(memory, cpu, true);
}

/* $85:CDFA: select the record for the first set bit, retaining the zero-mask loop. */
Lufia2ExecutionResult Lufia2BattleActionRecordPointer(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, 0x09fbu));
    OpAndValue(cpu, 0x80u);
    if (!cpu->zero)
        OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09fau));
    OpLoadA(cpu, 0xffu);
    do {
        OpIncA(cpu);
        const uint32_t address = OpAbs(cpu, 0x09fbu);
        const uint8_t value = Read8(memory, address);
        cpu->carry = (value & 1u) != 0u;
        Write8(memory, address, (uint8_t)(value >> 1));
        SetNz8(cpu, (uint8_t)(value >> 1));
    } while (!cpu->carry);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpAbs(cpu, 0x09fau));
    SetAccumulatorWidth(cpu, 0);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x859eecu));
    OpTax(cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x85ce20u);
}

/* Push P, DB and A/X/Y; DB = program bank. */
static void SaveActionRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
}

/* Pops what SaveActionRegisters pushed. */
static void RestoreActionRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* Copy 12 bytes from $7F:0000+X into the named record. */
static Lufia2ExecutionResult LoadActionRecord(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu,
                                              Lufia2PushedChildCall child,
                                              void *child_context, bool action) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);
    if (action && !BattleCall(&battle, 0xcdd0u, 0x85ccceu, 3u))
        return BattleChildUnwound(&battle);
    SaveActionRegisters(memory, cpu);
    if (!BattleCall(&battle, action ? 0xcddfu : 0xcdb5u, 0x85cdfau, 2u))
        return BattleChildUnwound(&battle);
    OpLdy(cpu, 0u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
        const uint8_t pointer = action ? 0xb8u : 0xb5u;
        const uint8_t low = Read8(memory, OpDp(cpu, pointer));
        const uint8_t high = Read8(memory, OpDp(cpu, (uint8_t)(pointer + 1u)));
        const uint8_t bank = Read8(memory, OpDp(cpu, (uint8_t)(pointer + 2u)));
        const uint32_t address = low | ((uint32_t)high << 8) | ((uint32_t)bank << 16);
        OpSta(memory, cpu, LongIndexedAddress(address, cpu->y));
        OpInx(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x0cu);
    } while (!cpu->zero);
    RestoreActionRegisters(memory, cpu);
    return ExecutionReturned(action ? 0x85cdf9u : 0x85cdcfu);
}

/* Loads a turn record through the pointer at DP $B5. */
Lufia2ExecutionResult Lufia2BattleLoadTurnRecord(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *child_context) {
    return LoadActionRecord(memory, cpu, child, child_context, false);
}

/* Run $85:CCCE, then load the record at DP $B8. */
Lufia2ExecutionResult Lufia2BattleLoadActionRecord(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu,
                                                   Lufia2PushedChildCall child,
                                                   void *child_context) {
    return LoadActionRecord(memory, cpu, child, child_context, true);
}
