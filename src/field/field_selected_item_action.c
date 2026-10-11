#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

static uint8_t SelectedItemChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame,
    Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x83u)) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

static uint8_t SelectedItemType(Lufia2CpuState *cpu, uint8_t type) {
    OpCmpValue(cpu, type);
    return cpu->zero;
}

static Lufia2ExecutionResult SelectedItemSecondaryAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    Lufia2ExecutionResult result;
    if (!SelectedItemChild(memory, cpu, child, context,
            0x83c2b6u, 0x83c0efu, 2u, &result) ||
        !SelectedItemChild(memory, cpu, child, context,
            0x83c2b9u, 0x83fb71u, 3u, &result))
        return result;
    OpCmpValue(cpu, 0x0eu);
    if (cpu->zero)
        return ExecutionReturned(0x83c2fdu);
    OpCmpValue(cpu, 7u);
    if (cpu->zero)
        return ExecutionReturned(0x83c2fdu);
    OpCmpValue(cpu, 0x0au);
    if (cpu->zero)
        return ExecutionReturned(0x83c2fdu);
    if (!SelectedItemChild(memory, cpu, child, context,
            0x83c2c9u, 0x83c33du, 2u, &result))
        return result;
    if (cpu->carry)
        return ExecutionReturned(0x83c2fdu);
    if (!SelectedItemChild(memory, cpu, child, context,
            0x83c2ceu, 0x83c32du, 2u, &result))
        return result;
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x83c2d1u);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x20u);
    if (!SelectedItemChild(memory, cpu, child, context,
            0x83c2d4u, 0x83d3a1u, 3u, &result))
        return result;
    OpLoadA(cpu, 4u);
    if (!SelectedItemChild(memory, cpu, child, context,
            0x83c2dau, 0x83df87u, 3u, &result))
        return result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_X));
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_Y));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_X + 1u));
    OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_FINE_Y + 1u));
    if (!SelectedItemChild(memory, cpu, child, context,
            0x83c2f5u, 0x83fcb4u, 2u, &result))
        return result;
    return ExecutionReturned(0x83c2fdu);
}

Lufia2ExecutionResult Lufia2FieldDispatchSelectedItemAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x83c24fu);
    Lufia2ExecutionResult result;
    if (!SelectedItemChild(memory, cpu, child, context,
            0x83c24fu, 0x83c0efu, 2u, &result) ||
        !SelectedItemChild(memory, cpu, child, context,
            0x83c252u, 0x83fb71u, 3u, &result))
        return result;
    OpCmpValue(cpu, 2u);
    if (cpu->zero)
        return ExecutionReturned(0x83c289u);
    OpCmpValue(cpu, 3u);
    if (cpu->zero)
        return ExecutionReturned(0x83c289u);
    OpLda(memory, cpu, WRAM_UNK_7FD0FE);
    if (!cpu->zero)
        return ExecutionReturned(0x83c289u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_SELECTED_ITEM));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_SELECTED_ITEM + 1u));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID + 1u));
    if (!SelectedItemChild(memory, cpu, child, context,
            0x83c270u, 0x83c692u, 2u, &result))
        return result;
    OpAndValue(cpu, 0x0fu);
    if (SelectedItemType(cpu, 1u)) {
        OpLoadA(cpu, 0x21u);
        if (!SelectedItemChild(memory, cpu, child, context,
                0x83c29du, 0x83c311u, 2u, &result))
            return result;
    } else if (SelectedItemType(cpu, 3u)) {
        OpLdx(cpu, 0u);
        OpLdy(cpu, 0x16u);
        if (!SelectedItemChild(memory, cpu, child, context,
                0x83c2a8u, 0x80e722u, 3u, &result))
            return result;
        OpLoadA(cpu, 4u);
        ExchangeAccumulatorBytes(cpu);
        OpLoadA(cpu, 5u);
        if (!SelectedItemChild(memory, cpu, child, context,
                0x83c2b1u, 0x83c2feu, 2u, &result))
            return result;
    } else if (SelectedItemType(cpu, 4u)) {
        return SelectedItemSecondaryAction(memory, cpu, child, context);
    } else if (SelectedItemType(cpu, 6u)) {
        OpLoadA(cpu, 0x23u);
        if (!SelectedItemChild(memory, cpu, child, context,
                0x83c28cu, 0x83c311u, 2u, &result))
            return result;
    } else if (SelectedItemType(cpu, 7u)) {
        OpLoadA(cpu, 0x20u);
        ExchangeAccumulatorBytes(cpu);
        OpLoadA(cpu, 0x28u);
        if (!SelectedItemChild(memory, cpu, child, context,
                0x83c296u, 0x83c2feu, 2u, &result))
            return result;
    } else {
        return ExecutionReturned(0x83c289u);
    }
    if (!SelectedItemChild(memory, cpu, child, context,
            0x83c2fau, 0x83c7c7u, 2u, &result))
        return result;
    return ExecutionReturned(0x83c2fdu);
}
