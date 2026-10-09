#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/system.h"

enum {
    DP_SAVE_SELECTED_FILE = 0x65u,
    SAVE_FILE_HIGH_FLAGS = 0xf0u,
    SAVED_FILE_CHECK_STATUS = 8u
};

Lufia2ExecutionResult Lufia2SaveCheckGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809073u);
    cpu->carry = 0u;
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_SAVE_SELECTED_FILE));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x80907fu, 0x80914bu, 3u, cpu->program_bank)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x80907fu);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpLda(memory, cpu, WRAM_SAVE_FILE_BUFFER);
    OpAndValue(cpu, SAVE_FILE_HIGH_FLAGS);
    if (cpu->zero) {
        OpLda(memory, cpu, OpStack(cpu, SAVED_FILE_CHECK_STATUS));
        OpOraValue(cpu, 1u);
        OpSta(memory, cpu, OpStack(cpu, SAVED_FILE_CHECK_STATUS));
    }
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x809098u);
}
