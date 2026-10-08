#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    TARGET_STATUS_OFFSET = 1u,
    TARGET_RECORD_SIZE = 15u,
    TARGET_STATE_BIT = 0x20u
};

static uint8_t TargetToggleContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x85u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2BattleTogglePartyTargetState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TargetToggleContext(cpu))
        return ExecutionHandoff(cpu, 0x85920eu);
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, SNES_WRMPYA);
    LoadA8(cpu, TARGET_RECORD_SIZE);
    OpSta(memory, cpu, SNES_WRMPYB);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, SNES_RDMPYL);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    const uint32_t status = OpAbsX(cpu,
        (uint16_t)(WRAM_BATTLE_EFFECT_PARTY_POSITIONS + TARGET_STATUS_OFFSET));
    OpLda(memory, cpu, status);
    OpLoadA(cpu, (uint16_t)(cpu->accumulator ^ TARGET_STATE_BIT));
    OpSta(memory, cpu, status);
    return ExecutionReturned(0x85922cu);
}

Lufia2ExecutionResult Lufia2BattleToggleSpecialTargetState(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TargetToggleContext(cpu))
        return ExecutionHandoff(cpu, 0x85922du);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_STATUS));
    OpLoadA(cpu, (uint16_t)(cpu->accumulator ^ TARGET_STATE_BIT));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_STATUS));
    return ExecutionReturned(0x859235u);
}
