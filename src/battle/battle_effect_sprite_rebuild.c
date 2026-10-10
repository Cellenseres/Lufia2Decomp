#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum { DP_EFFECT_STREAM = 0xc3u };

Lufia2ExecutionResult Lufia2BattleEffectRebuildSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819df6u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    PushY(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_SPRITE_BUILD_MODE));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819e02u, 0x858a2fu, 3u, 0x81u)) {
        const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, 0x819e02u,
                                              0u};
        return result;
    }
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_BG1_REQUEST));
    LoadA8(cpu, 0xffu);
    Write8(memory, WRAM_BATTLE_EFFECT_FRAME_MARKER, A8(cpu));
    OpPullY(memory, cpu);
    return ExecutionReturned(0x819e12u);
}
