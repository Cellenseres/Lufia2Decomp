#include "lufia2/field.h"
#include "lufia2/actor.h"
#include "actor/actor_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum {
    DP_ACTION_SPAWN_ID = 0x94,
    LEADER_FACING_ACTIONS = 0x83c1a5
};

static uint8_t ActionSetupWidths(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

static uint8_t ActionSetupContext(const Lufia2CpuState *cpu) {
    return ActionSetupWidths(cpu) && cpu->program_bank == 0x83u &&
        !cpu->direct_page && !cpu->decimal;
}

static uint8_t ActionSetupCall(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x83u)) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!ActionSetupWidths(cpu)) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

Lufia2ExecutionResult Lufia2FieldSpawnActionActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ActionSetupContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83c2feu);
    OpSta(memory, cpu, OpDp(cpu, DP_ACTION_SPAWN_ID));
    ExchangeAccumulatorBytes(cpu);
    OpOraValue(cpu, OpReadM(memory, cpu, WRAM_FIELD_CONTROL_FLAGS));
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpLda(memory, cpu, OpDp(cpu, DP_ACTION_SPAWN_ID));
    Lufia2ExecutionResult result;
    if (!ActionSetupCall(memory, cpu, child, context,
            0x83c30bu, 0x83df87u, 3u, &result))
        return result;
    cpu->carry = 1u;
    return ExecutionReturned(0x83c310u);
}

Lufia2ExecutionResult Lufia2FieldSpawnFacingActionActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ActionSetupContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83c311u);
    PushAccumulator8(memory, cpu);
    Lufia2ExecutionResult result;
    if (!ActionSetupCall(memory, cpu, child, context,
            0x83c312u, 0x83c32du, 2u, &result))
        return result;
    if (cpu->decimal)
        return ExecutionHandoff(cpu, 0x83c315u);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x20u);
    if (!ActionSetupCall(memory, cpu, child, context,
            0x83c318u, 0x83d3a1u, 3u, &result))
        return result;
    OpLoadA(cpu, Pull8(memory, cpu));
    if (!ActionSetupCall(memory, cpu, child, context,
            0x83c31du, 0x83df87u, 3u, &result))
        return result;
    OpLda(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    OpOraValue(cpu, 8u);
    OpSta(memory, cpu, WRAM_FIELD_CONTROL_FLAGS);
    cpu->carry = 1u;
    return ExecutionReturned(0x83c32cu);
}

Lufia2ExecutionResult Lufia2FieldReadLeaderFacingAction(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ActionSetupContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83c32du);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    Lufia2ExecutionResult result;
    if (!ActionSetupCall(memory, cpu, child, context,
            0x83c330u, 0x83ab4fu, 3u, &result))
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, LEADER_FACING_ACTIONS));
    return ExecutionReturned(0x83c33cu);
}

Lufia2ExecutionResult Lufia2ActorStartSecondaryActionScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!ActionSetupContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83d3a1u);
    Lufia2ExecutionResult result;
    if (!ActionSetupCall(memory, cpu, child, context,
            0x83d3a1u, 0x83d3f7u, 2u, &result))
        return result;
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLoadA(cpu, 0x80u);
    OpOraValue(cpu, OpReadM(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE)));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    return ExecutionReturned(0x83d3aeu);
}

Lufia2ExecutionResult Lufia2ActorPrepareSecondaryScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ActionSetupWidths(cpu) || cpu->program_bank != 0x83u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83d3f7u);
    Lufia2ActorInstallSecondaryScript(memory, cpu);
    return ExecutionReturned(0x83d415u);
}
