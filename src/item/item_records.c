/* Item and spell records ($81:F1C5, $81:F414). */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/wram_view.h"
#include "lufia2/item.h"
#include "system/wram.h"

enum {
    ITEM_NAME_LENGTH = 12,
    ITEM_ID_MASK = 0x01ff,
    CHAR_END = 0x00,
    CHAR_SPACE = 0x20,
    ITEM_FIELDS_OFFSET = ITEM_NAME_LENGTH + 1, /* after the name and its end marker */
    ITEM_FIXED_BYTES = 9,
    ITEM_PRESENCE_BYTES = 4, /* one bit per optional record word */
    ITEM_PRESENCE_BITS = 8,
    ITEM_RECORD_TABLE_BASE = 0xcf69, /* low half of the table address */
    SPELL_ID_MASK = 0x00ff,
    SPELL_RECORD_TABLE_BASE = 0xfa5b,
    SPELL_NAME_LENGTH = 8,
    SPELL_TERMINATOR_OFFSET = SPELL_NAME_LENGTH,
    SPELL_PAYLOAD_BYTES = 11,
    SPELL_RECORD_END = SPELL_NAME_LENGTH + 1 + SPELL_PAYLOAD_BYTES,
};

/* Scratch bytes shared with other routines; only meaningful during a load. */
enum {
    SCRATCH_PRESENCE_MASK = 0x09fcu,
    SCRATCH_BITS_LEFT = 0x0a00u,
};

#define SYSTEM_BANK_9E 0x9eu
#define ITEM_RECORD_BANK 0x96u
#define SPELL_RECORD_BANK 0x95u
#define ROM_ITEM_NAME_TABLE 0x9ec7e8u   /* 12 characters per item */
#define ROM_ITEM_RECORD_TABLE 0x96cf69u /* one offset word per item */
#define ROM_SPELL_RECORD_TABLE 0x95fa5bu /* one offset word per spell */

static uint8_t ReadRecordByte(const Lufia2Memory *memory, uint8_t bank,
                              uint16_t offset) {
    return Read8(memory, ((uint32_t)bank << 16) | offset);
}

/* Where the original routine leaves the CPU is part of the contract the
 * callers were verified against; the record logic itself is in the helpers. */

/* Copies the item's name from the ROM table; returns the last word copied. */
static uint16_t CopyItemName(Lufia2Wram wram, uint16_t item) {
    const uint16_t name_offset = (uint16_t)(item * ITEM_NAME_LENGTH);
    uint16_t last_word = 0;
    uint16_t i;

    for (i = 0; i < ITEM_NAME_LENGTH; i += 2) {
        last_word = Read16Long(wram.memory,
            LongIndexedAddress(ROM_ITEM_NAME_TABLE, (uint16_t)(name_offset + i)));
        WramWrite16At(wram, WRAM_RECORD_BUFFER, i, last_word);
    }
    return last_word;
}

typedef struct ItemNameTrim {
    uint16_t cursor;    /* index the scan stopped at, 0xffff past the first byte */
    uint8_t last_char;  /* last character examined */
} ItemNameTrim;

/* Blanks the trailing spaces of the name: they become end markers. */
static ItemNameTrim TrimItemName(Lufia2Wram wram) {
    ItemNameTrim trim = {ITEM_NAME_LENGTH, 0};

    WramWriteAt(wram, WRAM_RECORD_BUFFER, ITEM_NAME_LENGTH, CHAR_END);
    for (trim.cursor = ITEM_NAME_LENGTH; trim.cursor != 0xffffu; --trim.cursor) {
        trim.last_char = WramReadAt(wram, WRAM_RECORD_BUFFER, trim.cursor);
        if (trim.last_char == CHAR_SPACE)
            WramWriteAt(wram, WRAM_RECORD_BUFFER, trim.cursor, CHAR_END);
        else if (trim.last_char != CHAR_END)
            break;
    }
    return trim;
}

/* The routine pushes the data bank and the item offset; nothing reads them
 * back, but the bytes stay in the stack page. */
static void LeaveItemNameStackBytes(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint16_t item) {
    const uint16_t scaled = (uint16_t)(item * 4u);

    Write8(memory, cpu->stack, cpu->data_bank);
    Write8(memory, (uint16_t)(cpu->stack - 1u), (uint8_t)(scaled >> 8));
    Write8(memory, (uint16_t)(cpu->stack - 2u), (uint8_t)scaled);
}

