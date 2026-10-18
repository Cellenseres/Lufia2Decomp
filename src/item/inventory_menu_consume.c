#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/item.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2InventoryConsumeMenuItem(const Lufia2Memory *memory,
                                                     Lufia2CpuState *cpu,
                                                     Lufia2PushedChildCall child,
                                                     void *context) {
    if (!child || cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x81f034u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    PushAccumulator16(memory, cpu);
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpAndValue(cpu, 0x01ffu);
    OpOraValue(cpu, 0x0200u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpSepWidths(cpu, 0x20u);
    if (!CallChildWithFrame(memory, cpu, child, context, 0x81f04cu, 0x81f15bu, 3u,
                            0x81u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x81f04cu, 0u};
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x81f050u);
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f056u);
}
