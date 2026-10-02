/* Original save-file loading, packing and byte-stream encryption. */

#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/wram_view.h"
#include "lufia2/system.h"
#include "system/wram.h"

enum {
    SAVE_SELECTED_FILE = 0x65u,
    SAVE_FILE_INDEX = 0x54u,
    SAVE_FILE_POINTER = 0x60u,
    SAVE_FILE_POINTER_BANK = 0x62u,
    SAVE_CHECKSUM_POINTER = 0x5du,
    SAVE_CHECKSUM_POINTER_BANK = 0x5fu,
    SAVE_CHECKSUM = 0x56u,
};

/* Save RAM holds one 2 KiB slot per file: a header (state byte, random seed,
 * checksum word) followed by the encrypted game data. */
enum {
    SAVE_SRAM_BANK = 0x70u,
    SAVE_SLOT_SHIFT = 11,
    SAVE_SLOT_SIZE = 0x800u,
    SAVE_SEED_OFFSET = 1u,
    SAVE_CHECKSUM_OFFSET = 2u,
    SAVE_HEADER_SIZE = 4u,
    SAVE_CHECKSUM_SEED = 0x6502u,
    SAVE_RAM_STATUS = 0x700000u
};

static void SavePushA(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
}

static void SavePullA(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

static Lufia2ExecutionResult SaveUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);
    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

static uint8_t SaveChild(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context,
    uint32_t site, uint32_t target, uint8_t frame) {
    return CallChildWithFrame(
        memory, cpu, child, context, site, target, frame, cpu->program_bank);
}

#define SAVE_CALL(site, target, frame) \
    do { \
        if (!SaveChild(memory, cpu, child, context, site, target, frame)) \
            return SaveUnwound(site); \
    } while (0)

Lufia2ExecutionResult Lufia2LoadGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x809099u);
    if (checkpoint)
        checkpoint(context, cpu, 0x809099u);
    cpu->carry = 0;
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x30u);
    SavePushA(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, SAVE_SELECTED_FILE));
    SAVE_CALL(0x8090a5u, 0x80914bu, 3u);
    OpLda(memory, cpu, WRAM_SAVE_FILE_BUFFER);
    OpAndValue(cpu, 0xf0u);
    if (cpu->zero) {
        SAVE_CALL(0x8090b1u, 0x8eb993u, 3u);
        SAVE_CALL(0x8090b5u, 0x85c60eu, 3u);
    } else {
        OpLda(memory, cpu, OpStack(cpu, 8u));
        OpOraValue(cpu, 1u);
        OpSta(memory, cpu, OpStack(cpu, 8u));
    }
    OpRepWidths(cpu, 0x30u);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    SavePullA(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8090c8u);
}

Lufia2ExecutionResult Lufia2SaveGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    if (!child || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8090c9u);
    if (checkpoint)
        checkpoint(context, cpu, 0x8090c9u);
    SavePushA(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    SavePushA(memory, cpu);
    SAVE_CALL(0x8090d3u, 0x8eba0du, 3u);
    SAVE_CALL(0x8090d7u, 0x85c954u, 3u);
    SavePullA(memory, cpu);
    SAVE_CALL(0x8090dcu, 0x809184u, 2u);
    UnpackStatus(cpu, Pull8(memory, cpu));
    PullDataBank(memory, cpu);
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    SavePullA(memory, cpu);
    return ExecutionReturned(0x8090e4u);
}

/* Byte offset of save slot A (8-bit accumulator) in save RAM, returned in A
 * and X. */
Lufia2ExecutionResult Lufia2ResolveSaveFileAddress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint8_t slot = A8(cpu);

    (void)memory;
    /* The last bit shifted out of the slot number ends up in the carry. */
    cpu->carry = (uint16_t)(slot << (SAVE_SLOT_SHIFT - 1)) >> 15;
    cpu->accumulator = (uint16_t)(slot << SAVE_SLOT_SHIFT);
    TransferAToX(cpu);
    return ExecutionReturned(0x8091deu);
}

static uint32_t SaveByteAddress(uint16_t slot_offset, uint16_t index) {
    return ((((uint32_t)SAVE_SRAM_BANK << 16) | slot_offset) + index) & 0x00ffffffu;
}

/* Signed overflow of a - b. */
static int SubtractOverflows(uint8_t a, uint8_t b) {
    const uint8_t difference = (uint8_t)(a - b);

    return ((a ^ b) & (a ^ difference) & 0x80u) != 0;
}

/* Signed overflow of a + b. */
static int AddOverflows(uint8_t a, uint8_t b) {
    const uint8_t sum = (uint8_t)(a + b);

    return (~(a ^ b) & (a ^ sum) & 0x80u) != 0;
}

/* Checksum of the slot named by A: the words after the header added onto a
 * fixed seed (the carry is dropped); stored at $56. Needs M8, X16; the direct
 * page must lie in work RAM. */
Lufia2ExecutionResult Lufia2SaveFileChecksum(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t slot_offset, sum = SAVE_CHECKSUM_SEED, index;
    int overflow = 0;

    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8090fcu);
    SAVE_CALL(0x8090fcu, 0x8091d3u, 2u);
    slot_offset = cpu->x;
    WramWrite(wram, SAVE_CHECKSUM_POINTER_BANK, SAVE_SRAM_BANK);
    WramWrite16(wram, SAVE_CHECKSUM_POINTER, slot_offset);
    for (index = SAVE_HEADER_SIZE; index != SAVE_SLOT_SIZE; index += 2u) {
        const uint16_t word = Read16Long(memory, SaveByteAddress(slot_offset, index));
        const uint32_t total = (uint32_t)sum + word;

        overflow = ((~(sum ^ word) & (sum ^ total)) & 0x8000u) != 0;
        sum = (uint16_t)total;
    }
    WramWrite16(wram, SAVE_CHECKSUM, sum);
    /* Exit: A = checksum, Y = end of slot, flags of the last add and of the
     * loop's final comparison. */
    cpu->accumulator = sum;
    cpu->y = SAVE_SLOT_SIZE;
    cpu->carry = 1;
    cpu->zero = 1;
    cpu->negative = 0;
    cpu->overflow = (uint8_t)overflow;
    return ExecutionReturned(0x80911bu);
}

