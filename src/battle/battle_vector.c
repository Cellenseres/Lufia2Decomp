/* Battle vectors: the sine table lookup for an angle in $54 and the
 * conversion of an angle and speed into the two velocity words. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/wram_view.h"
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
    QUARTER = 0x40u,
    SIGN_LOW_BIT = 0x01u,
    SIGN_NEGATIVE = 0x80u,
    SIGN_OVERFLOW = 0x40u,
    PRODUCT_BANK = 0x85u
};

/* Looks the angle in A up in the quarter-wave table; the sign bit tells
 * the second half. Both entries pull the status they pushed. */
static Lufia2ExecutionResult SineLookup(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2Wram wram, uint8_t angle) {
    uint8_t index;
    uint16_t word;
    bool second_half;

    if (angle < QUARTER) {
        index = (uint8_t)(angle << 1);
        second_half = false;
    } else if (angle < 2 * QUARTER) {
        index = (uint8_t)((2 * QUARTER - angle) << 1);
        second_half = false;
    } else if (angle < 3 * QUARTER) {
        index = (uint8_t)((angle & 0x3fu) << 1);
        second_half = true;
    } else {
        index = (uint8_t)((uint8_t)(0x100u - angle) << 1);
        second_half = true;
    }
    word = WramRead16At(WramViewLong(memory), SINE_TABLE, index);
    if (second_half)
        word |= SINE_NEGATIVE;
    WramWrite16(wram, SINE, word);
    cpu->x = index;
    cpu->accumulator = word;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(second_half ? 0x85de61u : 0x85de6bu);
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
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint8_t angle = WramRead(wram, ANGLE);

    SineEntry(memory, cpu);
    WramWrite(wram, ANGLE_WORK, angle);
    return SineLookup(memory, cpu, wram, angle);
}

/* $85:DE1E: the same lookup for the angle in $54 plus a quarter turn, the
 * cosine. Any width, JSL; the status is restored. */
Lufia2ExecutionResult Lufia2BattleCosineOfAngle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint8_t angle = (uint8_t)(WramRead(wram, ANGLE) + QUARTER);

    SineEntry(memory, cpu);
    WramWrite(wram, ANGLE_WORK, angle);
    return SineLookup(memory, cpu, wram, angle);
}

/* One JSL into bank $85; false when it handed off. */
static bool CallSine(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t return_address,
    Lufia2ExecutionResult (*routine)(const Lufia2Memory *, Lufia2CpuState *),
    Lufia2ExecutionResult *result) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    *result = routine(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    SimulateRtlFrame(memory, cpu);
    return true;
}

/* One component: the speed times the table word, with the sign of the
 * word, or the speed itself when bit 0 of $64 is set. */
static bool Component(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2Wram wram, uint8_t target, uint16_t multiply_return,
    Lufia2ExecutionResult *result) {
    const uint8_t signs = WramRead(wram, SIGNS);
    const uint16_t speed = WramRead16(wram, SPEED);
    uint16_t value;

    LoadA8(cpu, SIGN_LOW_BIT);
    cpu->zero = (signs & SIGN_LOW_BIT) == 0;
    cpu->negative = (signs & SIGN_NEGATIVE) != 0;
    cpu->overflow = (signs & SIGN_OVERFLOW) != 0;
    if ((signs & SIGN_LOW_BIT) != 0) {
        LoadX16(cpu, speed);
        WramWrite16(wram, target, speed);
        return true;
    }
    LoadA8(cpu, WramRead(wram, SINE));
    WramWrite(wram, FACTOR_BYTE, A8(cpu));
    LoadX16(cpu, speed);
    WramWrite16(wram, FACTOR_LOW, speed);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PRODUCT_BANK);
    PullDataBank(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, multiply_return);
    *result = Lufia2Multiply16By8(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    SimulateRtlFrame(memory, cpu);
    PullDataBank(memory, cpu);
    LoadA8(cpu, WramRead(wram, SIGNS));
    value = WramRead16(wram, PRODUCT);
    if ((signs & SIGN_NEGATIVE) != 0) {
        value = (uint16_t)(~value + 1u);
        LoadA16(cpu, value);
    } else {
        LoadX16(cpu, value);
    }
    WramWrite16(wram, target, value);
    return true;
}

/* Stores the speed, negated or not, in a velocity word and clears the
 * other, for the four axis-aligned angles. */
typedef struct {
    uint8_t target;
    uint8_t cleared;
    bool negate;
    uint32_t pc;
} AxisAngle;

static const AxisAngle kAxisAngles[4] = {
    { VELOCITY_B, VELOCITY_A, false, 0x85ddafu },
    { VELOCITY_A, VELOCITY_B, false, 0x85dda3u },
    { VELOCITY_B, VELOCITY_A, true, 0x85dd97u },
    { VELOCITY_A, VELOCITY_B, true, 0x85dd87u }
};

/* $85:DD63: the velocity words $56 and $58 for the angle in $54 and the
 * speed in $5A, speed times sine and cosine of the angle. The direct page
 * must be zero for the product routine. M8/X16, JSL. */
Lufia2ExecutionResult Lufia2BattleVelocityOfAngle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ExecutionResult result = ExecutionReturned(0x85de1du);
    uint8_t angle;

    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x85dd63u);
    PushY(memory, cpu);
    angle = WramRead(wram, ANGLE);
    LoadA8(cpu, angle);
    BitImmediate8(cpu, 0x3fu);
    if (cpu->zero) {
        const AxisAngle *axis = &kAxisAngles[angle >> 6];
        const uint16_t speed = WramRead16(wram, SPEED);
        const uint16_t value = axis->negate ? (uint16_t)(~speed + 1u) : speed;
        Byte8Result steps = { (uint8_t)(angle & 0xc0u), cpu->carry,
            cpu->overflow };
        const unsigned turns = (angle >> 6) < 2u ? (angle >> 6) : 2u;
        unsigned turn;

        for (turn = 0; turn < turns; ++turn)
            steps = Difference8(steps.value, QUARTER);
        cpu->carry = steps.carry;
        cpu->overflow = steps.overflow;
        WramWrite16(wram, axis->target, value);
        WramWrite(wram, axis->cleared, 0);
        WramWrite(wram, (uint32_t)axis->cleared + 1u, 0);
        cpu->accumulator = value;
        cpu->y = PullIndexValue(memory, cpu);
        result.pc = axis->pc;
        return result;
    }
    if (!CallSine(memory, cpu, 0xddb3u, Lufia2BattleSineOfAngle, &result) ||
        !Component(memory, cpu, wram, VELOCITY_A, 0xddceu, &result) ||
        !CallSine(memory, cpu, 0xdde9u, Lufia2BattleCosineOfAngle, &result) ||
        !Component(memory, cpu, wram, VELOCITY_B, 0xde04u, &result))
        return result;
    cpu->y = PullIndexValue(memory, cpu);
    return ExecutionReturned(0x85de1du);
}
