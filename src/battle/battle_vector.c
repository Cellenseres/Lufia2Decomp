/* Battle vectors: the sine table lookup for an angle in $54 and the
 * conversion of an angle and speed into the two velocity words. */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"
#include "lufia2/system.h"

enum {
    ANGLE = 0x54u,
    ANGLE_WORK = 0x55u,
    SPEED = 0x5au,
    VELOCITY_A = 0x56u,
    VELOCITY_B = 0x58u,
    FACTOR_LOW = 0x4eu,
    FACTOR_BYTE = 0x50u,
    PRODUCT = 0x52u,
    SINE = 0x63u,
    SIGNS = 0x64u,
    SINE_TABLE = 0x97b226u,
    SINE_NEGATIVE = 0x8000u,
    QUARTER = 0x40u
};

/* Looks the angle in A up in the quarter-wave table; the sign bit tells
 * the second half. Both entries pull the status they pushed. */
static Lufia2ExecutionResult SineLookup(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t angle = A8(cpu);
    uint16_t word;
    uint32_t pc;

    if (angle < QUARTER) {
        LoadX8(cpu, (uint8_t)(angle << 1));
        pc = 0x85de6bu;
    } else if (angle < 2 * QUARTER) {
        LoadX8(cpu, (uint8_t)((2 * QUARTER - angle) << 1));
        pc = 0x85de6bu;
    } else if (angle < 3 * QUARTER) {
        LoadX8(cpu, (uint8_t)((angle & 0x3fu) << 1));
        pc = 0x85de61u;
    } else {
        LoadX8(cpu, (uint8_t)(((uint8_t)(angle ^ 0xffu) + 1u) << 1));
        pc = 0x85de61u;
    }
    SetAccumulatorWidth(cpu, 0);
    word = Read16Long(memory, LongIndexedAddress(SINE_TABLE, cpu->x));
    if (pc == 0x85de61u)
        word |= SINE_NEGATIVE;
    LoadA16(cpu, word);
    Write16Direct(memory, cpu, SINE, word);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(pc);
}

/* Pushes the status, switches to M8/X8 and stages the angle in $55. */
static void SineEntry(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
}

/* $85:DE2A: the table word for the angle in $54, flagged negative in the
 * second half-turn, in A and $63. Any width, JSL; the status is restored. */
Lufia2ExecutionResult Lufia2BattleSineOfAngle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SineEntry(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, ANGLE));
    StoreADirect8(memory, cpu, ANGLE_WORK);
    return SineLookup(memory, cpu);
}

/* $85:DE1E: the same lookup for the angle in $54 plus a quarter turn, the
 * cosine. Any width, JSL; the status is restored. */
Lufia2ExecutionResult Lufia2BattleCosineOfAngle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SineEntry(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, ANGLE));
    cpu->carry = 0;
    Adc8(cpu, QUARTER);
    StoreADirect8(memory, cpu, ANGLE_WORK);
    return SineLookup(memory, cpu);
}

/* One JSL into bank $85; false when it handed off. */
static int CallSine(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address,
    Lufia2ExecutionResult (*routine)(const Lufia2Memory *, Lufia2CpuState *),
    Lufia2ExecutionResult *result) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    *result = routine(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    SimulateRtlFrame(memory, cpu);
    return 1;
}

/* One component: the speed times the table word, with the sign of the
 * word, or the speed itself when bit 0 of $64 is set. */
static int Component(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t target, uint16_t multiply_return, Lufia2ExecutionResult *result) {
    LoadA8(cpu, 0x01u);
    cpu->zero = (DirectByte(memory, cpu, SIGNS) & A8(cpu)) == 0;
    cpu->negative = (DirectByte(memory, cpu, SIGNS) & 0x80u) != 0;
    cpu->overflow = (DirectByte(memory, cpu, SIGNS) & 0x40u) != 0;
    if (!cpu->zero) {
        LoadX16(cpu, Read16Direct(memory, cpu, SPEED));
        StoreXDirect16(memory, cpu, target);
        return 1;
    }
    LoadA8(cpu, DirectByte(memory, cpu, SINE));
    StoreADirect8(memory, cpu, FACTOR_BYTE);
    LoadX16(cpu, Read16Direct(memory, cpu, SPEED));
    StoreXDirect16(memory, cpu, FACTOR_LOW);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, multiply_return);
    *result = Lufia2Multiply16By8(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return 0;
    SimulateRtlFrame(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, SIGNS));
    if (cpu->negative) {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, PRODUCT) ^ 0xffffu));
        LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
        Write16Direct(memory, cpu, target, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
    } else {
        LoadX16(cpu, Read16Direct(memory, cpu, PRODUCT));
        StoreXDirect16(memory, cpu, target);
    }
    return 1;
}

/* Stores the speed, negated or not, in a velocity word and clears the
 * other, for the four axis-aligned angles. */
static void Axis(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t target, uint8_t cleared, int negate) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, SPEED));
    if (negate) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
    }
    Write16Direct(memory, cpu, target, cpu->accumulator);
    Write8(memory, DirectAddress(cpu, cleared), 0);
    Write8(memory, DirectAddress(cpu, (uint8_t)(cleared + 1u)), 0);
    SetAccumulatorWidth(cpu, 1);
}

/* $85:DD63: the velocity words $56 and $58 for the angle in $54 and the
 * speed in $5A, speed times sine and cosine of the angle. The direct page
 * must be zero for the product routine. M8/X16, JSL. */
Lufia2ExecutionResult Lufia2BattleVelocityOfAngle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result = ExecutionReturned(0x85de1du);

    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85dd63u);
    PushY(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, ANGLE));
    BitImmediate8(cpu, 0x3fu);
    if (cpu->zero) {
        And8(cpu, 0xc0u);
        if (cpu->zero) {
            Axis(memory, cpu, VELOCITY_B, VELOCITY_A, 0);
            result.pc = 0x85ddafu;
        } else {
            cpu->carry = 1;
            Sbc8(cpu, 0x40u);
            if (cpu->zero) {
                Axis(memory, cpu, VELOCITY_A, VELOCITY_B, 0);
                result.pc = 0x85dda3u;
            } else {
                cpu->carry = 1;
                Sbc8(cpu, 0x40u);
                if (cpu->zero) {
                    Axis(memory, cpu, VELOCITY_B, VELOCITY_A, 1);
                    result.pc = 0x85dd97u;
                } else {
                    Axis(memory, cpu, VELOCITY_A, VELOCITY_B, 1);
                    result.pc = 0x85dd87u;
                }
            }
        }
        cpu->y = PullIndexValue(memory, cpu);
        return result;
    }
    if (!CallSine(memory, cpu, 0xddb3u, Lufia2BattleSineOfAngle, &result) ||
        !Component(memory, cpu, VELOCITY_A, 0xddceu, &result) ||
        !CallSine(memory, cpu, 0xdde9u, Lufia2BattleCosineOfAngle, &result) ||
        !Component(memory, cpu, VELOCITY_B, 0xde04u, &result))
        return result;
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x85de1du);
}
