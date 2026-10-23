#include "battle/battle_internal.h"
#include "lufia2/battle_input_steps.h"

enum {
    INPUT_PAUSE_ENABLED = 0x057cu,
    INPUT_PAUSE_STATE = 0x129au,
    INPUT_HELD_HIGH = 0x47u,
    INPUT_LATCH_HIGH = 0x4bu,
    INPUT_HELD_WORD = 0x46u,
    INPUT_LATCH_WORD = 0x4au,
    INPUT_PAUSE_CONTROL = 0xd9u,
    INPUT_START = 0x10u
};

static bool BattleStartPressed(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, INPUT_HELD_HIGH));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, INPUT_LATCH_HIGH)));
    OpAndValue(cpu, INPUT_START);
    return !cpu->zero;
}

static bool BattleBeginInputPause(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, INPUT_PAUSE_ENABLED));
    if (cpu->zero || !BattleStartPressed(memory, cpu))
        return false;
    OpTestBits(memory, cpu, OpDp(cpu, INPUT_LATCH_HIGH), 0u);
    OpLda(memory, cpu, OpDp(cpu, INPUT_PAUSE_CONTROL));
    PushAccumulator8(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, INPUT_PAUSE_STATE));
    PushAccumulator8(memory, cpu);
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpDp(cpu, INPUT_PAUSE_CONTROL), 0u);
    OpStz(memory, cpu, OpAbs(cpu, INPUT_PAUSE_STATE));
    return true;
}

static bool BattleResumeInputPause(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!BattleStartPressed(memory, cpu))
        return false;
    OpTestBits(memory, cpu, OpDp(cpu, INPUT_LATCH_HIGH), 0u);
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpAbs(cpu, INPUT_PAUSE_STATE));
    LoadA8(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, INPUT_PAUSE_CONTROL));
    return true;
}

static void BattleFilterInput(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    OpLda(memory, cpu, OpDp(cpu, INPUT_HELD_WORD));
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, INPUT_LATCH_WORD)));
    OpTestBits(memory, cpu, OpDp(cpu, INPUT_LATCH_WORD), 0u);
    OpSta(memory, cpu, OpDp(cpu, BATTLE_DP_PAD_FILTERED));
    SetAccumulatorWidth(cpu, 1);
}

static bool BattleInputStepReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x85u && !cpu->direct_page &&
        cpu->accumulator_is_8_bit == 1u && !cpu->index_is_8_bit;
}

Lufia2ExecutionResult Lufia2BattleInputAfterWait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!BattleInputStepReady(cpu))
        return ExecutionHandoff(cpu, 0x85ec9au);
    if (BattleBeginInputPause(memory, cpu))
        return ExecutionHandoff(cpu, 0x85ecb7u);
    BattleFilterInput(memory, cpu);
    return ExecutionHandoff(cpu, 0x85ecdau);
}

Lufia2ExecutionResult Lufia2BattlePauseAfterWait(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!BattleInputStepReady(cpu))
        return ExecutionHandoff(cpu, 0x85ecbdu);
    if (!BattleResumeInputPause(memory, cpu))
        return ExecutionHandoff(cpu, 0x85ecb7u);
    BattleFilterInput(memory, cpu);
    return ExecutionHandoff(cpu, 0x85ecdau);
}

/* Wait until DP $40 changes. */
static void WaitForBattleFrame(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x40u));
    do {
        OpCmp(memory, cpu, OpDp(cpu, 0x40u));
    } while (cpu->zero);
}

/* Leave the frame wait to the consumer. */
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

/* Battle upkeep, frame waits and pause input. */
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
    if (BattleBeginInputPause(memory, cpu)) {
        do {
            WaitForBattleFrame(memory, cpu);
        } while (!BattleResumeInputPause(memory, cpu));
    }
    BattleFilterInput(memory, cpu);
    return ExecutionReturned(0x85ecdau);
}
