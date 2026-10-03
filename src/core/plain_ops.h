#ifndef LUFIA2_CORE_PLAIN_OPS_H
#define LUFIA2_CORE_PLAIN_OPS_H

/* Plain arithmetic for converted routines, small stack helpers, and the few
 * ways a routine hands its result back to the caller: the accumulator, and
 * the flags the original left behind, which later code still reads. */

#include <stdbool.h>
#include <stdint.h>

#include "core/cpu_internal.h"

/* A sum or difference with the flags the original instruction produced. */
typedef struct {
    uint16_t value;
    bool carry;
    bool overflow;
} Word16Result;

typedef struct {
    uint8_t value;
    bool carry;
    bool overflow;
} Byte8Result;

static inline Word16Result Sum16(uint16_t left, uint16_t right, bool carry) {
    const uint32_t sum = (uint32_t)left + right + (carry ? 1u : 0u);
    Word16Result result;

    result.value = (uint16_t)sum;
    result.carry = sum > 0xffffu;
    result.overflow =
        ((~(left ^ right) & (left ^ result.value)) & 0x8000u) != 0;
    return result;
}

/* Subtraction with no borrow in. */
static inline Word16Result Difference16(uint16_t left, uint16_t right) {
    return Sum16(left, (uint16_t)~right, true);
}

static inline Byte8Result Sum8(uint8_t left, uint8_t right, bool carry) {
    const uint16_t sum = (uint16_t)left + right + (carry ? 1u : 0u);
    Byte8Result result;

    result.value = (uint8_t)sum;
    result.carry = sum > 0xffu;
    result.overflow =
        ((~(left ^ right) & (left ^ result.value)) & 0x80u) != 0;
    return result;
}

static inline Byte8Result Difference8(uint8_t left, uint8_t right) {
    return Sum8(left, (uint8_t)~right, true);
}

/* A 16-bit value on the stack, high byte pushed first. */
static inline void PushStackWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t value) {
    Push8(memory, cpu, (uint8_t)(value >> 8));
    Push8(memory, cpu, (uint8_t)value);
}

static inline uint16_t PullStackWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint8_t low = Pull8(memory, cpu);

    return (uint16_t)(low | ((uint16_t)Pull8(memory, cpu) << 8));
}

/* The accumulator holds `value`; the sign and zero flags describe it. */
static inline void LeaveWord(Lufia2CpuState *cpu, uint16_t value) {
    LoadA16(cpu, value);
}

/* The accumulator holds the result of an addition or subtraction. */
static inline void LeaveSum(Lufia2CpuState *cpu, Word16Result result) {
    cpu->carry = result.carry;
    cpu->overflow = result.overflow;
    LoadA16(cpu, result.value);
}

/* Only the carry and overflow flags of an addition or subtraction. */
static inline void SetSumFlags(Lufia2CpuState *cpu, Word16Result result) {
    cpu->carry = result.carry;
    cpu->overflow = result.overflow;
}

/* Only the low byte of the accumulator changes. */
static inline void LeaveByteSum(Lufia2CpuState *cpu, Byte8Result result) {
    cpu->carry = result.carry;
    cpu->overflow = result.overflow;
    LoadA8(cpu, result.value);
}

/* Flags of a comparison; the accumulator is left alone. */
static inline void LeaveComparison(
    Lufia2CpuState *cpu, uint16_t left, uint16_t right) {
    Compare16(cpu, left, right);
}

/* The accumulator holds a sum or difference that was then compared with
 * `limit`: the comparison sets the carry, sign and zero flags, the overflow
 * flag stays that of the arithmetic. */
static inline void LeaveSumCompared(
    Lufia2CpuState *cpu, Word16Result result, uint16_t limit) {
    cpu->overflow = result.overflow;
    cpu->accumulator = result.value;
    Compare16(cpu, result.value, limit);
}

/* Sign and zero flags of a counter or index that was just stepped. */
static inline void LeaveCounter(Lufia2CpuState *cpu, uint16_t value) {
    SetNz16(cpu, value);
}

#endif