/* $81:F2A9: 12-character item name, trailing spaces cleared. */
static void ItemName(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewInBank(memory, cpu, SYSTEM_BANK_9E);
    const uint16_t item = WramRead16(wram, WRAM_ITEM_RECORD_ID) & ITEM_ID_MASK;
    const uint16_t last_word = CopyItemName(wram, item);
    const ItemNameTrim trim = TrimItemName(wram);

    LeaveItemNameStackBytes(memory, cpu, item);
    cpu->accumulator = (uint16_t)((last_word & 0xff00u) | trim.last_char);
    cpu->x = trim.cursor;
    cpu->y = (uint16_t)(item * ITEM_NAME_LENGTH + ITEM_NAME_LENGTH);
    /* The restored data bank sets N and Z; the last compare left C set unless
     * it met a control character; the offset sum cannot overflow. */
    SetNz8(cpu, cpu->data_bank);
    cpu->carry = trim.last_char == CHAR_END || trim.last_char >= CHAR_SPACE;
    cpu->overflow = 0;
    cpu->accumulator_is_8_bit = 1;
}

/* The record pointer is the table base plus the item's offset word. */
static uint16_t ReadItemRecordPointer(Lufia2Wram wram, uint16_t item) {
    const uint16_t offset = Read16Long(wram.memory,
        LongIndexedAddress(ROM_ITEM_RECORD_TABLE, (uint16_t)(item * 2u)));

    return (uint16_t)(offset + ITEM_RECORD_TABLE_BASE);
}

/* $81:F291: item record pointer, $96:CF69 table. */
static void ItemRecordAddress(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t item = WramRead16(wram, WRAM_ITEM_RECORD_ID) & ITEM_ID_MASK;
    const uint16_t pointer = ReadItemRecordPointer(wram, item);

    Write8(memory, cpu->stack, PackStatus(cpu));
    WramWrite16(wram, WRAM_ITEM_RECORD_POINTER, pointer);
    /* The status register is restored; A and X keep the last values. */
    cpu->accumulator = pointer;
    cpu->x = (uint16_t)(item * 2u);
    if (cpu->index_is_8_bit)
        cpu->x &= 0x00ffu;
}

/* The loaders start from an empty buffer. */
static void ClearRecordBuffer(Lufia2Wram wram) {
    uint16_t i;

    for (i = WRAM_RECORD_BUFFER_COUNT; i > 0; --i)
        WramWriteAt(wram, WRAM_RECORD_BUFFER, (uint16_t)(i - 1u), 0);
}

typedef struct ItemRecordLoad {
    uint16_t stream_end; /* record offset after the last byte read */
    uint8_t last_byte;   /* last byte moved, the bit count for an empty group */
} ItemRecordLoad;

/* Takes the lowest bit of a presence byte, leaving the rest shifted down. */
static bool TakePresenceBit(Lufia2Wram wram, unsigned byte) {
    const uint8_t bits = WramReadAt(wram, SCRATCH_PRESENCE_MASK, (uint16_t)byte);

    WramWriteAt(wram, SCRATCH_PRESENCE_MASK, (uint16_t)byte, (uint8_t)(bits >> 1));
    return (bits & 1u) != 0;
}

/* An item record is nine fixed bytes, four presence bytes and then the words
 * whose presence bit is set. Words keep their position in the buffer: bit n
 * fills the word at fixed bytes + 2n, absent words stay empty. */
