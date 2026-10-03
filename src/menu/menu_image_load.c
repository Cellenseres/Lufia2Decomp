/* Menu image loaders of bank $86: copy rows of an image into the work
 * buffer at $7E:6000 through the RAM block move, queue the buffer for the
 * video upload, and copy the palette blocks of bank $9F. */

#include <stdbool.h>

#include "core/cpu_ops.h"
#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/menu.h"
#include "lufia2/system.h"

/* Direct page: the row being copied, and the image set being walked. */
enum {
    ROW_SOURCE = 0x08u,
    ROW_BANK = 0x0au,
    ROW_TARGET = 0x0bu,
    SET_INDEX = 0x04u,
    IMAGE_BASE = 0x19u,
    IMAGE_BANK = 0x1bu,
    SET_TARGET = 0x1cu,
    SLOT_TARGET = 0x02u,
    SLOT_INDEX = 0x04u,
    SLOTS_LEFT = 0x15u
};

/* Direct page: the video transfer request, and the words that the transfer
 * routine at $82:8067 takes from it. */
enum {
    STAGE_SOURCE = 0x5du,
    STAGE_BANK = 0x5fu,
    STAGE_TARGET = 0x60u,
    STAGE_SIZE = 0x58u,
    TRANSFER_ADDRESS = 0x79u,
    TRANSFER_START = 0x75u,
    TRANSFER_START_VALUE = 0x41u,
    TRANSFER_PORT = 0x18u,
    TRANSFER_MODE = 0x01u,
    VIDEO_TARGET = 0x3000u
};

/* The block move stub in work RAM, and the buffer the rows are copied to. */
enum {
    MOVE_DESTINATION = 0x057eu,
    MOVE_SOURCE = 0x057fu,
    BUFFER = 0x6000u,
    BUFFER_BANK = 0x7eu
};

/* The list of entries to load: its length, then one byte per entry. */
enum {
    LIST_COUNT = 0x0a7au,
    LIST_ENTRIES = 0x0a7bu
};

/* Tables in ROM: seven grid rows (source word, bank byte), the image table
 * of the set loader (the same layout), and the class table of the slot
 * palettes. */
enum {
    GRID_TABLE = 0x8690abu,
    GRID_ENTRY_SIZE = 3u,
    GRID_END = 0x15u,
    IMAGE_TABLE = 0x8ee5c2u,
    CLASS_TABLE = 0x869165u
};

/* Palette blocks of bank $9F go to bank $00, slot palettes come from $97. */
enum {
    PALETTE_DESTINATION_BANK = 0x00u,
    PALETTE_SOURCE_BANK = 0x9fu,
    SLOT_SOURCE_BANK = 0x97u,
    SLOT_PALETTES = 0x04a0u,
    SLOT_BLOCK_SIZE = 0x20u,
    SLOT_BLOCKS = 0xccf8u
};

typedef Lufia2ExecutionResult (*Subroutine)(
    const Lufia2Memory *memory, Lufia2CpuState *cpu);

/* JSR into a routine of this file; a result that is not a plain return is
 * passed on. */
static bool CallSubroutine(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Subroutine routine, uint16_t last_byte, Lufia2ExecutionResult *result) {
    SimulateJsrFrame(memory, cpu, last_byte);
    *result = routine(memory, cpu);
    if (result->flow != LUFIA2_EXECUTION_RETURNED)
        return false;
    SimulateRtsFrame(memory, cpu);
    return true;
}

/* JSR $057D: copies count + 1 bytes from the offset `source` to the offset
 * `target` of the banks set in the stub. */
static bool MoveBytes(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t source, uint16_t target, uint16_t count, uint16_t last_byte,
    Lufia2ExecutionResult *result) {
    SetAccumulatorWidth(cpu, 0);
    cpu->x = source;
    cpu->y = target;
    LoadA16(cpu, count);
    return CallSubroutine(memory, cpu, Lufia2RamBlockMove, last_byte, result);
}

static void StepDirect8(Lufia2Wram wram, uint32_t location) {
    WramWrite(wram, location, (uint8_t)(WramRead(wram, location) + 1u));
}

/* $86:9009: one 256-byte row from the image bank at $1B to $7E:$0B. */
Lufia2ExecutionResult Lufia2MenuCopyImageRow256(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ExecutionResult result;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x869009u);
    WramWrite(wram, MOVE_DESTINATION, BUFFER_BANK);
    WramWrite(wram, MOVE_SOURCE, WramRead(wram, IMAGE_BANK));
    if (!MoveBytes(memory, cpu, WramRead16(wram, ROW_SOURCE),
            WramRead16(wram, ROW_TARGET), 0x00ffu, 0x901eu, &result))
        return result;
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x869021u);
}

