#ifndef LUFIA2_CORE_PLAIN_OPS_H
#define LUFIA2_CORE_PLAIN_OPS_H

/* Arithmetic results and caller-visible CPU state. */

#include <stdbool.h>
#include <stdint.h>

#include "core/cpu_internal.h"
#include "core/arithmetic_value.h"

static inline Word16Result Sum16(uint16_t left, uint16_t right, bool carry) {
    return Sum16Mode(left, right, carry, false);
}

/* Subtraction with no borrow in. */
static inline Word16Result Difference16(uint16_t left, uint16_t right) {
    return Sum16(left, (uint16_t)~right, true);
}

static inline Byte8Result Sum8(uint8_t left, uint8_t right, bool carry) {
    return Sum8Mode(left, right, carry, false);
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

/* Export the value with its sign and zero flags. */
static inline void LeaveWord(Lufia2CpuState *cpu, uint16_t value) {
    LoadA16(cpu, value);
}

/* Export the arithmetic value and flags. */
static inline void LeaveSum(Lufia2CpuState *cpu, Word16Result result) {
    cpu->carry = result.carry;
    cpu->overflow = result.overflow;
    LoadA16(cpu, result.value);
}

/* Export carry and overflow without changing the accumulator. */
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

/* Export comparison flags without changing the accumulator. */
static inline void LeaveComparison(
    Lufia2CpuState *cpu, uint16_t left, uint16_t right) {
    Compare16(cpu, left, right);
}

/* Comparison replaces carry, sign and zero, preserving arithmetic overflow. */
static inline void LeaveSumCompared(
    Lufia2CpuState *cpu, Word16Result result, uint16_t limit) {
    cpu->overflow = result.overflow;
    cpu->accumulator = result.value;
    Compare16(cpu, result.value, limit);
}

/* Export counter flags. */
static inline void LeaveCounter(Lufia2CpuState *cpu, uint16_t value) {
    SetNz16(cpu, value);
}

#endif
