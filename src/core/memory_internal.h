#ifndef LUFIA2_CORE_MEMORY_INTERNAL_H
#define LUFIA2_CORE_MEMORY_INTERNAL_H

/* Bus access and 65816 address modes. */

#include "lufia2/execution.h"

static inline uint8_t Read8(
    const Lufia2Memory *memory, uint32_t address) {
    return memory->read_byte(memory->context, address & 0x00ffffffu);
}

static inline void Write8(
    const Lufia2Memory *memory,
    uint32_t address,
    uint8_t value) {
    memory->write_byte(memory->context, address & 0x00ffffffu, value);
}

static inline uint32_t DirectAddress(
    const Lufia2CpuState *cpu, uint8_t offset) {
    return (uint16_t)(cpu->direct_page + offset);
}

/* Indexing carries into the next bank. */
static inline uint32_t AbsoluteIndexedAddress(
    const Lufia2CpuState *cpu, uint16_t address, uint16_t index) {
    return (((uint32_t)cpu->data_bank << 16) + address + index) &
           0x00ffffffu;
}

static inline uint32_t LongIndexedAddress(uint32_t address, uint16_t index) {
    return (address + index) & 0x00ffffffu;
}

static inline uint32_t ProgramAddress(
    const Lufia2CpuState *cpu, uint16_t address) {
    return ((uint32_t)cpu->program_bank << 16) | address;
}

static inline uint16_t Read16Direct(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint16_t address = (uint16_t)(cpu->direct_page + offset);
    return (uint16_t)(
        Read8(memory, address) |
        ((uint16_t)Read8(memory, (uint16_t)(address + 1u)) << 8));
}

static inline void Write16Direct(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset,
    uint16_t value) {
    const uint16_t address = (uint16_t)(cpu->direct_page + offset);
    Write8(memory, address, (uint8_t)value);
    Write8(memory, (uint16_t)(address + 1u), (uint8_t)(value >> 8));
}

static inline uint16_t Read16Long(
    const Lufia2Memory *memory, uint32_t address) {
    return (uint16_t)(
        Read8(memory, address) |
        ((uint16_t)Read8(memory, (address + 1u) & 0x00ffffffu) << 8));
}

static inline uint16_t Read16ProgramIndexed(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t base,
    uint16_t index) {
    const uint16_t address = (uint16_t)(base + index);
    return (uint16_t)(
        Read8(memory, ProgramAddress(cpu, address)) |
        ((uint16_t)Read8(
             memory, ProgramAddress(cpu, (uint16_t)(address + 1u))) << 8));
}


static inline uint16_t Read16AbsoluteIndexed(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t address,
    uint16_t index) {
    const uint32_t low = AbsoluteIndexedAddress(cpu, address, index);
    const uint32_t high = (low + 1u) & 0x00ffffffu;
    return (uint16_t)(
        Read8(memory, low) |
        ((uint16_t)Read8(memory, high) << 8));
}

static inline void Write16Long(
    const Lufia2Memory *memory,
    uint32_t address,
    uint16_t value) {
    Write8(memory, address, (uint8_t)value);
    Write8(memory, (address + 1u) & 0x00ffffffu, (uint8_t)(value >> 8));
}

/* 24-bit pointer at DP offset. */
static inline uint32_t DirectLongPointer(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    return Read16Direct(memory, cpu, offset) |
           ((uint32_t)Read8(memory, DirectAddress(cpu, (uint8_t)(offset + 2u)))
               << 16);
}

static inline void Write16Absolute(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t address,
    uint16_t value) {
    Write8(memory, AbsoluteIndexedAddress(cpu, address, 0), (uint8_t)value);
    Write8(memory, AbsoluteIndexedAddress(cpu, (uint16_t)(address + 1u), 0),
        (uint8_t)(value >> 8));
}

/* [dp],y address: 24-bit pointer plus Y. */
static inline uint32_t DirectLongIndirectY(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint32_t pointer =
        Read16Direct(memory, cpu, offset) |
        ((uint32_t)Read8(memory, DirectAddress(cpu, (uint8_t)(offset + 2u)))
            << 16);
    return (pointer + cpu->y) & 0x00ffffffu;
}

static inline uint8_t DirectByte(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    return Read8(memory, DirectAddress(cpu, offset));
}

static inline uint8_t AbsoluteByte(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t address,
    uint16_t index) {
    return Read8(memory, AbsoluteIndexedAddress(cpu, address, index));
}

/* dp,X: wraps inside bank 0. */
static inline uint32_t DirectIndexedAddress(
    const Lufia2CpuState *cpu, uint8_t offset, uint16_t index) {
    return (uint16_t)(cpu->direct_page + offset + index);
}

static inline uint16_t Read16DirectIndexed(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset,
    uint16_t index) {
    const uint16_t address = (uint16_t)DirectIndexedAddress(cpu, offset, index);

    return (uint16_t)(Read8(memory, address) |
        ((uint16_t)Read8(memory, (uint16_t)(address + 1u)) << 8));
}

/* [dp],Y: 24-bit pointer at dp, 16-bit read. */
static inline uint16_t Read16IndirectLongY(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint32_t pointer = Read16Direct(memory, cpu, offset) |
        ((uint32_t)DirectByte(memory, cpu, (uint8_t)(offset + 2u)) << 16);

    return Read16Long(memory, (pointer + cpu->y) & 0x00ffffffu);
}

static inline uint8_t Read8IndirectLongY(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint32_t pointer = Read16Direct(memory, cpu, offset) |
        ((uint32_t)DirectByte(memory, cpu, (uint8_t)(offset + 2u)) << 16);

    return Read8(memory, (pointer + cpu->y) & 0x00ffffffu);
}

/* 16-bit STX/STY to an MMIO pair. */
static inline void StoreWordAbsolute(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t address,
    uint16_t value) {
    Write8(memory, AbsoluteIndexedAddress(cpu, address, 0), (uint8_t)value);
    Write8(memory, AbsoluteIndexedAddress(cpu, (uint16_t)(address + 1u), 0),
        (uint8_t)(value >> 8));
}

/* Word at bank:address; the high byte wraps within the bank. */
static inline uint16_t Read16Bank(
    const Lufia2Memory *memory, uint8_t bank, uint16_t address) {
    const uint32_t base = (uint32_t)bank << 16;
    const uint8_t low = Read8(memory, base | address);
    const uint8_t high = Read8(memory, base | (uint16_t)(address + 1u));

    return (uint16_t)(low | (high << 8));
}

#endif
