#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_TARGETS_LEFT = 2u,
    DP_POPUP_HORIZONTAL = 4u,
    DP_POPUP_VERTICAL = 5u,
    DP_DIGIT_INDEX = 0x11u,
    DP_POPUP_SCRIPT = 0x12u,
    DP_POPUP_PHASE = 0x13u,
    DP_POPUP_ATTRIBUTE = 0x14u,
    TARGET_HORIZONTAL = 0u,
    TARGET_VERTICAL = 1u,
    TARGET_HEIGHT = 3u,
    TARGET_RESULT = 8u,
    TARGET_FLAGS = 9u,
    TARGET_STRIDE = 11u,
    TARGET_COUNT = WRAM_BATTLE_EFFECT_TARGET_COUNT & 0xffffu,
    NUMERIC_SCRIPT_BASE = 10u
};

static uint8_t SpawnPopup(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context, uint32_t site) {
    return CallChildWithFrame(memory, cpu, child, context,
        site, 0x819f39u, 2u, 0x81u);
}

static Lufia2ExecutionResult PopupUnwind(uint32_t site) {
    const Lufia2ExecutionResult result = {LUFIA2_EXECUTION_CHILD_UNWOUND, site};
    return result;
}

static void ReadTargetPosition(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_FLAGS));
    OpAndValue(cpu, 1u);
    OpAslA(cpu);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_POPUP_ATTRIBUTE));
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_HORIZONTAL));
    OpSta(memory, cpu, OpDp(cpu, DP_POPUP_HORIZONTAL));
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_HEIGHT));
    OpAslA(cpu);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpAbsX(cpu, TARGET_VERTICAL));
    OpSta(memory, cpu, OpDp(cpu, DP_POPUP_VERTICAL));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, DP_POPUP_PHASE));
}

static void OffsetPopup(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t up, uint8_t right) {
    OpLda(memory, cpu, OpDp(cpu, DP_POPUP_VERTICAL));
    cpu->carry = 1u;
    OpSbcValue(cpu, up);
    OpSta(memory, cpu, OpDp(cpu, DP_POPUP_VERTICAL));
    OpLda(memory, cpu, OpDp(cpu, DP_POPUP_HORIZONTAL));
    cpu->carry = 0u;
    OpAdcValue(cpu, right);
    OpSta(memory, cpu, OpDp(cpu, DP_POPUP_HORIZONTAL));
}

static Lufia2ExecutionResult SpawnDigits(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OffsetPopup(memory, cpu, 4u, 8u);
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, DP_DIGIT_INDEX));
    OpPushX(memory, cpu);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, TARGET_RESULT));
        if (cpu->negative)
            break;
        OpDex(cpu);
        for (unsigned step = 0u; step < 4u; ++step)
            OpStepMem(memory, cpu, OpDp(cpu, DP_POPUP_HORIZONTAL), -1);
        OpLda(memory, cpu, OpDp(cpu, DP_DIGIT_INDEX));
        OpDecA(cpu);
        OpSta(memory, cpu, OpDp(cpu, DP_DIGIT_INDEX));
        OpCmpValue(cpu, 1u);
        if (cpu->zero)
            break;
    }
    OpInx(cpu);
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, TARGET_RESULT));
        cpu->carry = 0u;
        OpAdcValue(cpu, NUMERIC_SCRIPT_BASE);
        OpSta(memory, cpu, OpDp(cpu, DP_POPUP_SCRIPT));
        if (!SpawnPopup(memory, cpu, child, context, 0x819e9du))
            return PopupUnwind(0x819e9du);
        OpLda(memory, cpu, OpDp(cpu, DP_POPUP_HORIZONTAL));
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, DP_POPUP_HORIZONTAL));
        OpLda(memory, cpu, OpDp(cpu, DP_POPUP_PHASE));
        cpu->carry = 0u;
        OpAdcValue(cpu, 2u);
        OpSta(memory, cpu, OpDp(cpu, DP_POPUP_PHASE));
        OpLda(memory, cpu, OpDp(cpu, DP_DIGIT_INDEX));
        OpCmpValue(cpu, 4u);
        if (cpu->zero)
            break;
        OpStepMem(memory, cpu, OpDp(cpu, DP_DIGIT_INDEX), 1);
        OpInx(cpu);
    }
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819f26u);
}

static Lufia2ExecutionResult SpawnResult(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context,
    uint8_t script, uint32_t first_site, uint32_t second_site) {
    OffsetPopup(memory, cpu, 8u, 12u);
    OpPushX(memory, cpu);
    OpInx(cpu);
    OpLoadA(cpu, script);
    OpSta(memory, cpu, OpDp(cpu, DP_POPUP_SCRIPT));
    if (!SpawnPopup(memory, cpu, child, context, first_site))
        return PopupUnwind(first_site);
    if (second_site) {
        OpLda(memory, cpu, OpDp(cpu, DP_POPUP_HORIZONTAL));
        cpu->carry = 1u;
        OpSbcValue(cpu, 16u);
        OpSta(memory, cpu, OpDp(cpu, DP_POPUP_HORIZONTAL));
        OpInx(cpu);
        OpLoadA(cpu, (uint8_t)(script + 1u));
        OpSta(memory, cpu, OpDp(cpu, DP_POPUP_SCRIPT));
        if (!SpawnPopup(memory, cpu, child, context, second_site))
            return PopupUnwind(second_site);
    }
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819f26u);
}

static Lufia2ExecutionResult SpawnTargetResult(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_RESULT));
    OpCmpValue(cpu, 10u);
    if (cpu->zero)
        return SpawnResult(memory, cpu, child, context, 20u,
            0x819eeau, 0x819ef9u);
    OpCmpValue(cpu, 11u);
    if (cpu->zero)
        return SpawnResult(memory, cpu, child, context, 22u,
            0x819ed0u, 0u);
    OpCmpValue(cpu, 12u);
    if (cpu->zero)
        return SpawnResult(memory, cpu, child, context, 23u,
            0x819f13u, 0x819f22u);
    return SpawnDigits(memory, cpu, child, context);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnTargetPopups(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f0au || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x819e22u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, TARGET_COUNT));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, DP_TARGETS_LEFT));
    PushY(memory, cpu);
    OpLdx(cpu, TARGET_COUNT + 1u);
    do {
        ReadTargetPosition(memory, cpu);
        const Lufia2ExecutionResult result =
            SpawnTargetResult(memory, cpu, child, context);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, TARGET_STRIDE);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, DP_TARGETS_LEFT), -1);
    } while (!cpu->zero);
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819f38u);
}
