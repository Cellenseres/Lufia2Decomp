#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_EFFECT_STREAM = 0xc3u,
    FRAME_MODE = 0x15abu,
    SPRITE_REBUILD_REQUEST = 0x15aeu
};

static Lufia2ExecutionResult FrameChildUnwound(uint32_t site) {
    const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
    return result;
}

Lufia2ExecutionResult Lufia2BattleEffectPrepareFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819b7eu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    PushY(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, FRAME_MODE));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819b8au, 0x858a39u, 3u, 0x81u))
        return FrameChildUnwound(0x819b8au);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, SPRITE_REBUILD_REQUEST));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, WRAM_BATTLE_EFFECT_FRAME_MARKER);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819b99u, 0x81b9afu, 3u, 0x81u))
        return FrameChildUnwound(0x819b99u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819b9du, 0x85ec81u, 3u, 0x81u))
        return FrameChildUnwound(0x819b9du);
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819ba2u);
}
