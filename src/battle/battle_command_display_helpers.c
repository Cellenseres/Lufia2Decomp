#include "battle/battle_internal.h"

/* $81:E4D1: five-character name above a party window. */
Lufia2ExecutionResult Lufia2BattlePartyName(const Lufia2Memory *memory,
                                            Lufia2CpuState *cpu,
                                            Lufia2PushedChildCall child,
                                            void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x818811u));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_PARTY_RECORDS)));
    cpu->carry = 1;
    OpSbcValue(cpu, 0x40u);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, 0x12u));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, 0u));
        if (!BattleCall(&battle, 0xe4edu, 0x81e835u, 2u))
            return BattleChildUnwound(&battle);
        OpBitValue(cpu, 0xffu);
        if (!cpu->zero) {
            cpu->carry = 1;
            OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x11u)));
            OpSta(memory, cpu, OpAbsX(cpu, 0u));
        }
        OpInx(cpu);
        OpInx(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x12u), -1);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81e502u);
}

static Lufia2ExecutionResult QueueCommandWindow(BattleContext *battle, uint16_t site,
                                                uint16_t source, uint16_t destination,
                                                uint16_t size, uint32_t exit) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;

    if (!BattleCall(battle, site, 0x85ecdbu, 3u))
        return BattleChildUnwound(battle);
    OpLoadA(cpu, source);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a91u));
    OpLoadA(cpu, destination);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a93u));
    OpLoadA(cpu, size);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a8fu));
    return ExecutionReturned(exit);
}

/* $85:9CD7: queue the action-window tilemap. */
Lufia2ExecutionResult Lufia2BattleQueueActionWindow(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu,
                                                    Lufia2PushedChildCall child,
                                                    void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);

    return QueueCommandWindow(&battle, 0x9cd7u, 0x2800u, 0x5c00u, 0x0400u, 0x859cedu);
}

/* $85:9CEE: queue the command-list tilemap. */
Lufia2ExecutionResult Lufia2BattleQueueListWindow(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu,
                                                  Lufia2PushedChildCall child,
                                                  void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);

    return QueueCommandWindow(&battle, 0x9ceeu, 0x3000u, 0x0800u, 0x0600u, 0x859d04u);
}
