#include "field/vehicle_transition_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum {
    DP_REQUESTED_DIRECTION = 0x23,
    DP_ACTOR_UPDATE_SCRIPT = 0x97,
    HEADER_ACTOR_ID = 0x7ef006
};

static uint8_t VehicleChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, 3u, (uint8_t)(site >> 16))) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit ||
            (cpu->index_is_8_bit && site != 0x8ebc52u)) {
        *result = ExecutionHandoff(cpu, site + 4u);
        return 0u;
    }
    return 1u;
}

static Lufia2ExecutionResult VehicleRejected(Lufia2CpuState *cpu) {
    cpu->carry = 0u;
    return ExecutionReturned(0x8ebbd0u);
}

static uint8_t QueueVehicleActorUpdate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint8_t script, uint32_t site, Lufia2ExecutionResult *result) {
    OpLoadA(cpu, script);
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_UPDATE_SCRIPT));
    OpLdy(cpu, 0u);
    return VehicleChild(memory, cpu, child, context,
        site, 0x80b404u, result);
}

static Lufia2ExecutionResult RestoreVehicleActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E089D));
    OpLda(memory, cpu, WRAM_UNK_7FD0FE);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E091D));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E08DD));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E085D));
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_UNK_7FD0FE);
    Lufia2ExecutionResult result;
    if (!VehicleChild(memory, cpu, child, context,
            0x8ebc19u, 0x848204u, &result) ||
        !VehicleChild(memory, cpu, child, context,
            0x8ebc1du, 0x8ebc99u, &result))
        return result;
    OpLoadA(cpu, 0xfdu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E091D));
    if (!VehicleChild(memory, cpu, child, context,
            0x8ebc29u, 0x83a9bau, &result) ||
        !VehicleChild(memory, cpu, child, context,
            0x8ebc2du, 0x83d416u, &result) ||
        !VehicleChild(memory, cpu, child, context,
            0x8ebc31u, 0x83aa30u, &result))
        return result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE4DE));
    OpStz(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E066A));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    if (!VehicleChild(memory, cpu, child, context,
            0x8ebc52u, 0x83a746u, &result))
        return result;
    OpRepWidths(cpu, 0x10u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpLoadA(cpu, 9u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7E070A));
    if (!VehicleChild(memory, cpu, child, context,
            0x8ebc65u, 0x83d416u, &result))
        return result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpLoadA(cpu, 0xf8u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X + 1u));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BF));
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BD));
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E05BE));
    if (!QueueVehicleActorUpdate(memory, cpu, child, context,
            0x48u, 0x8ebc8eu, &result))
        return result;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_TEXT_WAIT_ACTOR));
    cpu->carry = 1u;
    return ExecutionReturned(0x8ebc98u);
}

Lufia2ExecutionResult Lufia2FieldProbeVehicleTerrainBody(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    Lufia2ExecutionResult result;
    if (!VehicleChild(memory, cpu, child, context,
            0x8ebbb9u, 0x83fb71u, &result))
        return result;
    OpCmpValue(cpu, 6u);
    if (!cpu->zero)
        return VehicleRejected(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_REQUESTED_DIRECTION));
    if (!VehicleChild(memory, cpu, child, context,
            0x8ebbc3u, 0x83fb12u, &result) ||
        !VehicleChild(memory, cpu, child, context,
            0x8ebbc7u, 0x83fb71u, &result))
        return result;
    OpCmpValue(cpu, 6u);
    if (!cpu->zero)
        return VehicleRejected(cpu);
    return (Lufia2ExecutionResult){
        LUFIA2_EXECUTION_BOUNDARY, 0x8ebbd1u, 0u};
}

Lufia2ExecutionResult Lufia2FieldTryVehicleTransition(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x8ebbafu);
    Lufia2ExecutionResult result = Lufia2FieldProbeVehicleTerrainBody(
        memory, cpu, child, context);
    if (result.flow != LUFIA2_EXECUTION_BOUNDARY || result.pc != 0x8ebbd1u)
        return result;
    OpLda(memory, cpu, WRAM_UNK_7FD0FE);
    if (!cpu->zero)
        return RestoreVehicleActor(memory, cpu, child, context);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E089D));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    if (!cpu->zero)
        return VehicleRejected(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E08DD));
    OpCmp(memory, cpu, OpDp(cpu, DP_PROBE_X));
    if (!cpu->zero)
        return VehicleRejected(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_UNK_7E085D));
    OpCmp(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    if (!cpu->zero)
        return VehicleRejected(cpu);
    if (!QueueVehicleActorUpdate(memory, cpu, child, context,
            0x49u, 0x8ebbf4u, &result))
        return result;
    cpu->carry = 1u;
    return ExecutionReturned(0x8ebc98u);
}

Lufia2ExecutionResult Lufia2FieldSpawnHeaderActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
            cpu->index_is_8_bit || cpu->direct_page || cpu->decimal || !child)
        return ExecutionHandoff(cpu, 0x83c446u);
    OpLdx(cpu, 0x16u);
    Lufia2ExecutionResult result;
    if (!VehicleChild(memory, cpu, child, context,
            0x83c449u, 0x80bfaau, &result))
        return result;
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    PushAccumulator8(memory, cpu);
    OpLda(memory, cpu, OpLongX(cpu, HEADER_ACTOR_ID));
    if (!VehicleChild(memory, cpu, child, context,
            0x83c454u, 0x83df87u, &result))
        return result;
    OpLoadA(cpu, Pull8(memory, cpu));
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    if (!VehicleChild(memory, cpu, child, context,
            0x83c45bu, 0x83ab4fu, &result))
        return result;
    return ExecutionReturned(0x83c45fu);
}