/* $86:8FF6: two consecutive rows, the second one line further on. */
Lufia2ExecutionResult Lufia2MenuCopyImageBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ExecutionResult result;

    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868ff6u);
    SetAccumulatorWidth(cpu, 1);
    PushDataBank(memory, cpu);
    if (!CallSubroutine(memory, cpu, Lufia2MenuCopyImageRow256, 0x8ffbu,
            &result))
        return result;
    StepDirect8(wram, ROW_SOURCE + 1u);
    StepDirect8(wram, ROW_TARGET + 1u);
    StepDirect8(wram, ROW_TARGET + 1u);
    if (!CallSubroutine(memory, cpu, Lufia2MenuCopyImageRow256, 0x9004u,
            &result))
        return result;
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    return ExecutionReturned(0x869008u);
}

/* $86:906A: one 128-byte row from bank $0A to $7E:$0B; both positions then
 * move on, the source by $80 and the target by $200. */
Lufia2ExecutionResult Lufia2MenuCopyImageRow128(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ExecutionResult result;
    Word16Result source;
    Word16Result target;

    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86906au);
    WramWrite(wram, MOVE_DESTINATION, BUFFER_BANK);
    WramWrite(wram, MOVE_SOURCE, WramRead(wram, ROW_BANK));
    if (!MoveBytes(memory, cpu, WramRead16(wram, ROW_SOURCE),
            WramRead16(wram, ROW_TARGET), 0x007fu, 0x9081u, &result))
        return result;
    source = Sum16(WramRead16(wram, ROW_SOURCE), 0x0080u, false);
    WramWrite16(wram, ROW_SOURCE, source.value);
    target = Sum16(WramRead16(wram, ROW_TARGET), 0x0200u, false);
    WramWrite16(wram, ROW_TARGET, target.value);
    LeaveSum(cpu, target);
    return ExecutionReturned(0x869092u);
}

/* Stages the buffer for the video upload and waits for it; the wait is
 * left to the original code with the returns of the callers pushed. */
static Lufia2ExecutionResult StageBuffer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t size,
    uint16_t return_address) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    WramWrite16(wram, STAGE_SOURCE, BUFFER);
    WramWrite(wram, STAGE_BANK, BUFFER_BANK);
    WramWrite16(wram, STAGE_TARGET, VIDEO_TARGET);
    WramWrite16(wram, STAGE_SIZE, size);
    cpu->x = size;
    SimulateJslFrame(memory, cpu, 0x86u, return_address);
    return Lufia2MenuQueueVideoWrite(memory, cpu);
}

/* $86:9022: seven image rows of 128 bytes from the table at $86:90AB. */
Lufia2ExecutionResult Lufia2MenuLoadImageGrid(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ExecutionResult result;
    uint16_t entry = 0;

    if (cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x869022u);
    SetAccumulatorWidth(cpu, 0);
    WramWrite16(wram, ROW_TARGET, BUFFER);
    do {
        Word16Result back;

        WramWrite(wram, ROW_BANK,
            Read8(memory, LongIndexedAddress(GRID_TABLE + 2u, entry)));
        WramWrite16(wram, ROW_SOURCE,
            Read16Long(memory, LongIndexedAddress(GRID_TABLE, entry)));
        PushStackWord(memory, cpu, entry);
        PushDataBank(memory, cpu);
        if (!CallSubroutine(memory, cpu, Lufia2MenuCopyImageRow128, 0x9040u,
                &result) ||
            !CallSubroutine(memory, cpu, Lufia2MenuCopyImageRow128, 0x9043u,
                &result))
            return result;
        back = Difference16(WramRead16(wram, ROW_TARGET), 0x0380u);
        WramWrite16(wram, ROW_TARGET, back.value);
        if (!CallSubroutine(memory, cpu, Lufia2MenuCopyImageRow128, 0x904eu,
                &result) ||
            !CallSubroutine(memory, cpu, Lufia2MenuCopyImageRow128, 0x9051u,
                &result))
            return result;
        back = Difference16(WramRead16(wram, ROW_TARGET), 0x0080u);
        WramWrite16(wram, ROW_TARGET, back.value);
        PullDataBank(memory, cpu);
        entry = (uint16_t)(PullStackWord(memory, cpu) + GRID_ENTRY_SIZE);
        LeaveSum(cpu, back);
        LeaveComparison(cpu, entry, GRID_END);
    } while (entry != GRID_END);
    SetAccumulatorWidth(cpu, 1);
    SimulateJsrFrame(memory, cpu, 0x9068u);
    return StageBuffer(memory, cpu, 0x1c00u, 0x90a9u);
}

