#include "lufia2/battle.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum { PARTY_COMMAND_STRIDE = 12 };

Lufia2ExecutionResult Lufia2BattleSetPartyCommandBytes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    for (unsigned member = 0u; member < 4u; ++member)
        OpSta(memory, cpu, WRAM_BATTLE_PARTY_COMMAND_BYTE_0_LONG + member * PARTY_COMMAND_STRIDE);
    return ExecutionReturned(0x859336u);
}

Lufia2ExecutionResult Lufia2BattleSwapFirstTwoMembers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 2u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 2u), cpu->x);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x859444u);
}

Lufia2ExecutionResult Lufia2BattleSwapFirstThirdMembers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS + 2u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS + 2u));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS)));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 4u)));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS), cpu->y);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 4u), cpu->x);
    return ExecutionReturned(0x85945fu);
}

Lufia2ExecutionResult Lufia2BattleReversePartyMembers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS + 2u));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS));
    OpTxa(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_IDS + 2u));
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 6u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 6u), cpu->x);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 2u)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 4u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 2u));
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_RECORDS + 4u), cpu->x);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x85948bu);
}

