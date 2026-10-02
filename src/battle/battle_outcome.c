#include "battle/battle_internal.h"

/* $85:ED51: five pointer-based and six fixed status bytes. */
static void BattleSnapshotStatus(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x85u, 0x93bdu);
    for (unsigned i = 0; i < 5u; ++i) {
        TransferDirectToA(cpu);
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu,
            (uint16_t)(WRAM_BATTLE_PARTY_RECORDS + 2u * i))));
        if (!cpu->zero)
            OpLda(memory, cpu, OpAbsX(cpu, 0x000fu));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(0x1c01u + i)));
    }
    for (unsigned i = 0; i < 6u; ++i) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(0x162au + 0xbeu * i)));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(0x1c06u + i)));
    }
    SimulateRtlFrame(memory, cpu);
}

/* $85:EE82: clear status of each nonnull party record. */
static void BattleClearPartyStatus(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x85u, 0x9412u);
    for (unsigned i = 0; i < 4u; ++i) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu,
            (uint16_t)(WRAM_BATTLE_PARTY_RECORDS + 2u * i))));
        SimulateJsrFrame(memory, cpu, (uint16_t)(0xee87u + 6u * i));
        if (!cpu->zero)
            OpStz(memory, cpu, OpAbsX(cpu, 0x000fu));
        SimulateRtsFrame(memory, cpu);
    }
    SimulateRtlFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleCheckOutcome(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    BattleSnapshotStatus(memory, cpu);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, 0x11u));
    OpSta(memory, cpu, OpDp(cpu, 0x12u));
    OpLdy(cpu, 6u);
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_PARTY_RECORDS)));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, 0x000fu));
            OpAndValue(cpu, 0x04u);
            if (cpu->zero)
                OpStz(memory, cpu, OpDp(cpu, 0x11u));
        }
        OpDey(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpLdy(cpu, 10u);
    do {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbsY(cpu, WRAM_BATTLE_ENEMY_RECORDS)));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, 0x000fu));
            OpAndValue(cpu, 0x04u);
            if (cpu->zero)
                OpStz(memory, cpu, OpDp(cpu, 0x12u));
        }
        OpDey(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpLda(memory, cpu, OpDp(cpu, 0x11u));
    OpOra(memory, cpu, OpDp(cpu, 0x12u));
    if (cpu->zero) {
        cpu->carry = false;
        PullDataBank(memory, cpu);
        return ExecutionReturned(0x859418u);
    }
    OpLoadA(cpu, BATTLE_CONTROL_FINISHED);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CONTROL_FLAGS), 1u);
    OpLda(memory, cpu, OpDp(cpu, 0x12u));
    if (!cpu->zero) {
        OpLoadA(cpu, 0u);
        OpSta(memory, cpu, WRAM_FIELD_BATTLE_RESULT);
    } else {
        OpLda(memory, cpu, OpDp(cpu, 0x11u));
        if (!cpu->zero) {
            OpLoadA(cpu, 1u);
            OpSta(memory, cpu, WRAM_FIELD_BATTLE_RESULT);
            BattleClearPartyStatus(memory, cpu);
        }
    }
    cpu->carry = true;
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x859415u);
}
