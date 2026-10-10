#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

Lufia2ExecutionResult Lufia2BattleEffectRefreshTurnDisplay(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819968u);
    PushY(memory, cpu);
    const uint32_t sites[] = {0x819969u, 0x81996cu, 0x819970u};
    const uint32_t targets[] = {0x81dee9u, 0x859dd4u, 0x8591a1u};
    for (unsigned call = 0u; call < 3u; ++call) {
        const uint8_t frame = call == 0u ? 2u : 3u;
        if (!CallChildWithFrame(memory, cpu, child, context,
                sites[call], targets[call], frame, 0x81u)) {
            const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND,
                                                  sites[call], 0u};
            return result;
        }
    }
    OpPullY(memory, cpu);
    return ExecutionReturned(0x819975u);
}