static ItemRecordLoad LoadItemRecordFields(Lufia2Wram wram, uint16_t record) {
    ItemRecordLoad load = {record, 0};
    uint8_t value;
    uint16_t field = 0;
    unsigned group;
    unsigned i;

    for (i = 0; i < ITEM_FIXED_BYTES; ++i) {
        value = ReadRecordByte(wram.memory, ITEM_RECORD_BANK, load.stream_end++);
        WramWriteAt(wram, WRAM_RECORD_BUFFER, (uint16_t)(ITEM_FIELDS_OFFSET + i),
                    value);
    }
    field = ITEM_FIXED_BYTES;
    for (group = 0; group < ITEM_PRESENCE_BYTES; ++group) {
        value = ReadRecordByte(wram.memory, ITEM_RECORD_BANK,
                               (uint16_t)(load.stream_end + group));
        WramWriteAt(wram, SCRATCH_PRESENCE_MASK, (uint16_t)group, value);
    }
    load.stream_end = (uint16_t)(load.stream_end + ITEM_PRESENCE_BYTES);
    for (group = 0; group < ITEM_PRESENCE_BYTES; ++group) {
        unsigned bits_left = ITEM_PRESENCE_BITS;

        load.last_byte = ITEM_PRESENCE_BITS;
        WramWrite(wram, SCRATCH_BITS_LEFT, ITEM_PRESENCE_BITS);
        while (bits_left > 0) {
            if (TakePresenceBit(wram, group)) {
                for (i = 0; i < 2; ++i) {
                    load.last_byte = ReadRecordByte(wram.memory, ITEM_RECORD_BANK,
                                                    load.stream_end++);
                    WramWriteAt(wram, WRAM_RECORD_BUFFER,
                                (uint16_t)(ITEM_FIELDS_OFFSET + field + i),
                                load.last_byte);
                }
            }
            field = (uint16_t)(field + 2u);
            --bits_left;
            WramWrite(wram, SCRATCH_BITS_LEFT, (uint8_t)bits_left);
        }
    }
    return load;
}

/* $81:F1C5: name, 9 bytes, then masked words. */
Lufia2ExecutionResult Lufia2LoadItemRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t entry_status = PackStatus(cpu);
    ItemRecordLoad load;
    uint16_t record;

    Push8(memory, cpu, entry_status); /* F1C5 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    ClearRecordBuffer(WramViewOfCaller(memory, cpu));
    /* Flags and X as the clearing loop leaves them. */
    cpu->x = WRAM_RECORD_BUFFER;
    cpu->carry = 1;
    cpu->zero = 1;
    cpu->negative = 0;
    SimulateJslFrame(memory, cpu, 0x81u, 0xf1dau);
    ItemName(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SelectDataBank(memory, cpu, ITEM_RECORD_BANK); /* F1DB */
    SimulateJslFrame(memory, cpu, 0x81u, 0xf1e2u);
    ItemRecordAddress(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    record = WramRead16(WramViewOfCaller(memory, cpu), WRAM_ITEM_RECORD_POINTER);
    load = LoadItemRecordFields(WramViewOfCaller(memory, cpu), record);
    /* A keeps the pointer's high byte. */
    cpu->accumulator = (uint16_t)((record & 0xff00u) | load.last_byte);
    cpu->x =
        (uint16_t)(ITEM_FIXED_BYTES + 2u * ITEM_PRESENCE_BYTES * ITEM_PRESENCE_BITS);
    cpu->y = load.stream_end;
    PullDataBank(memory, cpu);                                 /* F28E */
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f290u);
}

/* The record pointer is the table base plus the spell's offset word. */
static uint16_t ReadSpellRecordOffset(Lufia2Wram wram, uint16_t spell) {
    return Read16Long(wram.memory,
        LongIndexedAddress(ROM_SPELL_RECORD_TABLE, (uint16_t)(spell * 2u)));
}

/* $81:F446: spell record pointer, $95:FA5B table. */
static void SpellRecordAddress(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t spell =
        WramRead16(wram, WRAM_MENU_SPELL_RECORD_ID) & SPELL_ID_MASK;
    const uint16_t offset = ReadSpellRecordOffset(wram, spell);

    /* The add leaves its pointer in A and N, Z, C and V as the routine exits. */
    cpu->accumulator = offset;
    cpu->carry = 0;
    Add16Value(cpu, SPELL_RECORD_TABLE_BASE);
    WramWrite16(wram, WRAM_SPELL_RECORD_POINTER, cpu->accumulator);
    cpu->x = (uint16_t)(spell * 2u);
    cpu->accumulator_is_8_bit = 1;
}

/* Spell name, the terminator the caller supplies, then the payload; returns
 * the last byte read. */
