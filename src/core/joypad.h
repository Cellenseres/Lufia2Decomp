#ifndef LUFIA2_CORE_JOYPAD_H
#define LUFIA2_CORE_JOYPAD_H

/* Joypad words in the direct page: the buttons held and the presses that
 * handlers have not consumed yet. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "system/wram.h"

/* Byte of the joypad words: A X L R, then B Y Select Start and the pad. */
enum {
    JOYPAD_LOW_BYTE = 0,
    JOYPAD_HIGH_BYTE = 1,
};

enum {
    JOY_LOW_A = 0x80,
    JOY_LOW_X = 0x40,
    JOY_LOW_L = 0x20,
    JOY_LOW_R = 0x10,
};

enum {
    JOY_HIGH_B = 0x80,
    JOY_HIGH_Y = 0x40,
    JOY_HIGH_SELECT = 0x20,
    JOY_HIGH_START = 0x10,
    JOY_HIGH_UP = 0x08,
    JOY_HIGH_DOWN = 0x04,
    JOY_HIGH_LEFT = 0x02,
    JOY_HIGH_RIGHT = 0x01,
};

/* The accumulator holds a button mask. Afterwards it holds the masked buttons
 * that are held, and a held button with a pending press has that press
 * consumed. Returns whether a press was consumed (the zero flag is clear). */
static inline bool TakeButtonPress8(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                    uint8_t joypad_byte) {
    And8(cpu, DirectByte(memory, cpu, (uint8_t)(DP_BUTTONS_HELD + joypad_byte)));
    if (cpu->zero)
        return false;
    TestBitsDirect(memory, cpu, (uint8_t)(DP_BUTTONS_PRESSED + joypad_byte), 0);
    return !cpu->zero;
}

/* The same for a 16-bit mask over both bytes. */
static inline bool TakeButtonPress16(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    And16(cpu, Read16Direct(memory, cpu, DP_BUTTONS_HELD));
    if (cpu->zero)
        return false;
    TestBitsDirect(memory, cpu, DP_BUTTONS_PRESSED, 0);
    return !cpu->zero;
}

#endif
