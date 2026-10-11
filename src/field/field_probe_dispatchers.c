#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum { DP_REQUESTED_ACTION = 0x22, DP_PROBE_DIRECTION = 0x23,
       DP_COLLISION_SPAN = 0x9a };

static uint8_t ProbeDispatcherContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page;
}

static uint8_t ProbeDispatcherChild(
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

Lufia2ExecutionResult Lufia2FieldTryLeaderLowerStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ProbeDispatcherContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83bdfcu);
    OpLda(memory, cpu, OpDp(cpu, DP_REQUESTED_ACTION));
    cpu->carry = 0u;
    if (cpu->zero)
        return ExecutionReturned(0x83be32u);
    Lufia2ExecutionResult result;
    if (!ProbeDispatcherChild(memory, cpu, child, context,
            0x83be01u, 0x83c0efu, 2u, &result))
        return result;
    if (!ProbeDispatcherChild(memory, cpu, child, context,
            0x83be04u, 0x83fb71u, 3u, &result))
        return result;
    OpCmpValue(cpu, 1u);
    cpu->carry = 0u;
    if (!cpu->zero)
        return ExecutionReturned(0x83be32u);
    if (!ProbeDispatcherChild(memory, cpu, child, context,
            0x83be0du, 0x83f988u, 2u, &result))
        return result;
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_PROBE_DIRECTION));
    OpTax(cpu);
    uint16_t target = Read16ProgramIndexed(memory, cpu, 0xbe33u, cpu->x);
    if (!ProbeDispatcherChild(memory, cpu, child, context,
            0x83be16u, 0x830000u | target, 2u, &result))
        return result;
    if (!cpu->carry)
        return ExecutionReturned(0x83be32u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 2u);
    if (!ProbeDispatcherChild(memory, cpu, child, context,
            0x83be21u, 0x80e722u, 3u, &result))
        return result;
    OpLda(memory, cpu, OpDp(cpu, DP_REQUESTED_ACTION));
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x83be27u);
    cpu->carry = 0u;
    OpAdcValue(cpu, 8u);
    if (!ProbeDispatcherChild(memory, cpu, child, context,
            0x83be2au, 0x83d350u, 3u, &result))
        return result;
    if (!ProbeDispatcherChild(memory, cpu, child, context,
            0x83be2eu, 0x83c7c7u, 2u, &result))
        return result;
    cpu->carry = 1u;
    return ExecutionReturned(0x83be32u);
}

void Lufia2ActorPrepareCollisionProbe(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushAccumulator8(memory, cpu);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_X));
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpDp(cpu, DP_PROBE_Y));
    OpTyx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE216));
    OpSta(memory, cpu, OpDp(cpu, DP_COLLISION_SPAN));
    TransferDirectToA(cpu);
    OpTxa(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, WRAM_ACTOR_DISPLAY_OFFSET_X));
    if (!cpu->zero) {
        OpLoadA(cpu, 1u);
        OpSta(memory, cpu, OpDp(cpu, DP_COLLISION_SPAN));
    }
    OpLoadA(cpu, Pull8(memory, cpu));
}

Lufia2ExecutionResult Lufia2FieldProbeActorStepCollision(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ProbeDispatcherContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83d87du);
    Lufia2ActorPrepareCollisionProbe(memory, cpu);
    OpTax(cpu);
    uint16_t target = Read16ProgramIndexed(memory, cpu, 0xd8a3u, cpu->x);
    Lufia2ExecutionResult result;
    if (!ProbeDispatcherChild(memory, cpu, child, context,
            0x83d89fu, 0x830000u | target, 2u, &result))
        return result;
    return ExecutionReturned(0x83d8a2u);
}
