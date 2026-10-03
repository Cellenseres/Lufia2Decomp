/* Item and spell records ($81:F1C5, $81:F414). */

#include "core/cpu_internal.h"
#include "core/wram_view.h"
#include "lufia2/item.h"
#include "system/wram.h"

enum {
    MASKS = 0x09fcu,                    /* 4 bytes: which words follow */
    MASK_BITS = 0x0a00u,
    FIELDS = 0x0b84u,
    ITEM_NAME_LENGTH = 12,
    ITEM_ID_MASK = 0x01ff,
    CHAR_END = 0x00,
    CHAR_SPACE = 0x20,
    ITEM_RECORD_TABLE_BASE = 0xcf69,    /* low half of the table address */
    SPELL_ID_MASK = 0x00ff,
    SPELL_RECORD_TABLE_BASE = 0xfa5b,
};

#define SYSTEM_BANK_9E 0x9eu
#define ROM_ITEM_NAME_TABLE 0x9ec7e8u   /* 12 characters per item */
#define ROM_ITEM_RECORD_TABLE 0x96cf69u /* one offset word per item */
#define ROM_SPELL_RECORD_TABLE 0x95fa5bu /* one offset word per spell */

static void DecrementX(Lufia2CpuState *cpu) {
    cpu->x = (uint16_t)(cpu->x - 1u);
    SetNz16(cpu, cpu->x);
}

/* Exit CPU state is part of the verified contract. */

/* Copy the item's name; return the last word copied. */
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

/* Trailing spaces of the name become end markers. */
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

/* Pushed DB and offset stay in the stack page. */
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
    /* Exit flags: N/Z from DB, C from the last compare. */
    SetNz8(cpu, cpu->data_bank);
    cpu->carry = trim.last_char == CHAR_END || trim.last_char >= CHAR_SPACE;
    cpu->overflow = 0;
    cpu->accumulator_is_8_bit = 1;
}

/* Record pointer: table base plus the item's offset word. */
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
    /* P restored; A and X keep their last values. */
    cpu->accumulator = pointer;
    cpu->x = (uint16_t)(item * 2u);
    if (cpu->index_is_8_bit)
        cpu->x &= 0x00ffu;
}

/* $81:F1C5: name, 9 bytes, then masked words. */
Lufia2ExecutionResult Lufia2LoadItemRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned mask;

    Push8(memory, cpu, PackStatus(cpu));                       /* F1C5 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, 0x0badu);
    do {
        DecrementX(cpu);
        StoreZeroAbsolute8(memory, cpu, 0x0000u, cpu->x);
        Compare16(cpu, cpu->x, WRAM_RECORD_BUFFER);
    } while (!cpu->zero);
    SimulateJslFrame(memory, cpu, 0x81u, 0xf1dau);
    ItemName(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0x96u);                                        /* F1DB */
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x81u, 0xf1e2u);
    ItemRecordAddress(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_ITEM_RECORD_POINTER, 0));
    LoadX16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        StoreAAbsolute8(memory, cpu, FIELDS, cpu->x);
        IncrementY16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0009u);
    } while (!cpu->zero);
    for (mask = 0; mask < 4u; ++mask) {                        /* F1F6 */
        LoadAAbsolute8(memory, cpu, (uint16_t)mask, cpu->y);
        StoreAAbsolute8(memory, cpu, (uint16_t)(MASKS + mask), 0);
    }
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    for (mask = 0; mask < 4u; ++mask) {                        /* F212 */
        const uint32_t bits = AbsoluteIndexedAddress(
            cpu, (uint16_t)(MASKS + mask), 0);

        StoreA8Absolute(memory, cpu, MASK_BITS, 0x08u);
        do {
            const uint8_t value = Read8(memory, bits);         /* LSR */

            cpu->carry = value & 1u;
            Write8(memory, bits, (uint8_t)(value >> 1));
            if (cpu->carry) {
                LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
                IncrementY16(cpu);
                StoreAAbsolute8(memory, cpu, FIELDS, cpu->x);
                LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
                IncrementY16(cpu);
                StoreAAbsolute8(memory, cpu, (uint16_t)(FIELDS + 1u), cpu->x);
            }
            IncrementX16(cpu);
            IncrementX16(cpu);
            {
                const uint32_t count =
                    AbsoluteIndexedAddress(cpu, MASK_BITS, 0);
                const uint8_t left = (uint8_t)(Read8(memory, count) - 1u);

                Write8(memory, count, left);
                SetNz8(cpu, left);
            }
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);                                 /* F28E */
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f290u);
}

/* Record pointer: table base plus the spell's offset word. */
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

    /* The add's pointer and flags are the exit state. */
    cpu->accumulator = offset;
    cpu->carry = 0;
    Add16Value(cpu, SPELL_RECORD_TABLE_BASE);
    WramWrite16(wram, WRAM_SPELL_RECORD_POINTER, cpu->accumulator);
    cpu->x = (uint16_t)(spell * 2u);
    cpu->accumulator_is_8_bit = 1;
}

/* $81:F414: name, D low byte as terminator, 11 bytes. */
Lufia2ExecutionResult Lufia2LoadSpellRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);                                 /* F414 */
    LoadA8(cpu, 0x95u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xf41bu);
    SpellRecordAddress(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_SPELL_RECORD_POINTER, 0));
    LoadX16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, LongIndexedAddress(WRAM_RECORD_BUFFER, cpu->x), A8(cpu));
        IncrementY16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0008u);
    } while (!cpu->zero);
    TransferDirectToA(cpu);                                    /* TDC */
    Write8(memory, LongIndexedAddress(WRAM_RECORD_BUFFER, cpu->x), A8(cpu));
    IncrementX16(cpu);
    do {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, LongIndexedAddress(WRAM_RECORD_BUFFER, cpu->x), A8(cpu));
        IncrementY16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0014u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81f445u);
}

/* $81:F194: A = first record byte of item $0A06. */
Lufia2ExecutionResult Lufia2ItemRecordByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    SimulateJslFrame(memory, cpu, 0x81u, 0xf19cu);
    ItemRecordAddress(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_ITEM_RECORD_POINTER, 0));
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x96u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f1aau);
}

static Lufia2ExecutionResult SpellRecordAt(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address, uint16_t offset,
    uint32_t rtl) {
    SimulateJsrFrame(memory, cpu, return_address);
    SpellRecordAddress(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_SPELL_RECORD_POINTER, 0));
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x95u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadAAbsolute8(memory, cpu, offset, cpu->y);
    PullDataBank(memory, cpu);
    return ExecutionReturned(rtl);
}

/* $81:F3F4: A = spell record byte $0C. */
Lufia2ExecutionResult Lufia2SpellRecordByteC(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return SpellRecordAt(memory, cpu, 0xf3f6u, 0x000cu, 0x81f403u);
}

/* $81:F404: A = spell record byte 8. */
Lufia2ExecutionResult Lufia2SpellRecordByte8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return SpellRecordAt(memory, cpu, 0xf406u, 0x0008u, 0x81f413u);
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
