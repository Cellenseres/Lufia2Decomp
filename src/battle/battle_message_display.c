#include "battle/battle_internal.h"

/* $85:9AAA/$9ABC: save/restore the 51 direct bytes used by message rendering. */
static Lufia2ExecutionResult MessageDirectBytes(const Lufia2Memory *memory,
                                                Lufia2CpuState *cpu, bool restore) {
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    OpLdx(cpu, 0x32u);
    do {
        const uint32_t direct = (uint16_t)(cpu->direct_page + cpu->x);
        if (restore) {
            OpLda(memory, cpu, OpAbsX(cpu, 0xf3dbu));
            OpSta(memory, cpu, direct);
        } else {
            OpLda(memory, cpu, direct);
            OpSta(memory, cpu, OpAbsX(cpu, 0xf3dbu));
        }
        OpDex(cpu);
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    return ExecutionReturned(restore ? 0x859acdu : 0x859abbu);
}

Lufia2ExecutionResult Lufia2BattleSaveMessageState(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    return MessageDirectBytes(memory, cpu, false);
}

Lufia2ExecutionResult Lufia2BattleRestoreMessageState(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    return MessageDirectBytes(memory, cpu, true);
}

static void MessageSaveRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
}

static void MessageRestoreRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
}

/* The original shared $967A tail tests only the low byte of the length. */
static void MessageClear(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7eu);
    OpLdx(cpu, 1u);
    OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, 0x1266u));
    if (!cpu->zero) {
        OpLdx(cpu, 0x100u);
        do {
            OpStz(memory, cpu, OpAbsX(cpu, 0x3000u));
            OpDex(cpu);
        } while (!cpu->negative);
        Push8(memory, cpu, cpu->program_bank);
        PullDataBank(memory, cpu);
    }
    MessageRestoreRegisters(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleClearMessage(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    MessageSaveRegisters(memory, cpu);
    MessageClear(memory, cpu);
    return ExecutionReturned(0x85969bu);
}

/* $85:95FE: render text, queue both tile regions, then the original frame. */
Lufia2ExecutionResult Lufia2BattleDisplayMessage(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu,
                                                 Lufia2PushedChildCall child,
                                                 void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);
    MessageSaveRegisters(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, 0x1268u));
    if (!BattleCall(&battle, 0x960eu, 0x81e73bu, 3u))
        return BattleChildUnwound(&battle);
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    if (cpu->zero) {
        MessageClear(memory, cpu);
        return ExecutionReturned(0x85969bu);
    }
    OpLdx(cpu, 0x303u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1250u), cpu->x);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpAbs(cpu, 0x1252u), cpu->x);
    if (!BattleCall(&battle, 0x9622u, 0x81e792u, 3u))
        return BattleChildUnwound(&battle);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER)));
    if (cpu->negative) {
        OpLdx(cpu, 0x78u);
        OpWriteX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_WAIT_COUNTER), cpu->x);
    }
    OpRepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0x9633u, 0x85ecdbu, 3u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0x3800u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a91u));
    OpLoadA(cpu, 0x5800u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a93u));
    OpLoadA(cpu, 0x800u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a8fu));
    if (!BattleCall(&battle, 0x9649u, 0x85ecdbu, 3u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0x3000u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a91u));
    OpLoadA(cpu, 0x5f80u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a93u));
    OpLoadA(cpu, 0x100u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a8fu));
    OpSepWidths(cpu, 0x20u);
    if (!BattleCall(&battle, 0x9661u, 0x85ec81u, 3u) ||
        !BattleCall(&battle, 0x9665u, 0x85aadcu, 3u))
        return BattleChildUnwound(&battle);
    MessageRestoreRegisters(memory, cpu);
    return ExecutionReturned(0x859670u);
}

Lufia2ExecutionResult Lufia2BattleQueueStatusSprites(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);
    if (!BattleCall(&battle, 0x9bdau, 0x85ecdbu, 3u))
        return BattleChildUnwound(&battle);
    OpLoadA(cpu, 0x9800u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a91u));
    OpLoadA(cpu, 0x6c00u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a93u));
    OpLoadA(cpu, 0x800u);
    OpSta(memory, cpu, OpAbsY(cpu, 0x1a8fu));
    return ExecutionReturned(0x859bf0u);
}
