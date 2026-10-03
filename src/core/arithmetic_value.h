#ifndef LUFIA2_CORE_ARITHMETIC_VALUE_H
#define LUFIA2_CORE_ARITHMETIC_VALUE_H

#include <stdbool.h>
#include <stdint.h>

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

/* Decimal correction also preserves behavior for invalid BCD digits. */
static inline Word16Result ArithmeticValue(
    uint16_t left, uint16_t right, bool carry, bool decimal,
    bool subtract, unsigned bits) {
    const uint32_t limit = 1u << bits;
    const uint32_t mask = limit - 1u;
    const uint32_t sign = limit >> 1;
    const uint32_t operand = subtract ? ((uint32_t)right ^ mask) : right;
    int32_t total;
    Word16Result result;

    if (!decimal) {
        total = (int32_t)(left + operand + (carry ? 1u : 0u));
    } else {
        unsigned shift;

        total = (int32_t)((left & 15u) + (operand & 15u) + (carry ? 1u : 0u));
        for (shift = 4; shift < bits; shift += 4) {
            const int32_t digit_limit = (int32_t)(1u << shift);
            const int32_t correction = (int32_t)(6u << (shift - 4));

            if (subtract && total < digit_limit) {
                total -= correction;
                total = (int32_t)((uint32_t)total &
                    (uint32_t)(total < 0 ? digit_limit - 1 : 2 * digit_limit - 1));
            } else if (!subtract && total > (int32_t)((10u << (shift - 4)) - 1u)) {
                total = ((total + correction) & (digit_limit - 1)) + digit_limit;
            }
            total += (int32_t)((left & (15u << shift)) +
                (operand & (15u << shift)));
        }
    }
    /* Overflow precedes the final decimal correction. */
    result.overflow = ((~(left ^ operand) & (left ^ (uint32_t)total)) & sign) != 0;
    if (decimal) {
        const int32_t correction = (int32_t)(6u << (bits - 4));

        if (subtract && total < (int32_t)limit)
            total -= correction;
        else if (!subtract && total > (int32_t)((10u << (bits - 4)) - 1u))
            total += correction;
    }
    result.value = (uint16_t)((uint32_t)total & mask);
    result.carry = total > (int32_t)mask;
    return result;
}

static inline Word16Result Sum16Mode(
    uint16_t left, uint16_t right, bool carry, bool decimal) {
    return ArithmeticValue(left, right, carry, decimal, false, 16);
}

static inline Word16Result Difference16Mode(
    uint16_t left, uint16_t right, bool decimal) {
    return ArithmeticValue(left, right, true, decimal, true, 16);
}

static inline Byte8Result Sum8Mode(
    uint8_t left, uint8_t right, bool carry, bool decimal) {
    const Word16Result word = ArithmeticValue(left, right, carry, decimal, false, 8);
    const Byte8Result result = {(uint8_t)word.value, word.carry, word.overflow};

    return result;
}

static inline Byte8Result Difference8Mode(
    uint8_t left, uint8_t right, bool decimal) {
    const Word16Result word = ArithmeticValue(left, right, true, decimal, true, 8);
    const Byte8Result result = {(uint8_t)word.value, word.carry, word.overflow};

    return result;
}

#endif
