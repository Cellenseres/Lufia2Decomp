#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum { DP_EFFECT_STREAM = 0xc3u };

Lufia2ExecutionResult Lufia2BattleEffectSendSoundCommand(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819e13u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    PushY(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819e16u, 0x80953bu, 3u, 0x81u)) {
        const Lufia2ExecutionResult result = {
            LUFIA2_EXECUTION_CHILD_UNWOUND, 0x819e16u
        };
        return result;
    }
    OpPullY(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x819e21u);
}
