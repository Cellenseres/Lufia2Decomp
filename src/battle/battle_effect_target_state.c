#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    DP_TARGET_STREAM = 0xc3u,
    DP_TARGET_NUMBER = 0x11u,
    EFFECT_TARGET_NUMBER = 0x25u,
    TARGET_HORIZONTAL_OFFSET = 0x15efu,
    TARGET_VERTICAL_OFFSET = WRAM_BATTLE_EFFECT_VERTICAL_OFFSET,
    TARGET_HORIZONTAL_MOTION = WRAM_BATTLE_EFFECT_TARGET_HORIZONTAL_MOTION,
    TARGET_VERTICAL_MOTION = WRAM_BATTLE_EFFECT_TARGET_VERTICAL_MOTION,
    TARGET_STATUS = 0x13dbu,
    SPECIAL_TARGET_STATUS = WRAM_BATTLE_EFFECT_SPECIAL_STATUS,
    SPECIAL_TARGET_HORIZONTAL = WRAM_BATTLE_EFFECT_SPECIAL_HORIZONTAL,
    SPECIAL_TARGET_VERTICAL = WRAM_BATTLE_EFFECT_SPECIAL_VERTICAL,
    TARGET_STATUS_REQUEST = WRAM_BATTLE_EFFECT_STATUS_REQUEST,
    PARTY_TARGET_STRIDE = 13u,
    ENEMY_TARGET_STRIDE = 15u,
    SPECIAL_TARGET_NUMBER = 4u
};

static uint8_t EffectTargetStateContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x81u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->decimal && cpu->direct_page == 0u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void ReadTargetMotionOffsets(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    for (unsigned axis = 0u; axis < 2u; ++axis) {
        OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_TARGET_STREAM));
        OpStepMem(memory, cpu, OpDp(cpu, DP_TARGET_STREAM), 1);
        OpBitValue(cpu, 0x80u);
        if (cpu->zero)
            OpAndValue(cpu, 0xffu);
        else
            OpOraValue(cpu, 0xff00u);
        OpSta(memory, cpu, OpAbs(cpu,
            (uint16_t)(TARGET_HORIZONTAL_OFFSET + axis * 2u)));
    }
    OpSepWidths(cpu, 0x20u);
}

Lufia2ExecutionResult Lufia2BattleEffectSetTargetMotionOffsets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectTargetStateContext(cpu))
        return ExecutionHandoff(cpu, 0x819af7u);
    OpSetDataBank(memory, cpu, 0x7eu);
    PushY(memory, cpu);
    ReadTargetMotionOffsets(memory, cpu);
    LoadA16(cpu, cpu->direct_page);
    LoadA8(cpu, PARTY_TARGET_STRIDE);
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_TARGET_NUMBER));
    if (!cpu->negative) {
        OpCmpValue(cpu, SPECIAL_TARGET_NUMBER);
        if (!cpu->zero) {
            OpSta(memory, cpu, SNES_WRMPYB);
            OpLda(memory, cpu, SNES_RDMPYL);
            OpTax(cpu);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpAbs(cpu, TARGET_HORIZONTAL_OFFSET));
            OpSta(memory, cpu, OpAbsX(cpu, TARGET_HORIZONTAL_MOTION));
            OpLda(memory, cpu, OpAbs(cpu, TARGET_VERTICAL_OFFSET));
            OpSta(memory, cpu, OpAbsX(cpu, TARGET_VERTICAL_MOTION));
            OpSepWidths(cpu, 0x20u);
        }
    }
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819b55u);
}

Lufia2ExecutionResult Lufia2BattleEffectMoveSpecialTarget(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectTargetStateContext(cpu))
        return ExecutionHandoff(cpu, 0x819d1fu);
    OpSetDataBank(memory, cpu, 0x7eu);
    PushY(memory, cpu);
    ReadTargetMotionOffsets(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x7fu);
    for (unsigned shift = 0u; shift < 3u; ++shift)
        OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbs(cpu, SPECIAL_TARGET_HORIZONTAL));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, TARGET_HORIZONTAL_OFFSET));
    OpSta(memory, cpu, OpAbs(cpu, SPECIAL_TARGET_HORIZONTAL));
    OpLda(memory, cpu, OpAbs(cpu, SPECIAL_TARGET_VERTICAL));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, TARGET_VERTICAL_OFFSET));
    OpSta(memory, cpu, OpAbs(cpu, SPECIAL_TARGET_VERTICAL));
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819d74u);
}

static void SetEnemyTargetStatus(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpTyx(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_TARGET_NUMBER));
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    OpAndValue(cpu, 0x7fu);
    OpSta(memory, cpu, SNES_WRMPYA);
    LoadA8(cpu, ENEMY_TARGET_STRIDE);
    OpSta(memory, cpu, SNES_WRMPYB);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, SNES_RDMPYL);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbsX(cpu, TARGET_STATUS));
    OpAndValue(cpu, 0x7fu);
    OpOra(memory, cpu, OpAbs(cpu, TARGET_HORIZONTAL_OFFSET));
    OpSta(memory, cpu, OpAbsX(cpu, TARGET_STATUS));
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectSetTargetStatus(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!EffectTargetStateContext(cpu))
        return ExecutionHandoff(cpu, 0x819c76u);
    OpSetDataBank(memory, cpu, 0x7eu);
    PushY(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_TARGET_STREAM));
    OpStepMem(memory, cpu, OpDp(cpu, DP_TARGET_STREAM), 1);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_HORIZONTAL_OFFSET));
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpAbsY(cpu, EFFECT_TARGET_NUMBER));
    if (cpu->negative)
        SetEnemyTargetStatus(memory, cpu);
    else {
        OpCmpValue(cpu, SPECIAL_TARGET_NUMBER);
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbs(cpu, SPECIAL_TARGET_STATUS));
            OpAndValue(cpu, 0x7fu);
            OpOra(memory, cpu, OpAbs(cpu, TARGET_HORIZONTAL_OFFSET));
            OpSta(memory, cpu, OpAbs(cpu, SPECIAL_TARGET_STATUS));
        }
    }
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, TARGET_STATUS_REQUEST));
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819ccbu);
}
