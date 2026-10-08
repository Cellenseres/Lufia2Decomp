#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_EFFECT_STREAM = 0xc3u,
    DP_TARGET_HORIZONTAL = 0u,
    DP_TARGET_VERTICAL = 1u,
    SAVED_ACTION_ACTOR = 0x7ff4d6u,
    ACTOR_INDEX_TABLE = 0x97fe26u,
    SCRIPT_ADDRESS_ARGUMENT = 0x15edu,
    SCRIPT_FIRST_PARAMETER = 0x13u,
    SCRIPT_LAST_PARAMETER = 0x19u,
    SCRIPT_ADDRESS = 3u,
    SCRIPT_HORIZONTAL_SPEED = 0x1bu,
    SCRIPT_VERTICAL_SPEED = 0x1du,
    SCRIPT_HORIZONTAL_POSITION = 0x1fu,
    SCRIPT_VERTICAL_POSITION = 0x21u,
    SCRIPT_HORIZONTAL_ADJUSTMENT = 0x23u,
    SCRIPT_VERTICAL_ADJUSTMENT = 0x24u,
    SCRIPT_TARGET = 0x25u,
    TARGET_SPRITE_FIELD = 0x50u
};

static bool EffectEntryFits(const Lufia2CpuState *cpu,
    Lufia2PushedChildCall child) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && !cpu->direct_page &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu && child;
}

static Lufia2ExecutionResult InterruptedEffectCall(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void SelectSavedActor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, SAVED_ACTION_ACTOR);
    const bool enemy = cpu->negative != 0u;
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x7fu);
    OpPushX(memory, cpu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, ACTOR_INDEX_TABLE));
    OpPullX(memory, cpu);
    if (enemy)
        OpLoadA(cpu, (uint16_t)(OpA(cpu) | 0x80u));
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_TARGET));
}

static void InitializeTargetScript(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    for (uint16_t field = SCRIPT_FIRST_PARAMETER;
         field <= SCRIPT_LAST_PARAMETER; field += 2u) {
        OpLda(memory, cpu, OpAbsY(cpu, field));
        OpSta(memory, cpu, OpAbsX(cpu, field));
    }
    OpLda(memory, cpu, OpAbs(cpu, SCRIPT_ADDRESS_ARGUMENT));
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_ADDRESS));
    OpLda(memory, cpu, OpDp(cpu, DP_TARGET_HORIZONTAL));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_HORIZONTAL_POSITION));
    OpLda(memory, cpu, OpDp(cpu, DP_TARGET_VERTICAL));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_VERTICAL_POSITION));
    OpStz(memory, cpu, OpAbsX(cpu, SCRIPT_HORIZONTAL_SPEED));
    OpStz(memory, cpu, OpAbsX(cpu, SCRIPT_VERTICAL_SPEED));
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnSavedActorScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectEntryFits(cpu, child))
        return ExecutionHandoff(cpu, 0x8192e9u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpAbs(cpu, SCRIPT_ADDRESS_ARGUMENT));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    PushY(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8192fbu, 0x818fabu, 2u, 0x81u))
        return InterruptedEffectCall(0x8192fbu);
    cpu->y = PullIndexValue(memory, cpu);
    SelectSavedActor(memory, cpu);
    PushY(memory, cpu);
    OpPushX(memory, cpu);
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0x80u));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81932fu, 0x81b80au, 2u, 0x81u))
        return InterruptedEffectCall(0x81932fu);
    OpPullX(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    InitializeTargetScript(memory, cpu);
    PushY(memory, cpu);
    OpPushX(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, SCRIPT_TARGET));
    OpLoadA(cpu, (uint16_t)(OpA(cpu) ^ 0x80u));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x819373u, 0x81b7d9u, 2u, 0x81u))
        return InterruptedEffectCall(0x819373u);
    OpPullX(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_TARGET_HORIZONTAL));
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_HORIZONTAL_ADJUSTMENT));
    OpLda(memory, cpu, OpDp(cpu, DP_TARGET_VERTICAL));
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_VERTICAL_ADJUSTMENT));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x0101u);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_ACTIVE_COUNT), 1);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x819392u);
}

Lufia2ExecutionResult Lufia2BattleEffectTargetAdjustment(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!EffectEntryFits(cpu, child))
        return ExecutionHandoff(cpu, 0x81b7d9u);
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero) {
        OpCmpValue(cpu, 0x84u);
        const bool special = cpu->zero != 0u;
        OpLoadA(cpu, special ? 6u : 4u);
        OpSta(memory, cpu, OpDp(cpu, DP_TARGET_HORIZONTAL));
        OpSta(memory, cpu, OpDp(cpu, DP_TARGET_VERTICAL));
        return ExecutionReturned(special ? 0x81b7eeu : 0x81b7e7u);
    }
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_ENEMY_RECORDS));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_SPRITE_FIELD));
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81b7fdu, 0x81fbc6u, 3u, 0x81u))
        return InterruptedEffectCall(0x81b7fdu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x81b801u, 0x81fb8eu, 3u, 0x81u))
        return InterruptedEffectCall(0x81b801u);
    OpSta(memory, cpu, OpDp(cpu, DP_TARGET_HORIZONTAL));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x81b809u);
}
