#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_EFFECT_STREAM = 0xc3u,
    DP_TARGET_HORIZONTAL = 0u,
    DP_TARGET_VERTICAL = 1u,
    SCRIPT_ADDRESS_ARGUMENT = 0x15edu,
    SCRIPT_ADDRESS = 3u,
    SCRIPT_FIRST_PARAMETER = 0x13u,
    SCRIPT_LAST_PARAMETER = 0x19u,
    SCRIPT_HORIZONTAL_SPEED = 0x1bu,
    SCRIPT_VERTICAL_SPEED = 0x1du,
    SCRIPT_HORIZONTAL_POSITION = 0x1fu,
    SCRIPT_VERTICAL_POSITION = 0x21u,
    SCRIPT_ADJUSTMENT = 0x23u,
    SCRIPT_TARGET = 0x25u,
    ROM_TARGET_INDEX_TABLE = 0x96ffecu,
    EFFECT_TARGET_REFERENCE = 0x7ff44eu
};

static Lufia2ExecutionResult InterruptedTargetCall(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static void InitializeCurrentTargetScript(const Lufia2Memory *memory,
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
    OpLoadA(cpu, 0x0404u);
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_ADJUSTMENT));
    OpLoadA(cpu, 0x0101u);
    OpSta(memory, cpu, OpAbsX(cpu, 0u));
    OpSta(memory, cpu, OpAbsX(cpu, 1u));
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_ACTIVE_COUNT), 1);
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnCurrentTargetScript(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819393u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_EFFECT_STREAM));
    OpSta(memory, cpu, OpAbs(cpu, SCRIPT_ADDRESS_ARGUMENT));
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpStepMem(memory, cpu, OpDp(cpu, DP_EFFECT_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsY(cpu, SCRIPT_TARGET));
    PushY(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8193a8u, 0x81b80au, 2u, 0x81u))
        return InterruptedTargetCall(0x8193a8u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x8193abu, 0x818fabu, 2u, 0x81u))
        return InterruptedTargetCall(0x8193abu);
    cpu->y = PullIndexValue(memory, cpu);
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpAbsY(cpu, SCRIPT_TARGET));
    OpSta(memory, cpu, OpAbsX(cpu, SCRIPT_TARGET));
    OpPushX(memory, cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, ROM_TARGET_INDEX_TABLE));
    OpPullX(memory, cpu);
    OpCmp(memory, cpu, EFFECT_TARGET_REFERENCE);
    if (!cpu->zero)
        InitializeCurrentTargetScript(memory, cpu);
    return ExecutionReturned(0x81940du);
}