/* Sets the source and target of the next block to the image base and the
 * set target, each moved on by a step. */
static void SetBlockPositions(Lufia2CpuState *cpu, Lufia2Wram wram,
    uint16_t source_step, uint16_t target_step) {
    const uint16_t source_base = WramRead16(wram, IMAGE_BASE);
    const uint16_t source = source_step == 0 ? source_base :
        Sum16Mode(source_base, source_step, false, cpu->decimal).value;
    Word16Result target;

    WramWrite16(wram, ROW_SOURCE, source);
    target.value = WramRead16(wram, SET_TARGET);
    if (target_step != 0)
        target = Sum16Mode(target.value, target_step, false, cpu->decimal);
    WramWrite16(wram, ROW_TARGET, target.value);
    if (target_step != 0)
        SetSumFlags(cpu, target);
}

/* $86:8F6F: three image blocks per entry of the list at $0A7B, taken from
 * the image table at $8E:E5C2, then the upload of the whole buffer. M8/X16. */
Lufia2ExecutionResult Lufia2MenuLoadImageSet(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    Lufia2ExecutionResult result;
    uint8_t done;
    uint8_t total;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868f6fu);
    WramWrite16(wram, SET_TARGET, BUFFER);
    WramWrite(wram, SET_INDEX, 0);
    WramWrite(wram, SET_INDEX + 1u, 0);
    do {
        /* Three table bytes per image: the offset is a byte quantity. */
        const uint16_t list_index = WramRead16(wram, SET_INDEX);
        const uint8_t doubled = (uint8_t)(2u * WramReadAt(wram, LIST_ENTRIES, list_index));
        const uint8_t image_entry = Sum8Mode(doubled,
            WramReadAt(wram, LIST_ENTRIES, list_index), false, cpu->decimal).value;
        Word16Result image;
        Word16Result next_target;

        WramWrite(wram, IMAGE_BANK,
            Read8(memory, LongIndexedAddress(IMAGE_TABLE + 2u, image_entry)));
        image = Sum16Mode(Read16Long(memory,
                          LongIndexedAddress(IMAGE_TABLE, image_entry)),
            0x0200u, false, cpu->decimal);
        WramWrite16(wram, IMAGE_BASE, image.value);
        SetBlockPositions(cpu, wram, 0, 0);
        SetSumFlags(cpu, image);
        if (!CallSubroutine(memory, cpu, Lufia2MenuCopyImageBlock, 0x8fa4u,
                &result))
            return result;
        SetBlockPositions(cpu, wram, 0x0400u, 0x0100u);
        if (!CallSubroutine(memory, cpu, Lufia2MenuCopyImageBlock, 0x8fb7u,
                &result))
            return result;
        SetBlockPositions(cpu, wram, 0x0a00u, 0x0400u);
        if (!CallSubroutine(memory, cpu, Lufia2MenuCopyImageBlock, 0x8fcau,
                &result))
            return result;
        next_target = Sum16Mode(WramRead16(wram, SET_TARGET), 0x0800u,
            false, cpu->decimal);
        WramWrite16(wram, SET_TARGET, next_target.value);
        StepDirect8(wram, SET_INDEX);
        done = WramRead(wram, SET_INDEX);
        total = WramRead(wram, LIST_COUNT);
        cpu->accumulator = (uint16_t)((next_target.value & 0xff00u) | done);
        cpu->overflow = next_target.overflow;
        Compare8(cpu, done, total);
    } while (done != total);
    SetAccumulatorWidth(cpu, 1);
    return StageBuffer(memory, cpu, 0x2000u, 0x8ff4u);
}

/* The palette blocks of bank $9F copied to $0320 and up. Each one runs
 * MVN with the destination bank $00. */
static Lufia2ExecutionResult CopyPalette(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t source,
    uint16_t target,
    uint16_t count,
    uint32_t return_address) {
    SetAccumulatorWidth(cpu, 0);
    PushDataBank(memory, cpu);
    cpu->x = source;
    cpu->y = target;
    cpu->accumulator = count;
    OpMoveNext(memory, cpu, PALETTE_DESTINATION_BANK, PALETTE_SOURCE_BANK);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(return_address);
}

