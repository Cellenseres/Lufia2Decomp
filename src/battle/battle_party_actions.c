#include "lufia2/battle.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum { TURN_ACTOR = 0x54, TURN_PRIORITY = 0x56 };

static void SavePartyActionRegisters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool include_y) {
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    if (include_y)
        PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_STAGED_ACTION));
}

static void RestorePartyActionRegisters(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, bool include_y) {
    OpRepWidths(cpu, 0x30u);
    if (include_y)
        OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

Lufia2ExecutionResult Lufia2BattlePublishPartyAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    static const uint8_t record_offsets[] = {0u, 2u, 6u, 8u};
    SavePartyActionRegisters(memory, cpu, false);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8592d8u, 0x85cdfau, 2u, 0x85u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x8592d8u, 0u};
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8592dbu);
    OpRepWidths(cpu, 0x20u);
    for (unsigned word = 0u; word < 4u; ++word) {
        OpLda(memory, cpu, OpAbs(cpu,
            (uint16_t)(WRAM_BATTLE_STAGED_ACTION + 2u * word)));
        OpSta(memory, cpu, OpLongX(cpu, 0x7f0000u + record_offsets[word]));
    }
    RestorePartyActionRegisters(memory, cpu, false);
    return ExecutionReturned(0x8592feu);
}

Lufia2ExecutionResult Lufia2BattleQueueStagedPartyAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    SavePartyActionRegisters(memory, cpu, true);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85930au, 0x85cdfau, 2u, 0x85u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x85930au, 0u};
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85930du);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_STAGED_ACTION));
    OpSta(memory, cpu, OpDp(cpu, TURN_ACTOR));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_STAGED_ACTION + 8u));
    OpSta(memory, cpu, OpDp(cpu, TURN_PRIORITY));
    OpSepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85931bu, 0x859337u, 3u, 0x85u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x85931bu, 0u};
    RestorePartyActionRegisters(memory, cpu, true);
    return ExecutionReturned(0x859325u);
}
