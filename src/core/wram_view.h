#ifndef LUFIA2_CORE_WRAM_VIEW_H
#define LUFIA2_CORE_WRAM_VIEW_H

/* Named access to the catalogued work-RAM locations. */

#include "core/memory_internal.h"

/* A view fixes the data bank and direct page that the original code ran with,
 * so a catalogued location resolves to the same bus address it did there.
 * Nothing is cached: every accessor is one bus access per byte, low byte
 * first. */
typedef struct Lufia2Wram {
    const Lufia2Memory *memory;
    uint8_t data_bank;
    uint16_t direct_page;
} Lufia2Wram;

/* System bank whose low half mirrors the first 8 KiB of work RAM. */
#define WRAM_MIRROR_BANK 0x80u

/* The view of a routine that keeps its caller's banks. */
static inline Lufia2Wram WramViewOfCaller(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu) {
    Lufia2Wram wram;

    wram.memory = memory;
    wram.data_bank = cpu->data_bank;
    wram.direct_page = cpu->direct_page;
    return wram;
}

/* Low work RAM reached by long addresses in bank $00; the direct page does not
 * apply. */
static inline Lufia2Wram WramViewLong(const Lufia2Memory *memory) {
    Lufia2Wram wram;

    wram.memory = memory;
    wram.data_bank = 0;
    wram.direct_page = 0;
    return wram;
}

/* The view of a routine that loaded its own data bank. */
static inline Lufia2Wram WramViewInBank(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint8_t data_bank) {
    Lufia2Wram wram;

    wram.memory = memory;
    wram.data_bank = data_bank;
    wram.direct_page = cpu->direct_page;
    return wram;
}

/* A location below $100 is a direct-page offset, below $10000 an absolute
 * address in the data bank, anything else a long address. Indexing wraps like
 * the matching 65816 addressing mode. */
static inline uint32_t WramAddress(
    Lufia2Wram wram, uint32_t location, uint16_t index) {
    if (location < 0x100u)
        return (uint16_t)(wram.direct_page + location + index);
    if (location < 0x10000u)
        return (((uint32_t)wram.data_bank << 16) + location + index) &
               0x00ffffffu;
    return (location + index) & 0x00ffffffu;
}

static inline uint32_t WramNextAddress(
    uint32_t location, uint32_t address) {
    if (location < 0x100u)
        return (uint16_t)(address + 1u);
    return (address + 1u) & 0x00ffffffu;
}

static inline uint8_t WramReadAt(
    Lufia2Wram wram, uint32_t location, uint16_t index) {
    return Read8(wram.memory, WramAddress(wram, location, index));
}

static inline void WramWriteAt(
    Lufia2Wram wram, uint32_t location, uint16_t index, uint8_t value) {
    Write8(wram.memory, WramAddress(wram, location, index), value);
}

static inline uint16_t WramRead16At(
    Lufia2Wram wram, uint32_t location, uint16_t index) {
    const uint32_t low = WramAddress(wram, location, index);
    const uint8_t low_byte = Read8(wram.memory, low);
    const uint8_t high_byte =
        Read8(wram.memory, WramNextAddress(location, low));

    return (uint16_t)(low_byte | ((uint16_t)high_byte << 8));
}

static inline void WramWrite16At(
    Lufia2Wram wram, uint32_t location, uint16_t index, uint16_t value) {
    const uint32_t low = WramAddress(wram, location, index);

    Write8(wram.memory, low, (uint8_t)value);
    Write8(wram.memory, WramNextAddress(location, low), (uint8_t)(value >> 8));
}

static inline uint8_t WramRead(Lufia2Wram wram, uint32_t location) {
    return WramReadAt(wram, location, 0);
}

static inline void WramWrite(
    Lufia2Wram wram, uint32_t location, uint8_t value) {
    WramWriteAt(wram, location, 0, value);
}

static inline uint16_t WramRead16(Lufia2Wram wram, uint32_t location) {
    return WramRead16At(wram, location, 0);
}

static inline void WramWrite16(
    Lufia2Wram wram, uint32_t location, uint16_t value) {
    WramWrite16At(wram, location, 0, value);
}

#endif
