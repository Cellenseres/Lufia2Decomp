#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

static Lufia2ExecutionResult GlyphUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

Lufia2ExecutionResult Lufia2BattleEmitGlyphTile(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->decimal || cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x81e8e1u);
    PushIndex(memory, cpu);
    OpLdx(cpu, 0x0a00u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81e8e5u, 0x81e835u, 2u, 0x81u))
        return GlyphUnwound(0x81e8e5u);
    ExchangeAccumulatorBytes(cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81e8e9u, 0x81e8eeu, 2u, 0x81u))
        return GlyphUnwound(0x81e8e9u);
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x81e8edu);
}
