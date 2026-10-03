#include "battle/battle_internal.h"

static void BattleCheckPartyStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    OpCpx(cpu, 0u);
    if (cpu->zero) {
        cpu->carry = true;
    } else {
        OpLda(memory, cpu, OpAbsX(cpu, 0x000fu));
        OpAndValue(cpu, 0x34u);
        cpu->carry = !cpu->zero;
    }
    SimulateRtsFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattlePartyStatusGate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t party_pointers[4] = {
        0x0a64u, 0x0a66u, 0x0a68u, 0x0a6au};
    static const uint16_t child_returns[4] = {
        0x923bu, 0x9243u, 0x924bu, 0x9253u};

    for (unsigned i = 0; i < 4u; ++i) {
        OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, party_pointers[i])));
        BattleCheckPartyStatus(memory, cpu, child_returns[i]);
        if (!cpu->carry)
            break;
    }
    return ExecutionReturned(0x859254u);
}

Lufia2ExecutionResult Lufia2BattlePrepareNextFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x81u);

    OpLoadA(cpu, 0x03u);
    OpTestBits(memory, cpu, OpAbs(cpu, WRAM_BATTLE_CONTROL_FLAGS), 1u);
    if (!BattleCall(&battle, 0xc245u, BATTLE_ROUTINE_SYNC_STATUS_MARKERS, 3u) ||
        !BattleCall(&battle, 0xc249u, BATTLE_ROUTINE_SPRITES, 3u))
        return BattleChildUnwound(&battle);

    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
    return ExecutionReturned(0x81c253u);
}

Lufia2ExecutionResult Lufia2BattleSaveWorkArea(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x00bfu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, 0x0a8du));
        OpSta(memory, cpu, OpLongX(cpu, 0x7ff8a6u));
        OpDex(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x8596afu);
}

Lufia2ExecutionResult Lufia2BattleRestoreWorkArea(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x00bfu);
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x7ff8a6u));
        OpSta(memory, cpu, OpAbsX(cpu, 0x0a8du));
        OpDex(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x8596bdu);
}

Lufia2ExecutionResult Lufia2BattleClearSpriteOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0u);
    OpLdy(cpu, 0u);
    do {
        TransferDirectToA(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, 0x13e3u));
        OpSta(memory, cpu, OpAbsY(cpu, 0x13e5u));
        OpLoadA(cpu, cpu->y);
        cpu->carry = false;
        OpAdcValue(cpu, 0x000fu);
        OpLdy(cpu, cpu->accumulator);
        OpCpy(cpu, 0x005au);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x858a02u);
}

Lufia2ExecutionResult Lufia2BattleStageTransfer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x42u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b12u));
    OpLoadA(cpu, 0x0fu);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b13u));
    OpLdx(cpu, 0xa092u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1b14u), cpu->x);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1b16u));
    OpSta(memory, cpu, 0x004377u);
    OpLoadA(cpu, 0x80u);
    OpTestBits(memory, cpu, OpDp(cpu, 0xdau), 1u);
    return ExecutionReturned(0x85ab97u);
}
