#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/battle.h"

enum {
    DP_MOTION_SCRIPT = 0xc3u,
    DP_MOTION_TARGET = 0x11u,
    MOTION_TARGET = 0x25u,
    MOTION_HORIZONTAL = 0x15efu,
    MOTION_VERTICAL = 0x15f1u,
    MOTION_PARTY_ORIGIN = 0x13a0u,
    MOTION_PARTY_SPRITES = 0x48c0u,
    MOTION_ENEMY_HORIZONTAL = 0x13e3u,
    MOTION_ENEMY_VERTICAL = 0x13e5u,
    MOTION_PARTY_STRIDE = 13u,
    MOTION_ENEMY_STRIDE = 15u,
    MOTION_TILE_SIZE = 16u
};

static void ReadMotionOffsets(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    for (unsigned axis = 0u; axis < 2u; ++axis) {
        OpLda(memory, cpu, DirectLongPointer(memory, cpu, DP_MOTION_SCRIPT));
        OpStepMem(memory, cpu, OpDp(cpu, DP_MOTION_SCRIPT), 1);
        OpBitValue(cpu, 0x80u);
        if (cpu->zero)
            OpAndValue(cpu, 0xffu);
        else
            OpOraValue(cpu, 0xff00u);
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(MOTION_HORIZONTAL + axis * 2u)));
    }
    OpSepWidths(cpu, 0x20u);
}

static void MovePartyTarget(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Write8(memory, SNES_WRMPYA, A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, MOTION_PARTY_STRIDE);
    Write8(memory, SNES_WRMPYB, A8(cpu));
    OpLoadA(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpAbsY(cpu, MOTION_TARGET));
    OpAslA(cpu);
    OpAslA(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, SNES_RDMPYL);
    OpTax(cpu);
    for (unsigned axis = 0u; axis < 2u; ++axis) {
        const uint16_t coordinate = (uint16_t)(MOTION_PARTY_SPRITES + axis);
        OpLda(memory, cpu, OpAbsX(cpu, MOTION_PARTY_ORIGIN));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, (uint16_t)(MOTION_HORIZONTAL + axis * 2u)));
        OpSta(memory, cpu, OpAbsY(cpu, coordinate));
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(coordinate + (axis ? 5u : 10u))));
        cpu->carry = 0u;
        OpAdcValue(cpu, MOTION_TILE_SIZE);
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(coordinate + (axis ? 10u : 5u))));
        OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(coordinate + 15u)));
    }
}

static void MoveEnemyTarget(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpAndValue(cpu, 0x7fu);
    Write8(memory, SNES_WRMPYA, A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, MOTION_ENEMY_STRIDE);
    Write8(memory, SNES_WRMPYB, A8(cpu));
    ExchangeAccumulatorBytes(cpu);
    OpTyx(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_MOTION_TARGET));
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, SNES_RDMPYL);
    OpTax(cpu);
    for (unsigned axis = 0u; axis < 2u; ++axis) {
        const uint16_t coordinate = (uint16_t)(MOTION_ENEMY_HORIZONTAL + axis * 2u);
        OpLda(memory, cpu, OpAbsX(cpu, coordinate));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, (uint16_t)(MOTION_HORIZONTAL + axis * 2u)));
        OpSta(memory, cpu, OpAbsX(cpu, coordinate));
    }
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
}

Lufia2ExecutionResult Lufia2BattleEffectMoveTarget(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x81u || !cpu->accumulator_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x819bbeu);
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    PushY(memory, cpu);
    ReadMotionOffsets(memory, cpu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpAbsY(cpu, MOTION_TARGET));
    if (cpu->negative)
        MoveEnemyTarget(memory, cpu);
    else {
        OpCmpValue(cpu, 4u);
        if (!cpu->zero)
            MovePartyTarget(memory, cpu);
    }
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x819c75u);
}
