#include "core/cpu_ops.h"
#include "lufia2/actor.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    FIELD_ANIMATION_TILE_PLANE = 0x7f0000u,
    FIELD_ANIMATION_CELL_MARK = 0x0400u,
};

static bool AnimationCellContext(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x83u && cpu->accumulator_is_8_bit &&
        !cpu->index_is_8_bit && !cpu->direct_page && !cpu->decimal &&
        cpu->stack >= 0x1f20u && cpu->stack <= 0x1ffcu;
}

static void AnimationRowPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, WRAM_FIELD_PENDING_OBJECT_X);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_HEIGHT);
    OpCmpValue(cpu, 4u);
    if (cpu->carry)
        OpDecA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, WRAM_FIELD_PENDING_OBJECT_Y);
}

Lufia2ExecutionResult Lufia2FieldFlagAnimationRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!AnimationCellContext(cpu))
        return ExecutionHandoff(cpu, 0x83898eu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    AnimationRowPosition(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x89a6u);
    Lufia2MapCellOffset(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0u);
    OpTxa(cpu);
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_FIELD_LAYER_TABLE_OFFSET)));
    OpAdc(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_CELL_BASE));
    OpTax(cpu);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, WRAM_FIELD_OBJECT_WIDTH);
    OpAndValue(cpu, 0xffu);
    OpTay(cpu);
    do {
        OpLda(memory, cpu, OpLongX(cpu, FIELD_ANIMATION_TILE_PLANE));
        OpOraValue(cpu, FIELD_ANIMATION_CELL_MARK);
        OpSta(memory, cpu, OpLongX(cpu, FIELD_ANIMATION_TILE_PLANE));
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8389cdu);
}
