#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

enum {
    DP_STEP_CODE = 0x22,
    LEADER_FACING_ACTIONS = 0x83c1a5,
    REVERSE_FACING_ACTIONS = 0x83c1ac,
    ITEM_ANIMATION_TYPES = 0xc63f,
    ITEM_ANIMATION_REQUESTS = 0xc646,
    ITEM_ANIMATION_DELAYS = 0xc64c
};

static uint8_t EffectCallerContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal;
}

static uint8_t EffectCallerChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site,
    uint32_t target, uint8_t frame, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context,
            site, target, frame, 0x83u)) {
        *result = (Lufia2ExecutionResult){
            LUFIA2_EXECUTION_CHILD_UNWOUND, site, 0u};
        return 0u;
    }
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        *result = ExecutionHandoff(cpu, site + frame + 1u);
        return 0u;
    }
    return 1u;
}

Lufia2ExecutionResult Lufia2FieldChangeLeaderFacing(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectCallerContext(cpu) || cpu->direct_page || !child)
        return ExecutionHandoff(cpu, 0x83be64u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpTax(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_STEP_CODE));
    OpCmp(memory, cpu, OpLongX(cpu, LEADER_FACING_ACTIONS));
    if (cpu->zero)
        return ExecutionReturned(0x83be8fu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, REVERSE_FACING_ACTIONS));
    OpCmp(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    uint8_t transition = cpu->zero ? 0x28u : 0x24u;
    OpLda(memory, cpu, OpDp(cpu, DP_STEP_CODE));
    cpu->carry = 0u;
    OpAdcValue(cpu, transition);
    Lufia2ExecutionResult result;
    if (!EffectCallerChild(memory, cpu, child, context,
            0x83be86u, 0x83d350u, 3u, &result))
        return result;
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_FACING_CHANGE_MARKER));
    return ExecutionReturned(0x83be8fu);
}

Lufia2ExecutionResult Lufia2FieldStepLeaderFacing(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectCallerContext(cpu) || cpu->direct_page || !child)
        return ExecutionHandoff(cpu, 0x83bf33u);
    Lufia2ExecutionResult result;
    if (!EffectCallerChild(memory, cpu, child, context,
            0x83bf33u, 0x83c0efu, 2u, &result))
        return result;
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    if (!EffectCallerChild(memory, cpu, child, context,
            0x83bf39u, 0x83fb12u, 3u, &result))
        return result;
    return ExecutionReturned(0x83bf3du);
}

Lufia2ExecutionResult Lufia2FieldSelectItemAnimation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectCallerContext(cpu) || !child)
        return ExecutionHandoff(cpu, 0x83c5feu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, WRAM_MENU_LIST_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_ITEM_RECORD_ID));
    OpIny(cpu);
    OpIny(cpu);
    OpSepWidths(cpu, 0x20u);
    Lufia2ExecutionResult result;
    if (!EffectCallerChild(memory, cpu, child, context,
            0x83c60au, 0x83c692u, 2u, &result))
        return result;
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    PushY(memory, cpu);
    OpLdy(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpAbsY(cpu, ITEM_ANIMATION_TYPES));
        if (cpu->negative)
            break;
        OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        if (cpu->zero) {
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
            OpLda(memory, cpu, OpAbsY(cpu, ITEM_ANIMATION_REQUESTS));
            OpSta(memory, cpu, OpLongX(cpu, WRAM_OBJECT_ANIMATION_REQUEST));
            OpLda(memory, cpu, OpAbsY(cpu, ITEM_ANIMATION_DELAYS));
            OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FDA2C));
            break;
        }
        OpIny(cpu);
    }
    OpPullY(memory, cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    return ExecutionReturned(0x83c632u);
}
