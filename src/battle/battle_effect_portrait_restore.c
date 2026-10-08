#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_PORTRAIT_SLOT = 0x11u,
    DP_PORTRAIT_POSE = 0x12u,
    EFFECT_TARGET = 0x25u,
    SPECIAL_TARGET = 4u,
    CURRENT_PORTRAIT_POSES = WRAM_BATTLE_PORTRAIT_POSES
};

Lufia2ExecutionResult Lufia2BattleEffectRestorePortraitPose(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819dc9u);
    OpTyx(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7e0000u + EFFECT_TARGET));
    if (cpu->negative)
        return ExecutionReturned(0x819df5u);
    OpCmpValue(cpu, SPECIAL_TARGET);
    if (cpu->zero)
        return ExecutionReturned(0x819df5u);
    OpSta(memory, cpu, OpDp(cpu, DP_PORTRAIT_SLOT));
    PushY(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x7fu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, CURRENT_PORTRAIT_POSES));
    OpSta(memory, cpu, OpDp(cpu, DP_PORTRAIT_POSE));
    OpSta(memory, cpu, OpDp(cpu, DP_PORTRAIT_POSE));
    PushDataBank(memory, cpu);
    OpSetDataBank(memory, cpu, 0x97u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819debu, 0x81bb75u, 2u, 0x81u)) {
        const Lufia2ExecutionResult result = {
            LUFIA2_EXECUTION_CHILD_UNWOUND, 0x819debu
        };
        return result;
    }
    PullDataBank(memory, cpu);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_STATUS_REQUEST));
    OpPullY(memory, cpu);
    return ExecutionReturned(0x819df5u);
}