static uint8_t CopySpellRecord(Lufia2Wram wram, uint16_t record, uint8_t terminator) {
    uint8_t value = 0;
    unsigned i;

    for (i = 0; i < SPELL_NAME_LENGTH; ++i) {
        value = ReadRecordByte(wram.memory, SPELL_RECORD_BANK, (uint16_t)(record + i));
        WramWriteAt(wram, WRAM_RECORD_BUFFER, (uint16_t)i, value);
    }
    WramWriteAt(wram, WRAM_RECORD_BUFFER, SPELL_TERMINATOR_OFFSET, terminator);
    for (i = SPELL_NAME_LENGTH; i < SPELL_RECORD_END - 1; ++i) {
        value = ReadRecordByte(wram.memory, SPELL_RECORD_BANK, (uint16_t)(record + i));
        WramWriteAt(wram, WRAM_RECORD_BUFFER, (uint16_t)(i + 1u), value);
    }
    return value;
}

/* $81:F414: name, D low byte as terminator, 11 bytes. */
Lufia2ExecutionResult Lufia2LoadSpellRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint16_t record;
    uint8_t last_byte;

    PushDataBank(memory, cpu);                                 /* F414 */
    SelectDataBank(memory, cpu, SPELL_RECORD_BANK);
    SimulateJsrFrame(memory, cpu, 0xf41bu);
    SpellRecordAddress(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    record = WramRead16(WramViewOfCaller(memory, cpu), WRAM_SPELL_RECORD_POINTER);
    last_byte = CopySpellRecord(WramViewOfCaller(memory, cpu), record,
                                (uint8_t)cpu->direct_page);
    cpu->accumulator = (uint16_t)((cpu->direct_page & 0xff00u) | last_byte);
    cpu->x = SPELL_RECORD_END;
    cpu->y = (uint16_t)(record + SPELL_RECORD_END - 1u);
    cpu->carry = 1;
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81f445u);
}

/* $81:F194: A = first record byte of item $0A06. */
Lufia2ExecutionResult Lufia2ItemRecordByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint16_t record;
    uint8_t first_byte;

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    SimulateJslFrame(memory, cpu, 0x81u, 0xf19cu);
    ItemRecordAddress(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    record = WramRead16(WramViewOfCaller(memory, cpu), WRAM_ITEM_RECORD_POINTER);
    first_byte = ReadRecordByte(memory, ITEM_RECORD_BANK, record);
    PushDataBank(memory, cpu);
    SelectDataBank(memory, cpu, ITEM_RECORD_BANK);
    LoadA8(cpu, first_byte);
    cpu->y = record;
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f1aau);
}

/* Byte of the selected spell's record at the given offset. */
static Lufia2ExecutionResult SpellRecordByte(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu,
                                             uint16_t return_address, uint16_t offset,
                                             uint32_t rtl) {
    uint16_t record;
    uint8_t value;

    SimulateJsrFrame(memory, cpu, return_address);
    SpellRecordAddress(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    record = WramRead16(WramViewOfCaller(memory, cpu), WRAM_SPELL_RECORD_POINTER);
    value = ReadRecordByte(memory, SPELL_RECORD_BANK, (uint16_t)(record + offset));
    cpu->y = record;
    SetNz16(cpu, record);
    PushDataBank(memory, cpu);
    SelectDataBank(memory, cpu, SPELL_RECORD_BANK);
    LoadA8(cpu, value);
    PullDataBank(memory, cpu);
    return ExecutionReturned(rtl);
}

/* $81:F3F4: A = spell record byte $0C. */
Lufia2ExecutionResult Lufia2SpellRecordByteC(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return SpellRecordByte(memory, cpu, 0xf3f6u, 0x000cu, 0x81f403u);
}

/* $81:F404: A = spell record byte 8. */
Lufia2ExecutionResult Lufia2SpellRecordByte8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return SpellRecordByte(memory, cpu, 0xf406u, 0x0008u, 0x81f413u);
}

/* $81:F2A9: item name with trailing spaces cleared; RTL. */
Lufia2ExecutionResult Lufia2ItemNameTrimmed(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ItemName(memory, cpu);
    return ExecutionReturned(0x81f2e6u);
}

/* $81:F291: item text pointer, preserving P; RTL. */
Lufia2ExecutionResult Lufia2ItemTextPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ItemRecordAddress(memory, cpu);
    return ExecutionReturned(0x81f2a8u);
}

/* $81:F446: spell text pointer; RTS. */
Lufia2ExecutionResult Lufia2SpellTextPointer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SpellRecordAddress(memory, cpu);
    return ExecutionReturned(0x81f45du);
}
