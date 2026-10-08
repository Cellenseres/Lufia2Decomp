#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_PORTRAIT_SLOT = 0x11u,
    DP_PORTRAIT_POSE = 0x12u,
    DP_EFFECT_STREAM = 0xc3u,
    EFFECT_TARGET = 0x25u,
    SPECIAL_TARGET = 4u,
    PORTRAIT_TARGET_MASKS = 0x96ffecu,
    BATTLER_STATUS = 0x0fu,
    SUPPRESSED_PORTRAIT_STATUSES = 0x2du
};

static Lufia2ExecutionResult PortraitStatusUnwound(uint32_t site) {
    const Lufia2ExecutionResult result = {
        LUFIA2_EXECUTION_CHILD_UNWOUND, site
    };
    return result;
}

Lufia2ExecutionResult Lufia2BattleEffectSetPortraitPoseIfStatusClear(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819cccu);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpDp(cpu, DP_PORTRAIT_POSE));
    OpRepWidths(cpu, 0x20u);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpTyx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7e0000u + EFFECT_TARGET));
    if (cpu->negative)
        return ExecutionReturned(0x819d0au);
    OpCmpValue(cpu, SPECIAL_TARGET);
    if (cpu->zero)
        return ExecutionReturned(0x819d0au);
    OpSta(memory, cpu, OpDp(cpu, DP_PORTRAIT_SLOT));
    PushY(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x7fu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, PORTRAIT_TARGET_MASKS));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819cf0u, 0x81b2b5u, 3u, 0x81u))
        return PortraitStatusUnwound(0x819cf0u);
    OpLda(memory, cpu, OpAbsX(cpu, BATTLER_STATUS));
    OpBitValue(cpu, SUPPRESSED_PORTRAIT_STATUSES);
    if (cpu->zero) {
        PushDataBank(memory, cpu);
        OpSetDataBank(memory, cpu, 0x97u);
        if (!CallChildWithFrame(memory, cpu, child, context,
                0x819d00u, 0x81bb75u, 2u, 0x81u))
            return PortraitStatusUnwound(0x819d00u);
        PullDataBank(memory, cpu);
        LoadA8(cpu, 0xffu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_STATUS_REQUEST));
    }
    OpPullY(memory, cpu);
    return ExecutionReturned(0x819d0au);
}
