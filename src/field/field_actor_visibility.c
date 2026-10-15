#include "actor/actor_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "field/actor_visibility_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"
#include "text/text_internal.h"

static bool VisibilityContext(const Lufia2CpuState *cpu, uint8_t bank) {
    return cpu->program_bank == bank && cpu->accumulator_is_8_bit &&
           !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
           cpu->stack >= 0x1f10u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2FieldFindWindowActor(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    if (!VisibilityContext(cpu, 0x80u))
        return ExecutionHandoff(cpu, 0x80bf6fu);
    Lufia2TextFindWindowActor(memory, cpu);
    return ExecutionReturned(cpu->carry ? 0x80bf89u : 0x80bf91u);
}

bool Lufia2FieldRevealActorState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, 0xfbu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpBitValue(cpu, 0x20u);
    if (cpu->zero)
        return false;
    OpAndValue(cpu, 0xdfu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_ID));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x50u);
    return true;
}

void Lufia2FieldHideActorState(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpOraValue(cpu, 4u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpOraValue(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    SimulateJslFrame(memory, cpu, 0x8eu, 0xbb2cu);
    cpu->program_bank = 0x83u;
    Lufia2ActorClearMapOccupancy(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x8eu;
}

Lufia2ExecutionResult Lufia2FieldRevealActor(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu,
                                             Lufia2PushedChildCall child,
                                             void *context) {
    if (!child || !VisibilityContext(cpu, 0x8eu))
        return ExecutionHandoff(cpu, 0x8ebaf6u);
    if (Lufia2FieldRevealActorState(memory, cpu) &&
        !CallChildWithFrame(memory, cpu, child, context, 0x8ebb12u, 0x818351u, 3u,
                            0x8eu))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x8ebb12u, 0u};
    return ExecutionReturned(0x8ebb16u);
}

Lufia2ExecutionResult Lufia2FieldHideActor(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    if (!VisibilityContext(cpu, 0x8eu))
        return ExecutionHandoff(cpu, 0x8ebb17u);
    Lufia2FieldHideActorState(memory, cpu);
    return ExecutionReturned(0x8ebb2du);
}
