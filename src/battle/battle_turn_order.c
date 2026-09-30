#include "battle/battle_internal.h"
#include "core/snes_registers.h"

enum { BATTLE_TURN_QUEUE = 0x1b8c, BATTLE_PARTY_COUNT = 0x153c };

/* $85:CDFA: decode the first target bit into a Battle action record. */
static void BattleActionRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    OpSta(memory, cpu, OpAbs(cpu, 0x09fbu));
    OpAndValue(cpu, 0x80u);
    if (!cpu->zero)
        OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, 0x09fau));
    OpLoadA(cpu, 0xffu);
    do {
        const uint32_t address = OpAbs(cpu, 0x09fbu);
        const uint8_t value = Read8(memory, address);
        OpIncA(cpu);
        cpu->carry = (value & 1u) != 0u;
        Write8(memory, address, (uint8_t)(value >> 1));
        SetNz8(cpu, (uint8_t)(value >> 1));
    } while (!cpu->carry);
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbs(cpu, 0x09fau));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x00ffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x859eecu));
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);
}

/* $85:9337: insert a three-byte target/priority entry, descending. */
static void BattleInsertTurn(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint8_t return_bank, uint16_t return_address) {
    static const uint8_t staged_bytes[3] = {0x54u, 0x56u, 0x57u};
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, BATTLE_TURN_QUEUE));
        if (cpu->zero)
            break;
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, BATTLE_TURN_QUEUE + 1u)));
        OpCpy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x56u)));
        if (!cpu->carry)
            break;
        OpInx(cpu);
        OpInx(cpu);
        OpInx(cpu);
    }
    OpWriteX(memory, cpu, OpDp(cpu, 0xcau), cpu->x);
    OpLdy(cpu, 0x21u);
    do {
        OpDey(cpu);
        OpDey(cpu);
        OpDey(cpu);
        for (unsigned i = 0; i < 3u; ++i) {
            OpLda(memory, cpu, OpAbsY(cpu, (uint16_t)(BATTLE_TURN_QUEUE + i)));
            OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(BATTLE_TURN_QUEUE + 3u + i)));
        }
        OpCpy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0xcau)));
    } while (!cpu->zero);
    for (unsigned i = 0; i < 3u; ++i) {
        OpLda(memory, cpu, OpDp(cpu, staged_bytes[i]));
        OpSta(memory, cpu, OpAbsX(cpu, (uint16_t)(BATTLE_TURN_QUEUE + i)));
    }
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $80:834C: 16x8 product in DP $51-$53, preserving A/X/status. */
static void BattlePriorityProduct(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x85u, 0xdd27u);
    PushAccumulator8(memory, cpu);
    OpPushX(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, 0x53u));
    OpLda(memory, cpu, OpDp(cpu, 0x50u));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpLda(memory, cpu, OpDp(cpu, 0x4eu));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYB));
    OpLda(memory, cpu, OpDp(cpu, 0x4fu));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x50u));
    OpRepWidths(cpu, 0x30u);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, SNES_RDMPYL)));
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    OpWriteX(memory, cpu, OpDp(cpu, 0x51u), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, 0x52u));
    cpu->carry = false;
    OpAdc(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    OpSta(memory, cpu, OpDp(cpu, 0x52u));
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullX(memory, cpu);
    OpLoadA(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $85:DD19: randomize DP $56 around its staged priority. */
static void BattleRandomizePriority(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x81u, return_address);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x54u));
    OpSta(memory, cpu, OpDp(cpu, 0x50u));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x56u)));
    OpWriteX(memory, cpu, OpDp(cpu, 0x4eu), cpu->x);
    BattlePriorityProduct(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 0x52u));
    OpIncA(cpu);
    OpCmpValue(cpu, 2u);
    OpAdcValue(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, 0x58u));
    BattleCallRandomFraction(memory, cpu, 0xdd38u);
    OpSta(memory, cpu, OpDp(cpu, 0x5au));
    OpLda(memory, cpu, OpDp(cpu, 0x58u));
    BattleCallRandomFraction(memory, cpu, 0xdd40u);
    cpu->carry = false;
    OpAdc(memory, cpu, OpDp(cpu, 0x5au));
    OpSta(memory, cpu, OpDp(cpu, 0x5au));
    OpLda(memory, cpu, OpDp(cpu, 0x4eu));
    cpu->carry = true;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x58u)));
    if (cpu->carry) {
        cpu->carry = false;
        OpAdc(memory, cpu, OpDp(cpu, 0x5au));
        if (cpu->carry)
            OpLoadA(cpu, 0xffffu);
    } else {
        cpu->carry = false;
        OpAdc(memory, cpu, OpDp(cpu, 0x5au));
        if (!cpu->carry)
            TransferDirectToA(cpu);
    }
    OpSta(memory, cpu, OpDp(cpu, 0x56u));
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleQueueEnemyTurns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0u);
    do {
        OpLdy(cpu, OpReadX(memory, cpu, OpAbsX(cpu, 0x0a6eu)));
        if (!cpu->zero) {
            OpLda(memory, cpu, OpAbsY(cpu, 0x000fu));
            OpBitValue(cpu, 0x2cu);
            if (cpu->zero) {
                OpPushX(memory, cpu);
                PushY(memory, cpu);
                OpRepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpAbsY(cpu, 0x002fu));
                cpu->carry = false;
                OpAdc(memory, cpu, OpAbsY(cpu, 0x003du));
                OpSta(memory, cpu, OpDp(cpu, 0x56u));
                OpSepWidths(cpu, 0x20u);
                OpLoadA(cpu, 0x0du);
                OpSta(memory, cpu, OpDp(cpu, 0x54u));
                BattleRandomizePriority(memory, cpu, 0xc27du);
                OpLoadA(cpu, 0x80u);
                OpOra(memory, cpu, OpDp(cpu, 0u));
                OpSta(memory, cpu, OpDp(cpu, 0x54u));
                BattleInsertTurn(memory, cpu, 0x81u, 0xc287u);
                OpPullY(memory, cpu);
                OpPullX(memory, cpu);
            }
        }
        cpu->carry = false;
        OpRolMem8(memory, cpu, OpDp(cpu, 0u));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 12u);
    } while (!cpu->zero);
    return ExecutionReturned(0x81c293u);
}

