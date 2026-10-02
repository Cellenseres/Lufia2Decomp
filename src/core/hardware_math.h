#ifndef LUFIA2_CORE_HARDWARE_MATH_H
#define LUFIA2_CORE_HARDWARE_MATH_H

/* The CPU's multiplication unit. */

#include "core/memory_internal.h"
#include "core/snes_registers.h"

/* Unsigned 8 x 8 multiply through the hardware registers, in the order the
 * unit expects: both operands, then the 16-bit product. */
static inline uint16_t HardwareMultiply8(const Lufia2Memory *memory,
                                         uint8_t multiplicand, uint8_t multiplier) {
    Write8(memory, SNES_WRMPYA, multiplicand);
    Write8(memory, SNES_WRMPYB, multiplier);
    return Read16Long(memory, SNES_RDMPYL);
}

#endif
