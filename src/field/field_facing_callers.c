#include "lufia2/field.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum {
    FACING_ACTION_TABLE = 0x83c1a5
};

static uint8_t FacingCallerContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

static uint8_t FacingCallerChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame, Lufia2ExecutionResult *result) {
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

Lufia2ExecutionResult Lufia2FieldProbeFacingObject(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FacingCallerContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83c3dcu);
    Lufia2ExecutionResult result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpCmpValue(cpu, 4u);
    if (!cpu->zero) {
        if (!FacingCallerChild(memory, cpu, child, context,
                0x83c3e3u, 0x83c0efu, 2u, &result) ||
            !FacingCallerChild(memory, cpu, child, context,
                0x83c3e6u, 0x83f988u, 2u, &result))
            return result;
        OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
        if (!FacingCallerChild(memory, cpu, child, context,
                0x83c3ebu, 0x83bf33u, 2u, &result) ||
            !FacingCallerChild(memory, cpu, child, context,
                0x83c3eeu, 0x83f988u, 2u, &result))
            return result;
        OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
        if (!cpu->zero) {
            TransferDirectToA(cpu);
            return ExecutionReturned(0x83c3f6u);
        }
    }
    if (!FacingCallerChild(memory, cpu, child, context,
            0x83c3f7u, 0x83bf33u, 2u, &result))
        return result;
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpSta(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
    if (!FacingCallerChild(memory, cpu, child, context,
            0x83c407u, 0x83fb51u, 3u, &result))
        return result;
    OpBitValue(cpu, 1u);
    return ExecutionReturned(0x83c40du);
}

Lufia2ExecutionResult Lufia2FieldCompareFacingTileHeight(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FacingCallerContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83c40eu);
    Lufia2ExecutionResult result;
    if (!FacingCallerChild(memory, cpu, child, context,
            0x83c40eu, 0x83bf33u, 2u, &result) ||
        !FacingCallerChild(memory, cpu, child, context,
            0x83c411u, 0x83f988u, 2u, &result))
        return result;
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_FACING_TILE_HEIGHT));
    if (!FacingCallerChild(memory, cpu, child, context,
            0x83c417u, 0x83c0efu, 2u, &result) ||
        !FacingCallerChild(memory, cpu, child, context,
            0x83c41au, 0x83f988u, 2u, &result))
        return result;
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_LEADER_TILE_HEIGHT));
    if (!FacingCallerChild(memory, cpu, child, context,
            0x83c420u, 0x83fb71u, 3u, &result))
        return result;
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_LEADER_CELL_CATEGORY));
    OpCmpValue(cpu, 2u);
    if (cpu->zero) {
        cpu->carry = 0u;
        return ExecutionReturned(0x83c43fu);
    }
    OpCmpValue(cpu, 3u);
    if (cpu->zero) {
        cpu->carry = 0u;
        return ExecutionReturned(0x83c43fu);
    }
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpCmpValue(cpu, 4u);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_LEADER_TILE_HEIGHT));
        OpCmp(memory, cpu, OpAbs(cpu, WRAM_FIELD_FACING_TILE_HEIGHT));
        if (!cpu->carry) {
            cpu->carry = 0u;
            return ExecutionReturned(0x83c43fu);
        }
    }
    cpu->carry = 1u;
    return ExecutionReturned(0x83c441u);
}

Lufia2ExecutionResult Lufia2FieldSpawnLeaderSecondaryAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!FacingCallerContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83fc1cu);
    PushAccumulator8(memory, cpu);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    Lufia2ExecutionResult result;
    if (!FacingCallerChild(memory, cpu, child, context,
            0x83fc20u, 0x83ab4fu, 3u, &result))
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, FACING_ACTION_TABLE));
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x83fc2cu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x20u);
    if (!FacingCallerChild(memory, cpu, child, context,
            0x83fc2fu, 0x83d3a1u, 3u, &result))
        return result;
    OpLoadA(cpu, Pull8(memory, cpu));
    if (!FacingCallerChild(memory, cpu, child, context,
            0x83fc34u, 0x83df87u, 3u, &result) ||
        !FacingCallerChild(memory, cpu, child, context,
            0x83fc38u, 0x83c7c7u, 2u, &result))
        return result;
    return ExecutionReturned(0x83fc3bu);
}
