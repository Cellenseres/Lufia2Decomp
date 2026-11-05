#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/legacy_fragments.h"
#include "system/wram.h"

enum {
    LEGACY_SCRIPT_POSITION = 0x2au,
    LEGACY_ACTOR_STEP_FLAGS = 0x0au,
    LEGACY_SCRIPT_BRANCH_OFFSET = 0x8000u
};

static bool LegacyCollisionContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2ActorContinueLegacyCollisionBranch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !LegacyCollisionContext(cpu) ||
        cpu->stack < 0x1f04u || cpu->stack > 0x1ff8u)
        return ExecutionHandoff(cpu, 0x83d858u);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, LEGACY_ACTOR_STEP_FLAGS);
    if (cpu->zero) {
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x83d85fu, 0x83d87du, 2u, 0x83u))
            return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND,
                0x83d85fu, 0u};
        if (!LegacyCollisionContext(cpu))
            return ExecutionHandoff(cpu,
                ((uint32_t)cpu->program_bank << 16) | 0xd862u);
        if (!cpu->zero) {
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, LEGACY_SCRIPT_POSITION)));
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbsX(cpu, 1u));
            cpu->carry = 0u;
            OpAdcValue(cpu, LEGACY_SCRIPT_BRANCH_OFFSET);
            OpTax(cpu);
            OpSepWidths(cpu, 0x20u);
            return ExecutionHandoff(cpu, 0x83d5c3u);
        }
    }
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, LEGACY_SCRIPT_POSITION)));
    OpLdx(cpu, (uint16_t)(cpu->x + 3u));
    return ExecutionHandoff(cpu, 0x83d5c3u);
}