/* Reads save slot A into the file buffer, decrypting it with the random
 * stream seeded from the slot's second byte. Needs M8, X16; the direct page
 * must lie in work RAM. */
Lufia2ExecutionResult Lufia2ReadGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, Lufia2ExecutionCheckpoint checkpoint,
    void *context) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t slot_offset, index;
    uint8_t seed;

    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x80914bu);
    if (checkpoint)
        checkpoint(context, cpu, 0x80914bu);
    WramWrite(wram, SAVE_FILE_INDEX, A8(cpu));
    SAVE_CALL(0x80914du, 0x8091d3u, 2u);
    slot_offset = cpu->x;
    WramWrite16(wram, SAVE_FILE_POINTER, slot_offset);
    WramWrite(wram, SAVE_FILE_POINTER_BANK, SAVE_SRAM_BANK);
    LoadY16(cpu, 1u);
    seed = Read8(memory, SaveByteAddress(slot_offset, SAVE_SEED_OFFSET));
    LoadA8(cpu, seed);
    WramWrite(wram, WRAM_RANDOM_SEED_WORK, seed);
    SAVE_CALL(0x80915eu, 0x8082e7u, 3u);
    for (index = SAVE_HEADER_SIZE; index != SAVE_SLOT_SIZE; ++index) {
        uint8_t stream, stored;

        cpu->x = index;
        cpu->y = index;
        LoadA8(cpu, 0xffu);
        SAVE_CALL(0x809168u, 0x808299u, 3u);
        stream = A8(cpu);
        stored = Read8(memory, SaveByteAddress(slot_offset, index));
        Write8(memory, WRAM_SAVE_FILE_BUFFER + index, (uint8_t)(stored - stream));
        cpu->overflow = (uint8_t)SubtractOverflows(stream, stored);
        cpu->x = (uint16_t)(index + 1u);
        cpu->y = cpu->x;
        Compare16(cpu, cpu->y, SAVE_SLOT_SIZE);
    }
    /* The first byte is stored in the clear. */
    LoadA8(cpu, Read8(memory, SaveByteAddress(slot_offset, 0u)));
    Write8(memory, WRAM_SAVE_FILE_BUFFER, A8(cpu));
    return ExecutionReturned(0x809183u);
}

/* Writes the file buffer to save slot A, encrypted with the random stream,
 * then stores the checksum. Needs M8, X16; the direct page must lie in work
 * RAM. */
Lufia2ExecutionResult Lufia2WriteGameFile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t slot_offset, index, checksum;
    uint8_t seed, status;

    if (!child || cpu->decimal || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x809184u);
    WramWrite(wram, SAVE_FILE_INDEX, A8(cpu));
    SAVE_CALL(0x809186u, 0x8091d3u, 2u);
    slot_offset = cpu->x;
    WramWrite16(wram, SAVE_FILE_POINTER, slot_offset);
    LoadA8(cpu, SAVE_SRAM_BANK);
    WramWrite(wram, SAVE_FILE_POINTER_BANK, SAVE_SRAM_BANK);
    LoadY16(cpu, 1u);
    SAVE_CALL(0x809192u, 0x808299u, 3u);
    seed = A8(cpu);
    Write8(memory, SaveByteAddress(slot_offset, SAVE_SEED_OFFSET), seed);
    WramWrite(wram, WRAM_RANDOM_SEED_WORK, seed);
    SAVE_CALL(0x80919bu, 0x8082e7u, 3u);
    for (index = SAVE_HEADER_SIZE; index != SAVE_SLOT_SIZE; ++index) {
        uint8_t stream, plain, sum;

        cpu->x = index;
        cpu->y = index;
        LoadA8(cpu, 0xffu);
        SAVE_CALL(0x8091a5u, 0x808299u, 3u);
        stream = A8(cpu);
        plain = Read8(memory, WRAM_SAVE_FILE_BUFFER + index);
        sum = (uint8_t)(stream + plain);
        Write8(memory, SaveByteAddress(slot_offset, index), sum);
        cpu->overflow = (uint8_t)AddOverflows(stream, plain);
        cpu->x = (uint16_t)(index + 1u);
        cpu->y = cpu->x;
        Compare16(cpu, cpu->y, SAVE_SLOT_SIZE);
    }
    LoadA8(cpu, WramRead(wram, SAVE_FILE_INDEX));
    SAVE_CALL(0x8091b9u, 0x8090fcu, 2u);
    checksum = WramRead16(wram, SAVE_CHECKSUM);
    Write16Long(memory, SaveByteAddress(slot_offset, SAVE_CHECKSUM_OFFSET), checksum);
    /* The first byte carries the low nibble of the save RAM's status byte. */
    status = Read8(memory, SAVE_RAM_STATUS) & 0x0fu;
    Write8(memory, SaveByteAddress(slot_offset, 0u), status);
    cpu->accumulator = (uint16_t)((checksum & 0xff00u) | status);
    LoadY16(cpu, 0u);
    cpu->negative = 0;
    cpu->zero = status == 0;
    return ExecutionReturned(0x8091d2u);
}