#define PALETTE(name, entry, source, target, count)                           \
    Lufia2ExecutionResult name(const Lufia2Memory *memory,                    \
        Lufia2CpuState *cpu) {                                                \
        if (cpu->index_is_8_bit)                                              \
            return ExecutionHandoff(cpu, entry);                              \
        return CopyPalette(memory, cpu, source, target, count, (entry) + 0x12u); \
    }

PALETTE(Lufia2MenuLoadPalette0, 0x8690c0u, 0x8200u, 0x0320u, 0x003fu)
PALETTE(Lufia2MenuLoadPalette1, 0x8690d3u, 0x8420u, 0x0340u, 0x001fu)
PALETTE(Lufia2MenuLoadPalette2, 0x8690e6u, 0x8040u, 0x0360u, 0x00bfu)
PALETTE(Lufia2MenuLoadPalette3, 0x8690f9u, 0x8100u, 0x0420u, 0x007fu)
PALETTE(Lufia2MenuLoadPalette4, 0x86910cu, 0x8140u, 0x04c0u, 0x001fu)

/* $86:911F: one 32-byte block of bank $97 per entry of the list at $0A7B,
 * chosen by the class table at $86:9165, stored from $04A0 on. X16. */
Lufia2ExecutionResult Lufia2MenuLoadSlotPalettes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t remaining;

    if (cpu->index_is_8_bit || !DirectWorkWordAvailable(cpu, SLOTS_LEFT))
        return ExecutionHandoff(cpu, 0x86911fu);
    SetAccumulatorWidth(cpu, 0);
    WramWrite16(wram, SLOT_TARGET, SLOT_PALETTES);
    WramWrite16(wram, SLOTS_LEFT, WramRead(wram, LIST_COUNT));
    WramWrite16(wram, SLOT_INDEX, 0);
    do {
        const uint8_t entry = (uint8_t)WramRead16At(WramViewOfCaller(memory, cpu), LIST_ENTRIES,
            WramRead16(wram, SLOT_INDEX));
        uint8_t block;
        Word16Result next;

        PushDataBank(memory, cpu);
        block = Read8(memory, LongIndexedAddress(CLASS_TABLE, entry));
        cpu->x = Sum16Mode((uint16_t)(block * SLOT_BLOCK_SIZE), SLOT_BLOCKS,
            false, cpu->decimal).value;
        cpu->y = WramRead16(wram, SLOT_TARGET);
        cpu->accumulator = SLOT_BLOCK_SIZE - 1u;
        OpMoveNext(memory, cpu, PALETTE_DESTINATION_BANK, SLOT_SOURCE_BANK);
        PullDataBank(memory, cpu);
        next = Sum16Mode(WramRead16(wram, SLOT_TARGET), SLOT_BLOCK_SIZE,
            false, cpu->decimal);
        WramWrite16(wram, SLOT_TARGET, next.value);
        (void)WramStep16(wram, SLOT_INDEX, 1);
        remaining = WramStep16(wram, SLOTS_LEFT, -1);
        LeaveSum(cpu, next);
        LeaveCounter(cpu, remaining);
    } while (remaining != 0);
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x869164u);
}

/* $82:8044: sets up the video transfer of $58 bytes from the long address
 * $5D to the VRAM address $60 and waits for the frame that performs it.
 * The wait is left to the original code. M8/X16, JSL. */
Lufia2ExecutionResult Lufia2MenuQueueVideoWrite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t video_address;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x828044u);
    WramWrite(wram, SNES_DMAP(0), TRANSFER_MODE);
    WramWrite16(wram, SNES_A1TL(0), WramRead16(wram, STAGE_SOURCE));
    WramWrite(wram, SNES_A1B(0), WramRead(wram, STAGE_BANK));
    WramWrite16(wram, SNES_DASL(0), WramRead16(wram, STAGE_SIZE));
    WramWrite(wram, SNES_BBAD(0), TRANSFER_PORT);
    video_address = WramRead16(wram, STAGE_TARGET);
    WramWrite16(wram, TRANSFER_ADDRESS, video_address);
    WramWrite(wram, TRANSFER_START, TRANSFER_START_VALUE);
    cpu->x = video_address;
    LoadA8(cpu, TRANSFER_START_VALUE);
    SimulateJsrFrame(memory, cpu, 0x8067u);
    cpu->program_bank = 0x82u;
    return ExecutionHandoff(cpu, 0x8293c2u);
}
