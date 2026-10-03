#include "battle/battle_internal.h"
#include "core/snes_registers.h"

/* Apply the tick damage to the staged target and queue its presentation. */
static bool StatusTickApply(BattleContext *battle) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    OpTxy(cpu);
    OpLda(memory, cpu, OpDp(cpu, 3u));
    if (!BattleCall(battle, 0xc688u, 0x81b2dbu, 3u))
        return false;
    OpTyx(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, 0x11u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVL));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x14u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRDIVB));
    OpRepWidths(cpu, 0x20u);
    PushAccumulator16(memory, cpu);
    PullAccumulator16(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    cpu->carry = false;
    OpAdcValue(cpu, 0xffffu);
    TransferDirectToA(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDDIVL));
    OpPushX(memory, cpu);
    if (!BattleCall(battle, 0xc6adu, BATTLE_ROUTINE_RANDOM_FRACTION, 3u))
        return false;
    OpPullX(memory, cpu);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x54u));
    OpLsrA(cpu);
    if (cpu->zero)
        OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpLda(memory, cpu, OpAbsX(cpu, 0x11u));
    cpu->carry = true;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x54u)));
    if (cpu->carry && !cpu->zero) {
        OpSta(memory, cpu, OpAbsX(cpu, 0x11u));
        OpSepWidths(cpu, 0x20u);
    } else {
        OpStz(memory, cpu, OpAbsX(cpu, 0x11u));
        OpRepWidths(cpu, 0x20u);
        OpLoadA(cpu, 4u);
        OpSta(memory, cpu, OpAbsX(cpu, 0x0fu));
        OpStepMem(memory, cpu, OpAbsX(cpu, 0xbau), 1);
        if (cpu->zero)
            OpStepMem(memory, cpu, OpAbsX(cpu, 0xbau), -1);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpDp(cpu, 0u));
        if (cpu->negative) {
            OpLda(memory, cpu, OpAbsX(cpu, 0x50u));
            OpSta(memory, cpu, OpAbs(cpu, 0x09fau));
            if (!BattleCall(battle, 0xc6ebu, 0x81fbdbu, 3u))
                return false;
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, 0x09fcu));
            cpu->carry = false;
            OpAdc(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EXPERIENCE_REWARD));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EXPERIENCE_REWARD));
            if (cpu->carry)
                OpStepMem(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_EXPERIENCE_REWARD + 2u)),
                          1);
            OpLda(memory, cpu, OpAbs(cpu, 0x09feu));
            cpu->carry = false;
            OpAdc(memory, cpu, OpAbs(cpu, WRAM_BATTLE_GOLD_REWARD));
            OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_GOLD_REWARD));
            if (cpu->carry)
                OpStepMem(memory, cpu, OpAbs(cpu, (WRAM_BATTLE_GOLD_REWARD + 2u)), 1);
            OpSepWidths(cpu, 0x20u);
        } else {
            OpStz(memory, cpu, OpAbsX(cpu, 0xbcu));
        }
    }
    OpLda(memory, cpu, OpDp(cpu, 3u));
    if (!BattleCall(battle, 0xc718u, 0x85d9c9u, 3u))
        return false;
    OpLoadA(cpu, 0x80u);
    OpSta(memory, cpu, OpLongX(cpu, 0x7f0000u));
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpLongX(cpu, 0x7f0002u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x54u));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0xffffu));
    OpIncA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, 0x7f0004u));
    OpSepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, 2u), 1);
    return true;
}

/* $81:C652: apply the status-bit-1 HP effect to the staged target. */
static bool BattleApplyStatusTick(BattleContext *battle, uint16_t call_site) {
    const Lufia2Memory *memory = battle->memory;
    Lufia2CpuState *cpu = battle->cpu;
    SimulateJsrFrame(memory, cpu, (uint16_t)(call_site + 2u));
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpOra(memory, cpu, OpDp(cpu, 1u));
    OpSta(memory, cpu, OpDp(cpu, 3u));
    if (!BattleCall(battle, 0xc658u, 0x85d9c9u, 3u))
        return false;
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLdx(cpu, 0x1eu);
    do {
        OpStz(memory, cpu, OpAbs(cpu, SNES_WMDATA));
        OpDex(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpDp(cpu, 3u));
    if (!BattleCall(battle, 0xc66fu, 0x81b2b5u, 3u))
        return false;
    OpCpx(cpu, 0u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsX(cpu, 0x0fu));
        OpBitValue(cpu, 1u);
        if (!cpu->zero && !StatusTickApply(battle))
            return false;
    }
    SimulateRtsFrame(memory, cpu);
    return true;
}

/* $81:C600: status ticks for five allies and six enemies, then presentation. */
Lufia2ExecutionResult Lufia2BattleStatusTick(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleContext battle = BattleContextCreate(memory, cpu, child, child_context, 0x81u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, 2u));
    OpStz(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 0x10u);
    OpSta(memory, cpu, OpDp(cpu, 1u));
    for (unsigned side = 0; side < 2u; ++side) {
        do {
            if (!BattleApplyStatusTick(&battle, side ? 0xc61au : 0xc60bu))
                return BattleChildUnwound(&battle);
            const uint8_t mask = Read8(memory, OpDp(cpu, 1u));
            cpu->carry = (mask & 1u) != 0u;
            Write8(memory, OpDp(cpu, 1u), (uint8_t)(mask >> 1));
            SetNz8(cpu, (uint8_t)(mask >> 1));
        } while (!cpu->carry);
        if (side == 0u) {
            OpLoadA(cpu, 0x80u);
            OpSta(memory, cpu, OpDp(cpu, 0u));
            OpLoadA(cpu, 0x20u);
            OpSta(memory, cpu, OpDp(cpu, 1u));
        }
    }
    OpLda(memory, cpu, OpDp(cpu, 2u));
    if (!cpu->zero) {
        OpLdx(cpu, 0xf2ccu);
        if (!BattleCall(&battle, 0xc628u, 0x8594e7u, 3u))
            return BattleChildUnwound(&battle);
        OpLdx(cpu, 1u);
        OpWriteX(memory, cpu, OpAbs(cpu, 0x1266u), cpu->x);
        OpLdx(cpu, 0xffffu);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER), cpu->x);
        if (!BattleCall(&battle, 0xc638u, 0x8595feu, 3u))
            return BattleChildUnwound(&battle);
        OpLoadA(cpu, 0u);
        if (!BattleCall(&battle, 0xc63eu, 0x81c35fu, 3u))
            return BattleChildUnwound(&battle);
        OpLoadA(cpu, 0xdcu);
        if (!BattleCall(&battle, 0xc644u, 0x81895eu, 3u) ||
            !BattleCall(&battle, 0xc648u, 0x859671u, 3u) ||
            !BattleCall(&battle, 0xc64cu, BATTLE_ROUTINE_FRAME_INPUT, 3u))
            return BattleChildUnwound(&battle);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81c651u);
}
