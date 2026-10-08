#include "core/cpu_ops.h"
#include "lufia2/battle.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_POPUP_HORIZONTAL = 4u,
    DP_POPUP_VERTICAL = 5u,
    DP_POPUP_SCRIPT = 0x12u,
    DP_POPUP_PHASE = 0x13u,
    DP_POPUP_ATTRIBUTE = 0x14u,
    POPUP_SCRIPT_TABLE = 0x958200u,
    ACTOR_SCRIPT = 3u,
    ACTOR_HORIZONTAL_VELOCITY = 0x1bu,
    ACTOR_VERTICAL_VELOCITY = 0x1du,
    ACTOR_HORIZONTAL_POSITION = 0x1fu,
    ACTOR_VERTICAL_POSITION = 0x21u,
    ACTOR_ATTRIBUTES = 0x23u,
    ACTOR_ORDER = 0x25u,
    ACTOR_POPUP_ATTRIBUTE = 0x28u
};

static void InitializePopupPosition(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpTxy(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_POPUP_SCRIPT));
    OpAndValue(cpu, 0xffu);
    OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, POPUP_SCRIPT_TABLE));
    OpSta(memory, cpu, OpAbsY(cpu, ACTOR_SCRIPT));
    OpLda(memory, cpu, OpDp(cpu, DP_POPUP_HORIZONTAL));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsY(cpu, ACTOR_HORIZONTAL_POSITION));
    OpLda(memory, cpu, OpDp(cpu, DP_POPUP_VERTICAL));
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsY(cpu, ACTOR_VERTICAL_POSITION));
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpAbsY(cpu, ACTOR_HORIZONTAL_VELOCITY));
    OpSta(memory, cpu, OpAbsY(cpu, ACTOR_VERTICAL_VELOCITY));
    OpLoadA(cpu, 0x8080u);
    OpSta(memory, cpu, OpAbsY(cpu, ACTOR_ATTRIBUTES));
}

static void InitializePopupState(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsY(cpu, ACTOR_ORDER));
    OpLda(memory, cpu, OpDp(cpu, DP_POPUP_ATTRIBUTE));
    OpSta(memory, cpu, OpAbsY(cpu, ACTOR_POPUP_ATTRIBUTE));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsY(cpu, 0u));
    OpLda(memory, cpu, OpDp(cpu, DP_POPUP_PHASE));
    OpSta(memory, cpu, OpAbsY(cpu, 2u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbsY(cpu, 1u));
    OpStepMem(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_ACTIVE_COUNT), 1);
}

Lufia2ExecutionResult Lufia2BattleEffectSpawnPopupActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f04u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x819f39u);
    OpPushX(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x9f3cu);
    const Lufia2ExecutionResult result =
        Lufia2BattleEffectFindActorSlot(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    InitializePopupPosition(memory, cpu);
    InitializePopupState(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819f8au);
}
