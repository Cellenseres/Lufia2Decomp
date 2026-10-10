#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_TARGET_INDEX = 0x11u,
    EFFECT_TARGET_INDEX = 0x25u,
    SPECIAL_TARGET_INDEX = 4u
};

static uint8_t TargetToggleCallerContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f06u && cpu->stack <= 0x1ffcu;
}

static Lufia2ExecutionResult ToggleTargetState(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target) {
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, 3u, 0x81u)) {
        const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return result;
    }
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    return ExecutionReturned(site + 6u);
}

Lufia2ExecutionResult Lufia2BattleEffectTogglePartyTargetState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!TargetToggleCallerContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x819bacu);
    OpTyx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7e0025u));
    if (!cpu->negative)
        return ExecutionReturned(0x819bbdu);
    OpSta(memory, cpu, OpDp(cpu, DP_TARGET_INDEX));
    return ToggleTargetState(memory, cpu, child, context,
        0x819bb7u, 0x85920eu);
}

Lufia2ExecutionResult Lufia2BattleEffectToggleSpecialTargetState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!TargetToggleCallerContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x819d0bu);
    OpTyx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7e0025u));
    if (cpu->negative)
        return ExecutionReturned(0x819d1eu);
    OpCmpValue(cpu, SPECIAL_TARGET_INDEX);
    if (!cpu->zero)
        return ExecutionReturned(0x819d1eu);
    return ToggleTargetState(memory, cpu, child, context,
        0x819d18u, 0x85922du);
}
