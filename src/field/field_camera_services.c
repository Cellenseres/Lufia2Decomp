#include "lufia2/field.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

enum {
    PRESSED_LOW = 0x46,
    PRESSED_HIGH = 0x47,
    PRESSED_LATCH_LOW = 0x4a,
    PRESSED_LATCH_HIGH = 0x4b,
    MAP_RESOURCE_FLAGS = 0x91b8b5,
    MAP_RESOURCE_VALUE = 0x91b8b7
};

static uint8_t CameraWidths(const Lufia2CpuState *cpu) {
    return cpu->accumulator_is_8_bit && !cpu->index_is_8_bit;
}

static void ConsumePressed(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t pressed, uint8_t latch) {
    OpAndValue(cpu, OpReadM(memory, cpu, OpDp(cpu, pressed)));
    if (!cpu->zero)
        OpTestBits(memory, cpu, OpDp(cpu, latch), 0u);
}

Lufia2ExecutionResult Lufia2FieldConsumePressedLow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CameraWidths(cpu))
        return ExecutionHandoff(cpu, 0x8eb142u);
    ConsumePressed(memory, cpu, PRESSED_LOW, PRESSED_LATCH_LOW);
    return ExecutionReturned(0x8eb148u);
}

Lufia2ExecutionResult Lufia2FieldConsumePressedHigh(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CameraWidths(cpu))
        return ExecutionHandoff(cpu, 0x8eb149u);
    ConsumePressed(memory, cpu, PRESSED_HIGH, PRESSED_LATCH_HIGH);
    return ExecutionReturned(0x8eb14fu);
}

Lufia2ExecutionResult Lufia2FieldReadBankedStreamByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CameraWidths(cpu))
        return ExecutionHandoff(cpu, 0x8eb7cdu);
    OpLda(memory, cpu, OpAbsY(cpu, 0u));
    OpIny(cpu);
    if (!cpu->negative) {
        PushAccumulator8(memory, cpu);
        OpLda(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK));
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_SCENE_SCRIPT_BANK));
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadA8(cpu, Pull8(memory, cpu));
        OpLdy(cpu, 0x8000u);
    }
    return ExecutionReturned(0x8eb7e1u);
}

Lufia2ExecutionResult Lufia2FieldMarkEncounterActor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CameraWidths(cpu))
        return ExecutionHandoff(cpu, 0x8ebae7u);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_PRIMARY_TIMER));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpOraValue(cpu, 0x10u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    return ExecutionReturned(0x8ebaf5u);
}

Lufia2ExecutionResult Lufia2FieldCountValue2B(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8ec1c5u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpAndValue(cpu, 0x3ffu);
    OpCmpValue(cpu, 0x2bu);
    const uint8_t matches = cpu->zero;
    if (matches) {
        OpStepMem(memory, cpu, OpAbs(cpu, WRAM_FIELD_VALUE_2B_COUNT), 1);
        if (cpu->zero)
            OpStepMem(memory, cpu, OpAbs(cpu, WRAM_FIELD_VALUE_2B_COUNT), -1);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->carry = matches;
    return ExecutionReturned(matches ? 0x8ec1dau : 0x8ec1ddu);
}

Lufia2ExecutionResult Lufia2FieldReadMapResourceFlags(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!CameraWidths(cpu))
        return ExecutionHandoff(cpu, 0x8ec36du);
    OpLda(memory, cpu, OpLongX(cpu, MAP_RESOURCE_FLAGS));
    OpAndValue(cpu, 0x40u);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAdcValue(cpu, 0u);
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpLongX(cpu, MAP_RESOURCE_VALUE));
    return ExecutionReturned(0x8ec37cu);
}
