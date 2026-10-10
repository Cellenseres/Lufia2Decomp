#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    DP_GRAPHICS_RESOURCE = 0x54u,
    DP_GRAPHICS_DESTINATION = 0x60u,
    DP_GRAPHICS_DESTINATION_BANK = 0x62u,
    DP_EFFECT_STREAM = 0xc3u,
    GRAPHICS_RESOURCE_TABLE = 0x85ef53u,
    GRAPHICS_BANK_SELECTOR = 0x007eu
};

static void ReadGraphicsResource(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    AslA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, GRAPHICS_RESOURCE_TABLE));
    OpSta(memory, cpu, OpDp(cpu, DP_GRAPHICS_RESOURCE));
}

static void ReadGraphicsDestination(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSta(memory, cpu, OpDp(cpu, DP_GRAPHICS_DESTINATION));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, GRAPHICS_BANK_SELECTOR));
    OpSta(memory, cpu, OpDp(cpu, DP_GRAPHICS_DESTINATION_BANK));
}

Lufia2ExecutionResult Lufia2BattleEffectLoadGraphicsResource(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819664u);
    ReadGraphicsResource(memory, cpu);
    ReadGraphicsDestination(memory, cpu);
    PushY(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819684u, 0x808e9du, 3u, 0x81u)) {
        const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND,
                                              0x819684u, 0u};
        return result;
    }
    OpPullY(memory, cpu);
    return ExecutionReturned(0x819689u);
}