Lufia2ExecutionResult Lufia2BattleQueueCapsuleTurn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0x0a6cu)));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbsY(cpu, 0x000fu));
        OpBitValue(cpu, 0x2cu);
        if (cpu->zero) {
            PushY(memory, cpu);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsY(cpu, 0x002fu));
            cpu->carry = false;
            OpAdc(memory, cpu, OpAbsY(cpu, 0x003du));
            OpSta(memory, cpu, OpDp(cpu, 0x56u));
            OpSepWidths(cpu, 0x20u);
            OpLoadA(cpu, 0x0du);
            OpSta(memory, cpu, OpDp(cpu, 0x54u));
            BattleRandomizePriority(memory, cpu, 0xc2b5u);
            OpLoadA(cpu, 0x10u);
            OpSta(memory, cpu, OpDp(cpu, 0x54u));
            BattleInsertTurn(memory, cpu, 0x81u, 0xc2bdu);
            OpPullY(memory, cpu);
        }
    }
    return ExecutionReturned(0x81c2bfu);
}

/* $85:92CE/$85:92FF: publish the descriptor, then its queued priority. */
static void BattlePublishPartyTurn(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t descriptor_offsets[4] = {0u, 2u, 6u, 8u};
    SimulateJslFrame(memory, cpu, 0x85u, 0x92bcu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0x1b7fu));
    BattleActionRecord(memory, cpu, 0x92dau);
    OpRepWidths(cpu, 0x20u);
    for (unsigned i = 0; i < 4u; ++i) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(0x1b7fu + 2u * i)));
        OpSta(memory, cpu, OpLongX(cpu, 0x7f0000u + descriptor_offsets[i]));
    }
    OpRepWidths(cpu, 0x30u);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);

    SimulateJslFrame(memory, cpu, 0x85u, 0x92c0u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0x1b7fu));
    BattleActionRecord(memory, cpu, 0x930cu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0x1b7fu));
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpLda(memory, cpu, OpAbs(cpu, 0x1b87u));
    OpSta(memory, cpu, OpDp(cpu, 0x56u));
    OpSepWidths(cpu, 0x20u);
    BattleInsertTurn(memory, cpu, 0x85u, 0x931eu);
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleQueuePartyTurns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0u);
    do {
        OpPushX(memory, cpu);
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        OpSta(memory, cpu, 0x001be8u);
        OpAslA(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, 0x0a64u));
        OpSta(memory, cpu, OpDp(cpu, 0xd5u));
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, 0x000fu));
        OpSepWidths(cpu, 0x20u);
        OpBitValue(cpu, 0x10u);
        if (!cpu->zero) {
            OpStz(memory, cpu, OpAbs(cpu, 0x1b81u));
            TransferDirectToA(cpu);
            OpLda(memory, cpu, 0x001be8u);
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, 0x96ffecu));
            OpSta(memory, cpu, OpAbs(cpu, 0x1b7fu));
            OpSta(memory, cpu, OpDp(cpu, 0x54u));
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0xd5u)));
            OpLoadA(cpu, 0x0fu);
            OpSta(memory, cpu, OpAbs(cpu, 0x1b83u));
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsX(cpu, 0x002fu));
            cpu->carry = false;
            OpAdc(memory, cpu, OpAbsX(cpu, 0x003du));
            OpSta(memory, cpu, OpAbs(cpu, 0x1b87u));
            BattlePublishPartyTurn(memory, cpu);
            OpSepWidths(cpu, 0x20u);
        }
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpTxa(cpu);
        OpCmp(memory, cpu, OpAbs(cpu, BATTLE_PARTY_COUNT));
    } while (!cpu->zero);
    return ExecutionReturned(0x8592cdu);
}
