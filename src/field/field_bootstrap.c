#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "system/wram.h"

static void BootstrapSaveRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
}

static void BootstrapRestoreRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

Lufia2ExecutionResult Lufia2FieldResetSpriteBuffer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u)
        return ExecutionHandoff(cpu, 0x83ac7au);
    BootstrapSaveRegisters(memory, cpu);
    SetAccumulatorWidth(cpu, 1u);
    SetIndexWidth(cpu, 0u);
    OpLoadA(cpu, 0x83u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0x01fcu);
    OpLoadA(cpu, 0xf0u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_FIELD_OAM_LOW_BUFFER));
        OpSta(memory, cpu, OpAbsX(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 1u));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 2u));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_FIELD_OAM_LOW_BUFFER + 3u));
        for (unsigned byte = 0u; byte < 4u; ++byte)
            OpDex(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, 0x1eu);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_FIELD_OAM_HIGH_BUFFER));
        OpStz(memory, cpu, OpAbsX(cpu, WRAM_FIELD_OAM_HIGH_BUFFER + 1u));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, WRAM_FIELD_OAM_DIRTY);
    BootstrapRestoreRegisters(memory, cpu);
    return ExecutionReturned(0x83acb6u);
}

Lufia2ExecutionResult Lufia2FieldResetObjectAnimation(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83b512u);
    OpLdx(cpu, 6u);
    LoadA16(cpu, cpu->direct_page);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_STATE_BITS));
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_OBJECT_STATE_BITS + 1u));
        OpDex(cpu);
        OpDex(cpu);
    } while (!cpu->negative);
    OpLdx(cpu, 7u);
    LoadA16(cpu, cpu->direct_page);
    do {
        OpSta(memory, cpu, OpLongX(cpu, WRAM_FIELD_ANIMATION_SLOT_STATE));
        OpDex(cpu);
    } while (!cpu->negative);
    return ExecutionReturned(0x83b52du);
}

Lufia2ExecutionResult Lufia2FieldSelectSceneRecordBase(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || cpu->decimal)
        return ExecutionHandoff(cpu, 0x83b5adu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1u);
    LoadA16(cpu, cpu->direct_page);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
    SetAccumulatorWidth(cpu, 0u);
    SetIndexWidth(cpu, 0u);
    for (unsigned bit = 0u; bit < 3u; ++bit)
        OpAslA(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x878010u));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x8000u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BASE));
    SetAccumulatorWidth(cpu, 1u);
    OpLda(memory, cpu, OpLongX(cpu, 0x878012u));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x87u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_RECORD_BANK));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x83b5d2u);
}
