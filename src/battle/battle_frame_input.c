#include "battle/battle_internal.h"

/* Wait until DP $40 changes. */
static void WaitForBattleFrame(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x40u));
    do {
        OpCmp(memory, cpu, OpDp(cpu, 0x40u));
    } while (cpu->zero);
}

/* Stop before the first frame wait so a consumer can deliver interrupts. */
Lufia2ExecutionResult Lufia2BattleFrameInputUpkeep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    BattleContext battle =
        BattleContextCreate(memory, cpu, child, child_context, 0x85u);
    if (!BattleCall(&battle, 0xec81u, 0x85919cu, 3u))
        return BattleChildUnwound(&battle);
    OpLda(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
    if (!cpu->zero) {
        if (!BattleCall(&battle, 0xec8bu, BATTLE_ROUTINE_BUILD_SPRITES, 3u))
            return BattleChildUnwound(&battle);
        TransferDirectToA(cpu);
        OpSta(memory, cpu, BATTLE_SPRITE_REBUILD_REQUEST);
    }
    Lufia2ExecutionResult result = ExecutionReturned(0x85ec94u);
    result.flow = LUFIA2_EXECUTION_BOUNDARY;
    cpu->resume_pc = result.pc;
    return result;
}

/* $85:EC81: status/sprite upkeep, original frame waits and pause input. */
Lufia2ExecutionResult Lufia2BattleFrameInput(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *child_context) {
    const uint32_t entry_resume = cpu->resume_pc;
    const Lufia2ExecutionResult upkeep =
        Lufia2BattleFrameInputUpkeep(memory, cpu, child, child_context);
    if (upkeep.flow == LUFIA2_EXECUTION_CHILD_UNWOUND)
        return upkeep;
    cpu->resume_pc = entry_resume;
    WaitForBattleFrame(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, 0x057cu));
    if (!cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x47u));
        OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x4bu)));
        OpAndValue(cpu, 0x10u);
        if (!cpu->zero) {
            OpTestBits(memory, cpu, OpDp(cpu, 0x4bu), 0u);
            OpLda(memory, cpu, OpDp(cpu, 0xd9u));
            PushAccumulator8(memory, cpu);
            OpLda(memory, cpu, OpAbs(cpu, 0x129au));
            PushAccumulator8(memory, cpu);
            OpLoadA(cpu, 1u);
            OpTestBits(memory, cpu, OpDp(cpu, 0xd9u), 0u);
            OpStz(memory, cpu, OpAbs(cpu, 0x129au));
            do {
                WaitForBattleFrame(memory, cpu);
                OpLda(memory, cpu, OpDp(cpu, 0x47u));
                OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x4bu)));
                OpAndValue(cpu, 0x10u);
            } while (cpu->zero);
            OpTestBits(memory, cpu, OpDp(cpu, 0x4bu), 0u);
            LoadA8(cpu, Pull8(memory, cpu));
            OpSta(memory, cpu, OpAbs(cpu, 0x129au));
            LoadA8(cpu, Pull8(memory, cpu));
            OpSta(memory, cpu, OpDp(cpu, 0xd9u));
        }
    }
    SetAccumulatorWidth(cpu, 0);
    OpLda(memory, cpu, OpDp(cpu, 0x46u));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0x4au)));
    OpTestBits(memory, cpu, OpDp(cpu, 0x4au), 0u);
    OpSta(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED));
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x85ecdau);
}
