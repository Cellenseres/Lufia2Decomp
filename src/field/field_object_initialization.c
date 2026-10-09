#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/field.h"

enum {
    OBJECT_CONDITION_TABLE = 0xb8b5u,
    OBJECT_CONDITION_RECORD_SIZE = 3u,
    OBJECT_CONTROL_COUNT = 64u
};

static Lufia2ExecutionResult ObjectInitializationUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2FieldApplyObjectConditions(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8386eau);
    PushDataBank(memory, cpu);
    OpLoadA(cpu, 0x91u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_CONDITION_LIST_OFFSET + 1u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_CONDITION_LIST_OFFSET);
    OpTay(cpu);
    for (;;) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpAbsY(cpu, OBJECT_CONDITION_TABLE));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero) break;
        OpTyx(cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x838702u, 0x8ec338u, 3u, cpu->program_bank))
            return ObjectInitializationUnwound(0x838702u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x838706u, 0x8ec34fu, 3u, cpu->program_bank))
            return ObjectInitializationUnwound(0x838706u);
        OpAndValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E093B)));
        if (!cpu->zero) {
            PushY(memory, cpu);
            OpLda(memory, cpu, OpAbsY(cpu, OBJECT_CONDITION_TABLE));
            OpAndValue(cpu, 0x1fu);
            OpSta(memory, cpu, WRAM_FIELD_SELECTED_OBJECT_RECORD_ID);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x838719u, 0x8387a3u, 3u, cpu->program_bank))
                return ObjectInitializationUnwound(0x838719u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x83871du, 0x83873fu, 2u, cpu->program_bank))
                return ObjectInitializationUnwound(0x83871du);
            OpPullY(memory, cpu);
        }
        for (unsigned byte = 0u; byte < OBJECT_CONDITION_RECORD_SIZE; ++byte)
            OpIny(cpu);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x838727u);
}

Lufia2ExecutionResult Lufia2FieldApplySavedObjectControls(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x838728u);
    TransferDirectToA(cpu);
    do {
        PushAccumulator8(memory, cpu);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x83872au, 0x838ac9u, 3u, cpu->program_bank))
            return ObjectInitializationUnwound(0x83872au);
        if (!cpu->zero) {
            LoadA8(cpu, Pull8(memory, cpu));
            PushAccumulator8(memory, cpu);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x838732u, 0x838848u, 2u, cpu->program_bank))
                return ObjectInitializationUnwound(0x838732u);
            if (!CallChildWithFrame(memory, cpu, child, context,
                    0x838735u, 0x83873fu, 2u, cpu->program_bank))
                return ObjectInitializationUnwound(0x838735u);
        }
        LoadA8(cpu, Pull8(memory, cpu));
        OpIncA(cpu);
        OpCmpValue(cpu, OBJECT_CONTROL_COUNT);
    } while (!cpu->zero);
    return ExecutionReturned(0x83873eu);
}
