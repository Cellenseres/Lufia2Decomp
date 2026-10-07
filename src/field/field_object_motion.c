#include "core/cpu_ops.h"
#include "field_coordinates_internal.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    MOTION_DISTANCE = 0x54,
    MOTION_RATE = 0x56,
    MOTION_ORIGIN = 0x58,
    MOTION_CURRENT = 0x5d,
    MOTION_TARGET = 0x60,
    DIVIDEND = 0x4204,
    DIVISOR = 0x4206,
    QUOTIENT = 0x4214,
    MULTIPLICAND = 0x211b,
    MULTIPLIER = 0x211c,
    PRODUCT = 0x2134
};

static bool MotionContext(const Lufia2CpuState *cpu) {
    return !cpu->accumulator_is_8_bit && !cpu->index_is_8_bit &&
        !cpu->decimal && !cpu->direct_page && cpu->program_bank == 0x83u &&
        cpu->stack >= 0x1f00u && cpu->stack <= 0x1ffcu;
}

static void MotionExtractDirection(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t distance = Read16Direct(memory, cpu, MOTION_DISTANCE);
    const uint16_t shifted = (uint16_t)(distance << 1);
    cpu->carry = (distance & 0x8000u) != 0;
    Write8(memory, DirectAddress(cpu, MOTION_DISTANCE + 1u), (uint8_t)(shifted >> 8));
    Write8(memory, DirectAddress(cpu, MOTION_DISTANCE), (uint8_t)shifted);
    SetNz16(cpu, shifted);
}

void Lufia2ObjectInterpolateCoordinateBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_SLOT_WORD_OFFSET)));
    OpSta(memory, cpu, OpDp(cpu, MOTION_DISTANCE));
    OpOraValue(cpu, 0u);
    if (cpu->negative) {
        OpLoadA(cpu, cpu->accumulator ^ 0xffffu);
        OpIncA(cpu);
    }
    OpSta(memory, cpu, OpAbs(cpu, DIVIDEND));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpAbs(cpu, DIVISOR));
    SimulateJsrFrame(memory, cpu, 0xe627u);
    SimulateRtsFrame(memory, cpu);
    OpLda(memory, cpu, OpAbs(cpu, QUOTIENT));
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLICAND));
    OpLda(memory, cpu, OpAbs(cpu, QUOTIENT + 1u));
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLICAND));
    OpLda(memory, cpu, OpDp(cpu, MOTION_RATE));
    OpSta(memory, cpu, OpAbs(cpu, MULTIPLIER));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, PRODUCT));
    MotionExtractDirection(memory, cpu);
    if (cpu->carry) {
        cpu->carry = false;
        OpAdc(memory, cpu, OpDp(cpu, MOTION_ORIGIN));
    } else {
        cpu->carry = true;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, MOTION_ORIGIN)));
        OpLoadA(cpu, cpu->accumulator ^ 0xffffu);
        OpIncA(cpu);
    }
}

Lufia2ExecutionResult Lufia2ObjectInterpolateCoordinate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionContext(cpu))
        return ExecutionHandoff(cpu, 0x83e60eu);
    Lufia2ObjectInterpolateCoordinateBody(memory, cpu);
    return ExecutionReturned(0x83e650u);
}

void Lufia2ObjectApproachCoordinateBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, MOTION_TARGET));
    OpLda(memory, cpu, DirectLongPointer(memory, cpu, MOTION_CURRENT));
    cpu->carry = true;
    OpSbcValue(cpu, OpReadM(memory, cpu, DirectLongPointer(memory, cpu, MOTION_TARGET)));
    if (cpu->zero)
        return;
    if (cpu->negative) {
        OpLoadA(cpu, cpu->accumulator ^ 0xffffu);
        OpIncA(cpu);
        OpCmp(memory, cpu, OpDp(cpu, MOTION_DISTANCE));
        OpLda(memory, cpu, OpDp(cpu, MOTION_DISTANCE));
        if (!cpu->carry) {
            OpLda(memory, cpu, DirectLongPointer(memory, cpu, MOTION_TARGET));
            OpSta(memory, cpu, DirectLongPointer(memory, cpu, MOTION_CURRENT));
            return;
        }
    } else {
        OpCmp(memory, cpu, OpDp(cpu, MOTION_DISTANCE));
        OpLda(memory, cpu, OpDp(cpu, MOTION_DISTANCE));
        OpLoadA(cpu, cpu->accumulator ^ 0xffffu);
        OpIncA(cpu);
        if (!cpu->carry) {
            OpLda(memory, cpu, DirectLongPointer(memory, cpu, MOTION_TARGET));
            OpSta(memory, cpu, DirectLongPointer(memory, cpu, MOTION_CURRENT));
            return;
        }
    }
    cpu->carry = false;
    OpAdc(memory, cpu, DirectLongPointer(memory, cpu, MOTION_CURRENT));
    OpSta(memory, cpu, DirectLongPointer(memory, cpu, MOTION_CURRENT));
}

Lufia2ExecutionResult Lufia2ObjectApproachCoordinate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!MotionContext(cpu))
        return ExecutionHandoff(cpu, 0x83e6aau);
    Lufia2ObjectApproachCoordinateBody(memory, cpu);
    return ExecutionReturned(0x83e6d2u);
}
