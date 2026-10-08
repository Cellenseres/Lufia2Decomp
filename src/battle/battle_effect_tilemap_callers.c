#include "core/child_call.h"
#include "lufia2/battle.h"

static Lufia2ExecutionResult CallTilemapCommand(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, site);
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, 2u, 0x81u)) {
        const Lufia2ExecutionResult result = {
            LUFIA2_EXECUTION_CHILD_UNWOUND, site
        };
        return result;
    }
    return ExecutionReturned(site + 3u);
}

Lufia2ExecutionResult Lufia2BattleEffectClearWindowTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return CallTilemapCommand(memory, cpu, child, context,
        0x8196f6u, 0x81c2fbu);
}

Lufia2ExecutionResult Lufia2BattleEffectResetPartyTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    return CallTilemapCommand(memory, cpu, child, context,
        0x8196fau, 0x81c2e3u);
}
