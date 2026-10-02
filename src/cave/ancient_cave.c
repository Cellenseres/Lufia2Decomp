/* Ancient Cave floor entry ($83:9E31) and builder ($83:9013). */

#include "cave/cave_internal.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "field/event_script_internal.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "lufia2/system.h"
#include "party/party_internal.h"
#include "system/system_internal.h"
#include "system/wram.h"
#include "text/text_internal.h"

#define BFAA_HANDOFF 0x80bfbcu

static void CaveRandomByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    Lufia2CallRandomByte(memory, cpu, (uint16_t)(site + 3u));
}

/* JSL to a whole-function RTL body in the decomp library. */
static void CaveCallLong(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site,
    Lufia2ExecutionResult (*body)(const Lufia2Memory *, Lufia2CpuState *)) {
    SimulateJslFrame(memory, cpu, 0x83u, (uint16_t)(site + 3u));
    (void)body(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:9013-$83:9032: clear the room grid to the direct page's value and fill
 * the shape grid with the blank shape. Leaves the registers as the compare
 * that ends the original loop did. */
static void CaveClearGrid(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram work = WramViewLong(memory);
    uint16_t offset;

    PushDataBank(memory, cpu);                                 /* 9013 */
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    for (offset = 0; offset < CAVE_GRID_BYTES; offset += 2u) {
        WramWrite16At(work, CAVE_ROOM_GRID_LONG, offset, cpu->direct_page);
        WramWrite16At(work, CAVE_SHAPE_GRID_LONG, offset, CAVE_BLANK_SHAPE_PAIR);
    }
    LoadX16(cpu, CAVE_GRID_BYTES);
    LoadA16(cpu, CAVE_BLANK_SHAPE_PAIR);
    OpCpx(cpu, CAVE_GRID_BYTES);
}

/* $83:9034-$83:90C3: sort the item records that are allowed on this floor into
 * two lists in bank $7F. A record qualifies when it is a field item, is not
 * excluded, and its price is below the floor's limit (1000 per floor up to
 * floor 59, unlimited after). Bit 0 of its use word picks the list; the lists
 * are written at $7F:0000 and $7F:1000, each entry the item number. */
static void CaveCollectItems(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram work = WramViewLong(memory);
    Lufia2Wram records;
    uint16_t item;

    OpSepWidths(cpu, 0x20u);                                         /* 9034 */
    OpSetDataBank(memory, cpu, ITEM_RECORD_BANK);
    records = WramViewInBank(memory, cpu, ITEM_RECORD_BANK);
    OpRepWidths(cpu, 0x20u);
    OpSepWidths(cpu, 0x20u);
    LoadX16(cpu, 0); /* 903E */
    WramWrite16(wram, CAVE_DP_LIST_A_END, cpu->x);
    LoadX16(cpu, CAVE_ITEM_LIST_B_BASE);
    WramWrite16(wram, CAVE_DP_LIST_B_END, cpu->x);
    LoadA8(cpu, WramRead(work, CAVE_FLOOR_LONG)); /* 9048 */
    OpCmpValue(cpu, CAVE_UNLIMITED_PRICE_FLOOR);
    if (cpu->carry) {
        LoadA8(cpu, 0xffu);
        WramWrite(wram, CAVE_DP_PRICE_LIMIT, A8(cpu));
        WramWrite(wram, CAVE_DP_PRICE_LIMIT + 1u, A8(cpu));
    } else {
        /* Limit = 1000 * (floor + 1) through the PPU multiplier, reached
         * through the data bank's mirror of the registers. */
        LoadA8(cpu, CAVE_PRICE_PER_FLOOR & 0xffu); /* 9058 */
        WramWrite(records, SNES_M7A, A8(cpu));
        LoadA8(cpu, CAVE_PRICE_PER_FLOOR >> 8);
        WramWrite(records, SNES_M7A, A8(cpu));
        LoadA8(cpu, WramRead(work, CAVE_FLOOR_LONG));
        OpIncA(cpu);
        WramWrite(records, SNES_M7B, A8(cpu));
        LoadA8(cpu, WramRead(records, SNES_MPYL));
        WramWrite(wram, CAVE_DP_PRICE_LIMIT, A8(cpu));
        LoadA8(cpu, WramRead(records, SNES_MPYM));
        WramWrite(wram, CAVE_DP_PRICE_LIMIT + 1u, A8(cpu));
    }
    cpu->y = 0; /* 9074 */
    do {
        uint16_t record;
        uint8_t list_end;

        item = cpu->y;
        OpRepWidths(cpu, 0x20u);                                     /* 9077 */
        LoadA16(cpu, WramRead16At(records, ITEM_RECORD_TABLE, item));
        TransferAToX(cpu);
        record = cpu->x;
        OpSepWidths(cpu, 0x20u);
        LoadA8(cpu, WramReadAt(records, ITEM_RECORD_TABLE + ITEM_RECORD_FLAGS, record));
        OpBitValue(cpu, ITEM_FLAG_FIELD_USABLE);
        if (cpu->zero)
            goto next;
        OpBitValue(cpu, ITEM_FLAG_EXCLUDED);
        if (!cpu->zero)
            goto next;
        LoadA8(cpu,
               WramReadAt(records, ITEM_RECORD_TABLE + ITEM_RECORD_FLAGS2, record));
        OpBitValue(cpu, ITEM_FLAG_EXCLUDED);
        if (!cpu->zero)
            goto next;
        OpRepWidths(cpu, 0x20u);                                     /* 9091 */
        LoadA16(cpu,
                WramRead16At(records, ITEM_RECORD_TABLE + ITEM_RECORD_PRICE, record));
        OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_PRICE_LIMIT));
        if (cpu->carry)
            goto next;
        LoadA16(cpu,
                WramRead16At(records, ITEM_RECORD_TABLE + ITEM_RECORD_USE, record));
        OpBitValue(cpu, ITEM_USE_LIST_B);
        list_end = cpu->zero ? CAVE_DP_LIST_B_END : CAVE_DP_LIST_A_END; /* 90A2/90B0 */
        LoadX16(cpu, WramRead16(wram, list_end));
        OpTya(cpu);
        OpLsrA(cpu);
        WramWrite16At(work, CAVE_ITEM_LISTS_LONG, cpu->x, cpu->accumulator);
        OpInx(cpu);
        OpInx(cpu);
        WramWrite16(wram, list_end, cpu->x);
next:
        OpSepWidths(cpu, 0x20u);                                     /* 90BC */
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, ITEM_RECORD_TABLE_BYTES);
    } while (!cpu->carry);
}

/* $83:90C5-$83:9141: the optional first chests. Y is the next chest slot (two
 * bytes each) and is advanced when a chest is placed.
 *
 * A one in five roll may place a story item chest: a random one of nine, once
 * per game (tracked in the seen bits) and only while its game flag is clear. Failing
 * that, the first time on a floor below 21 a chest with the spell scroll
 * (word $022D) may appear, with a chance that falls as the floor rises. */
static void CaveFirstChests(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram work = WramViewLong(memory);
    Lufia2Wram cave;

    OpSetDataBank(memory, cpu, 0x7fu);                         /* 90C5 */
    cave = WramViewInBank(memory, cpu, 0x7fu);
    LoadY16(cpu, 0);
    TransferDirectToA(cpu);
    WramWrite(cave, CAVE_STORY_CHEST_STATE, A8(cpu));
    CaveRandomByte(memory, cpu, 0x90d0u);
    OpCmpValue(cpu, CAVE_STORY_CHEST_ODDS);
    if (!cpu->carry) {
        LoadA8(cpu, CAVE_STORY_CHEST_KINDS); /* 90D8 */
        Lufia2CaveRandomBelow(memory, cpu, 0x90dau);
        WramWrite(wram, CAVE_DP_CHEST_KIND, A8(cpu));
        Lufia2EventFlagBitFrom(memory, cpu, 0x83u, 0x90e2u);
        LoadA8(cpu, WramReadAt(work, CAVE_STORY_CHEST_SEEN_LONG, cpu->x)); /* 90E3 */
        OpBit(memory, cpu, OpDp(cpu, CAVE_DP_CHEST_BIT));
        if (cpu->zero) {
            OpOra(memory, cpu, OpDp(cpu, CAVE_DP_CHEST_BIT));
            WramWriteAt(work, CAVE_STORY_CHEST_SEEN_LONG, cpu->x, A8(cpu));
            LoadA8(cpu, WramRead(wram, CAVE_DP_CHEST_KIND));
            cpu->carry = 0;
            OpAdcValue(cpu, CAVE_STORY_FLAG_BASE);
            /* JSL $80:BE1A = JSR $80:BE1E; RTL. */
            SimulateJslFrame(memory, cpu, 0x83u, 0x90f9u);     /* 90F6 */
            Lufia2TextTestFlag(memory, cpu, 0xbe1cu);
            SimulateRtlFrame(memory, cpu);
            if (cpu->zero) {
                TransferDirectToA(cpu);                        /* 90FC */
                LoadA8(cpu, WramRead(wram, CAVE_DP_CHEST_KIND));
                OpRepWidths(cpu, 0x20u);
                OpAslA(cpu);
                OpTax(cpu);
                LoadA16(cpu, Read16Long(memory, LongIndexedAddress(
                                                    CAVE_STORY_CHEST_ITEMS, cpu->x)));
                OpOraValue(cpu, CAVE_CHEST_ITEM_MARK);
                WramWrite16(cave, CAVE_CHEST_WORDS, cpu->accumulator);
                OpSepWidths(cpu, 0x20u);
                LoadA8(cpu, CAVE_STORY_CHEST_PLACED);
                WramWrite(cave, CAVE_STORY_CHEST_STATE, A8(cpu));
                goto take_slot;
            }
        }
    }
    LoadA8(cpu, WramRead(work, CAVE_SCROLL_CHEST_STATE_LONG)); /* 9116 */
    if (cpu->negative)
        return;
    TransferDirectToA(cpu);
    WramWrite(work, CAVE_SCROLL_CHEST_STATE_LONG, A8(cpu));
    LoadA8(cpu, WramRead(work, CAVE_FLOOR_LONG));
    OpCmpValue(cpu, CAVE_SCROLL_CHEST_FLOOR_LIMIT);
    if (!cpu->carry)
        return;
    LoadA8(cpu, CAVE_SCROLL_CHEST_CHANCE); /* 9129 */
    Lufia2CaveRandomBelow(memory, cpu, 0x912bu);
    OpCmp(memory, cpu, CAVE_FLOOR_LONG);
    if (cpu->carry)
        return;
    LoadX16(cpu, CAVE_SCROLL_CHEST_WORD); /* 9134 */
    WramWrite16(cave, CAVE_CHEST_WORDS, cpu->x);
    LoadA8(cpu, 0x01u);
    WramWrite(work, CAVE_SCROLL_CHEST_STATE_LONG, A8(cpu));
take_slot:
    OpIny(cpu);                                                /* 9140 */
    OpIny(cpu);
}

/* Copies the word at table[X] to the chest word at Y, setting bits in its high
 * byte. */
static void CaveChestFromTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t table,
    uint8_t high_or) {
    const Lufia2Wram cave = WramViewOfCaller(memory, cpu);

    LoadA8(cpu, Read8(memory, LongIndexedAddress(table, cpu->x)));
    WramWriteAt(cave, CAVE_CHEST_WORDS, cpu->y, A8(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(table + 1u, cpu->x)));
    if (high_or)
        OpOraValue(cpu, high_or);
    WramWriteAt(cave, CAVE_CHEST_WORDS + 1u, cpu->y, A8(cpu));
}

/* $83:91AD: a common item, drawn from the middle of the table at $91:FFDC. */
static void CaveCommonChest(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, CAVE_COMMON_ITEM_COUNT); /* 91AD */
    Lufia2CaveRandomMean(memory, cpu, 0x91afu);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    CaveChestFromTable(memory, cpu, CAVE_COMMON_ITEM_TABLE, 0);
}

/* $83:916F/$83:9176: a random entry of the item list that starts at `base` in
 * bank $7F and ends at the direct-page pointer `end`. */
static void CaveChestFromList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t base,
    uint8_t end) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    LoadX16(cpu, base);
    LoadA8(cpu, WramRead(wram, end));
    WramWrite16(wram, CAVE_DP_PRICE_LIMIT, cpu->x); /* 917B */
    OpLsrA(cpu);
    Lufia2CaveRandomIndex(memory, cpu, 0x917eu);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_PRICE_LIMIT));
    OpTax(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, CAVE_ITEM_LISTS, cpu->x));
    WramWrite16At(wram, CAVE_CHEST_WORDS, cpu->y, cpu->accumulator);
    OpSepWidths(cpu, 0x20u);
}

/* $83:9192: a spell scroll, or a common item when a party member already knows
 * the spell. */
static void CaveSpellChest(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    LoadA8(cpu, CAVE_SPELL_COUNT); /* 9192 */
    Lufia2CaveRandomBelow(memory, cpu, 0x9194u);
    WramWrite(wram, CAVE_DP_CHEST_BIT, A8(cpu));
    PushY(memory, cpu);
    CaveCallLong(memory, cpu, 0x919au, Lufia2PartyListHasEntry);
    OpPullY(memory, cpu);
    if (cpu->carry) {
        CaveCommonChest(memory, cpu);
        return;
    }
    LoadA8(cpu, WramRead(wram, CAVE_DP_CHEST_BIT)); /* 91A1 */
    WramWriteAt(wram, CAVE_CHEST_WORDS, cpu->y, A8(cpu));
    LoadA8(cpu, CAVE_SPELL_CHEST_MARK);
    WramWriteAt(wram, CAVE_CHEST_WORDS + 1u, cpu->y, A8(cpu));
}

/* $83:9142-$83:91E4: fill the eight chest words. Each draws a random byte and
 * takes the first matching kind: the top rolls pick from the two item lists,
 * then a spell scroll, a piece of equipment, a common item, and last a
 * consumable. */
static void CaveChestContents(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    do {
        CaveRandomByte(memory, cpu, 0x9142u);                  /* 9142 */
        OpCmpValue(cpu, CAVE_ROLL_LIST_A);
        if (cpu->carry) {
            CaveChestFromList(memory, cpu, 0x0000u, CAVE_DP_LIST_A_END); /* 916F */
            goto next;
        }
        OpCmpValue(cpu, CAVE_ROLL_LIST_B);
        if (cpu->carry) {
            CaveChestFromList(memory, cpu, CAVE_ITEM_LIST_B_BASE,
                              CAVE_DP_LIST_B_END); /* 9176 */
            goto next;
        }
        OpCmpValue(cpu, CAVE_ROLL_SPELL);
        if (cpu->carry) {
            CaveSpellChest(memory, cpu);
            goto next;
        }
        OpCmpValue(cpu, CAVE_ROLL_EQUIPMENT);
        if (cpu->carry) {
            LoadA8(cpu, CAVE_EQUIPMENT_COUNT); /* 91C8 */
            Lufia2CaveRandomIndex(memory, cpu, 0x91cau);
            CaveChestFromTable(memory, cpu, CAVE_EQUIPMENT_TABLE,
                               CAVE_CHEST_EQUIPMENT_MARK);
            goto next;
        }
        OpCmpValue(cpu, CAVE_ROLL_COMMON);
        if (cpu->carry) {
            CaveCommonChest(memory, cpu);                      /* 91AD */
            goto next;
        }
        LoadA8(cpu, CAVE_CONSUMABLE_COUNT); /* 915A */
        Lufia2CaveRandomIndex(memory, cpu, 0x915cu);
        CaveChestFromTable(memory, cpu, CAVE_CONSUMABLE_TABLE, 0);
next:
        OpIny(cpu);                                            /* 91DD */
        OpIny(cpu);
        OpCpy(cpu, CAVE_CHEST_WORD_BYTES);
    } while (!cpu->carry);
}

/* $83:91E7-$83:9265: up to 7 room rectangles. */
static void CavePlaceRooms(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);                                         /* 91E7 */
    OpSetDataBank(memory, cpu, 0x83u);
    LoadA8(cpu, 0x01u);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_COLUMN));
    LoadA8(cpu, 0x04u);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_ROW));
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_COUNT));
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_TALLEST_ROOM));
    for (;;) {
        OpLda(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_COLUMN)); /* 91F9 */
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_COLUMN));
        LoadA8(cpu, 0x03u);
        Lufia2CaveRandomBelow(memory, cpu, 0x91ffu);
        OpDecA(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_ROW));
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_ROW));
        LoadA8(cpu, 0x04u);
        Lufia2CaveRandomMean(memory, cpu, 0x920au);
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_WIDTH));
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_COLUMN));
        OpCmpValue(cpu, 0x09u);
        if (!cpu->carry) {
            LoadA8(cpu, 0x04u);                                /* 9217 */
            Lufia2CaveRandomMean(memory, cpu, 0x9219u);
            OpIncA(cpu);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_HEIGHT));
            cpu->carry = 0;
            OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_ROW));
            OpCmpValue(cpu, 0x0fu);
            if (!cpu->carry) {
                OpLda(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_HEIGHT)); /* 9226 */
                OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_TALLEST_ROOM));
                if (cpu->carry)
                    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TALLEST_ROOM));
                OpLda(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_WIDTH)); /* 922E */
                cpu->carry = 0;
                OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_HEIGHT));
                OpCmpValue(cpu, 0x03u);
                if (!cpu->carry)
                    OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_WIDTH), 1);
                OpLda(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_COUNT)); /* 9239 */
                OpIncA(cpu);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_COUNT));
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_FILL_ROOM));
                OpCmpValue(cpu, 0x08u);
                if (cpu->carry)
                    return;                                    /* 9266 */
                Lufia2CaveFillRoom(memory, cpu, 0x9244u);
            }
        }
        OpLda(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_COLUMN)); /* 9247 */
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_ROOM_WIDTH));
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_COLUMN));
        OpCmpValue(cpu, 0x09u);
        if (!cpu->carry)
            continue;
        LoadA8(cpu, 0x01u);                                    /* 9252 */
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_COLUMN));
        LoadA8(cpu, 0x04u);
        Lufia2CaveRandomMean(memory, cpu, 0x9258u);
        OpIncA(cpu);
        OpIncA(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_ROW));
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_ROW));
        OpCmpValue(cpu, 0x0fu);
        if (cpu->carry)
            return;
    }
}

/* Corridor bit for one cell pair. */
static void CaveOpenCorridor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t id_offset,
    uint16_t opposite, uint16_t pair, uint16_t mark) {
    OpSta(memory, cpu, OpDp(cpu, id_offset));
    OpLda(memory, cpu, OpAbsX(cpu, opposite));
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, id_offset));
        OpOraValue(cpu, CAVE_CELL_LINKED);
        OpSta(memory, cpu, OpAbsX(cpu, pair));
        OpSta(memory, cpu, OpAbsX(cpu, mark));
    } else if (!cpu->negative) {
        return;
    } else {
        OpSta(memory, cpu, OpAbsX(cpu, mark));
    }
}

/* $83:9266-$83:9387: merge and link rooms, open corridors. */
static void CaveLinkFloor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7fu);                         /* 9266 */
    LoadA8(cpu, 0x08u);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_MERGE_PASSES));
    do {
        Lufia2CaveMergeRoom(memory, cpu, 0x926eu);             /* 926E */
        OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_MERGE_PASSES), -1);
    } while (!cpu->zero);
    Lufia2CaveCountCells(memory, cpu, 0x9275u);
    Lufia2CavePickCell(memory, cpu, 0x9278u);
    OpWriteX(memory, cpu, OpAbs(cpu, CAVE_START_COLUMN), cpu->x);
    Lufia2CaveLinkRooms(memory, cpu, 0x927eu);
    OpLdx(cpu, 0x00ffu);                                       /* 9281 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));              /* 9284 */
        if (cpu->zero)
            goto next;
        OpBitValue(cpu, CAVE_CELL_FLAGS);
        if (!cpu->zero)
            goto next;
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CELL_VALUE)); /* 9290 */
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_RIGHT));
        if (cpu->zero) {
            /* $92A9: empty right neighbour beside a room below or above. */
            OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_UP_RIGHT));
            if (cpu->zero)
                OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT));
            if (!cpu->zero) {
                OpLda(memory, cpu, OpDp(cpu, CAVE_DP_CELL_VALUE)); /* 92B6 */
                OpOraValue(cpu, CAVE_CELL_LINKED);
                OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_RIGHT));
            }
            goto next;
        }
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_LEFT)); /* 9297 */
        if (cpu->zero) {
            OpLda(memory, cpu,
                  OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_UP_LEFT)); /* 92C0 */
            if (cpu->zero)
                OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN_LEFT));
            if (!cpu->zero) {
                OpLda(memory, cpu, OpDp(cpu, CAVE_DP_CELL_VALUE)); /* 92CD */
                OpOraValue(cpu, CAVE_CELL_LINKED);
                OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_LEFT));
            }
            goto next;
        }
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN)); /* 929C */
        if (cpu->zero) {
            OpTxa(cpu);                                        /* 92D7 */
            OpAndValue(cpu, 0xf0u);
            OpCmpValue(cpu, 0xe0u);
            if (cpu->carry)
                goto next;
            OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_RIGHT));
            if (cpu->negative) {
                CaveOpenCorridor(memory, cpu, 0x55u,
                                 CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT, /* 9304 */
                                 CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT,
                                 CAVE_ROOM_GRID + CAVE_CELL_DOWN);
            } else {
                OpLda(memory, cpu,
                      OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_LEFT)); /* 92E3 */
                if (cpu->negative)
                    CaveOpenCorridor(memory, cpu, 0x55u,
                                     CAVE_ROOM_GRID + CAVE_CELL_DOWN_LEFT,
                                     CAVE_ROOM_GRID + CAVE_CELL_DOWN_LEFT,
                                     CAVE_ROOM_GRID + CAVE_CELL_DOWN); /* 92EA */
            }
            goto next;
        }
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_UP)); /* 92A1 */
        if (cpu->zero) {
            OpTxa(cpu);                                        /* 931E */
            OpAndValue(cpu, 0xf0u);
            OpCmpValue(cpu, 0x30u);
            if (!cpu->carry)
                goto next;
            OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_RIGHT));
            if (cpu->negative) {
                CaveOpenCorridor(
                    memory, cpu, 0x55u, CAVE_ROOM_GRID + CAVE_CELL_UP_RIGHT, /* 934B */
                    CAVE_ROOM_GRID + CAVE_CELL_UP_RIGHT, CAVE_ROOM_GRID + CAVE_CELL_UP);
            } else {
                OpLda(memory, cpu,
                      OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_LEFT)); /* 932A */
                if (cpu->negative)
                    CaveOpenCorridor(memory, cpu, 0x55u,
                                     CAVE_ROOM_GRID + CAVE_CELL_UP_LEFT,
                                     CAVE_ROOM_GRID + CAVE_CELL_UP_LEFT,
                                     CAVE_ROOM_GRID + CAVE_CELL_UP); /* 9331 */
            }
        }
next:
        OpDex(cpu);                                            /* 9363 */
        OpCpx(cpu, 0x0010u);
    } while (cpu->carry);
    Lufia2CaveClearVisited(memory, cpu, 0x936cu);
    Lufia2CaveLinkRooms(memory, cpu, 0x936fu);
    OpLdx(cpu, 0x00ffu);                                       /* 9372 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        if (!cpu->zero) {
            if (!cpu->negative)
                TransferDirectToA(cpu);                        /* 937C */
            OpAndValue(cpu, CAVE_CELL_ID_MASK);
            OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        }
        OpDex(cpu);                                            /* 9382 */
        OpCpx(cpu, 0x0010u);
    } while (cpu->carry);
}

/* $83:9388-$83:940D: keep one random link per room pair. */
static void CaveDedupeLinks(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x0000u);                                       /* 9388 */
    for (;;) {
        OpCompareIndex(cpu, cpu->x, /* 938B */
                       OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_LINK_COUNT)));
        if (cpu->carry)
            break;
        OpWriteX(memory, cpu, OpDp(cpu, CAVE_DP_LINK_INDEX), cpu->x);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_LINKS));
        if (!cpu->zero) {
            OpSta(memory, cpu, OpAbs(cpu, CAVE_LINK_SCRATCH)); /* 9397 */
            OpTay(cpu);
            OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_LINK_KEY));
            OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN));
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_LINK_KEY + 1u));
            LoadA8(cpu, 0x01u);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_SCRATCH_COUNT));
            OpStz(memory, cpu, OpDp(cpu, CAVE_DP_SCRATCH_COUNT + 1u));
            TransferDirectToA(cpu);
            for (;;) {
                OpInx(cpu);                                    /* 93AC */
                OpCompareIndex(cpu, cpu->x,
                               OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_LINK_COUNT)));
                if (cpu->carry)
                    break;
                OpLda(memory, cpu, OpAbsX(cpu, CAVE_LINKS));
                OpTay(cpu);
                OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));
                OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_LINK_KEY));
                if (!cpu->zero)
                    continue;
                OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN));
                OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_LINK_KEY + 1u));
                if (!cpu->zero)
                    continue;
                OpLdy(cpu, OpReadX(memory, cpu,
                                   OpDp(cpu, CAVE_DP_SCRATCH_COUNT))); /* 93C3 */
                OpLda(memory, cpu, OpAbsX(cpu, CAVE_LINKS));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_LINK_SCRATCH));
                OpIny(cpu);
                OpWriteX(memory, cpu, OpDp(cpu, CAVE_DP_SCRATCH_COUNT), cpu->y);
            }
            OpLda(memory, cpu, OpDp(cpu, CAVE_DP_SCRATCH_COUNT)); /* 93D0 */
            OpCmpValue(cpu, 0x01u);
            if (!cpu->zero) {
                Lufia2CaveRandomBelow(memory, cpu, 0x93d6u);
                ExchangeAccumulatorBytes(cpu);
                LoadA8(cpu, 0x00u);
                ExchangeAccumulatorBytes(cpu);
                OpTax(cpu);
                LoadA8(cpu, 0xffu);
                OpSta(memory, cpu, OpAbsX(cpu, CAVE_LINK_SCRATCH));
                OpLdx(cpu, 0x0000u);                           /* 93E3 */
                do {
                    OpLda(memory, cpu, OpAbsX(cpu, CAVE_LINKS));
                    OpLdy(cpu, 0x0000u);
                    do {
                        OpCmp(memory, cpu, OpAbsY(cpu, CAVE_LINK_SCRATCH)); /* 93EC */
                        if (cpu->zero) {
                            OpStz(memory, cpu, OpAbsX(cpu, CAVE_LINKS));
                            break;
                        }
                        OpIny(cpu);
                        OpCompareIndex(
                            cpu, cpu->y,
                            OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_SCRATCH_COUNT)));
                    } while (!cpu->carry);
                    OpInx(cpu);                                /* 93FB */
                    OpCompareIndex(cpu, cpu->x,
                                   OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_LINK_COUNT)));
                } while (!cpu->carry);
            }
        }
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_LINK_INDEX))); /* 9400 */
        OpInx(cpu);
    }
    TransferDirectToA(cpu);                                    /* 9405 */
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_LINK_COUNT));
    OpTay(cpu);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_LINKS));
}

/* $83:940E-$83:949C: start cell, link marks and stairs. */
static void CavePlaceStart(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2CaveCountCells(memory, cpu, 0x940eu);                /* 940E */
    Lufia2CavePickCell(memory, cpu, 0x9411u);
    Lufia2CaveCellPosition(memory, cpu, 0x9414u);
    OpIncA(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_DESTINATION_Y);
    OpSta(memory, cpu, CAVE_START_ROW_LONG);
    ExchangeAccumulatorBytes(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_DESTINATION_X);
    OpSta(memory, cpu, CAVE_START_COLUMN_LONG);
    OpLdx(cpu, 0x0000u);                                       /* 942D */
    for (;;) {
        TransferDirectToA(cpu);                                /* 9430 */
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_LINKS));
        if (!cpu->zero) {
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero)
                break;
            OpTay(cpu);                                        /* 943A */
            OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));
            OpOraValue(cpu, 0x80u);
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));
            OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN));
            OpOraValue(cpu, 0x80u);
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN));
        }
        OpInx(cpu);                                            /* 944B */
    }
    Lufia2CavePickCell(memory, cpu, 0x944eu);                  /* 944E */
    if (!cpu->carry) {
        Lufia2CaveClearVisited(memory, cpu, 0x9453u);
        Lufia2CavePickCell(memory, cpu, 0x9456u);
    }
    Lufia2CaveCellPosition(memory, cpu, 0x9459u);              /* 9459 */
    Lufia2CaveStairOffset(memory, cpu, 0x945cu);
    OpSta(memory, cpu, CAVE_STAIR_ROW_LONG);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, CAVE_STAIR_COLUMN_LONG);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, CAVE_LINK_MARK_A_LONG);
    OpSta(memory, cpu, CAVE_LINK_MARK_B_LONG);
    CaveRandomByte(memory, cpu, 0x9472u);
    OpCmpValue(cpu, 0x10u);
    if (!cpu->carry) {
        Lufia2CavePickCell(memory, cpu, 0x947au);
        if (cpu->carry) {
            Lufia2CaveCellPosition(memory, cpu, 0x947fu);      /* 947F */
            Lufia2CaveStairOffset(memory, cpu, 0x9482u);
            OpSta(memory, cpu, CAVE_LINK_MARK_B_LONG);
            ExchangeAccumulatorBytes(cpu);
            OpIncA(cpu);
            OpSta(memory, cpu, CAVE_LINK_MARK_A_LONG);
        }
    }
    Lufia2CaveClearVisited(memory, cpu, 0x948fu);              /* 948F */
    OpLdx(cpu, 0x0013u);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, CAVE_OBJECT_COLUMNS_LONG));           /* 9496 */
        OpDex(cpu);
    } while (!cpu->negative);
}

/* $83:949D-$83:9514: one floor in sixteen gets a 2x2 treasure room. The first
 * 2x2 block of identical cells (searched from the second row, then thinned out
 * by a coin flip per candidate) becomes the room: thirteen objects are
 * scattered within eight tiles of its corner and eight chests are added. */
static void CaveTreasureRoom(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    CaveRandomByte(memory, cpu, 0x949du);                      /* 949D */
    OpCmpValue(cpu, CAVE_TREASURE_ROOM_ODDS);
    if (cpu->carry)
        return;
    LoadA8(cpu, CAVE_TREASURE_MARK);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_SCRATCH_COUNT + 1u));
    OpLdy(cpu, CAVE_FIRST_ROOM_CELL);
    for (;;) {
        OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));              /* 94AC */
        if (!cpu->zero &&
            (OpCmp(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID + CAVE_CELL_RIGHT)),
             cpu->zero) &&
            (OpCmp(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN)),
             cpu->zero) &&
            (OpCmp(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT)),
             cpu->zero) &&
            (CaveRandomByte(memory, cpu, 0x94c0u),
             OpCmpValue(cpu, CAVE_TREASURE_CELL_ODDS), !cpu->carry)) {
            OpTya(cpu);                                        /* 94C8 */
            OpSta(memory, cpu, OpAbs(cpu, CAVE_TREASURE_CELL));
            Lufia2CaveCellPosition(memory, cpu, 0x94ccu);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_ROW));
            ExchangeAccumulatorBytes(cpu);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_COLUMN));
            OpLdx(cpu, CAVE_TREASURE_OBJECTS - 1u);
            do {
                OpPushX(memory, cpu);                          /* 94D7 */
                LoadA8(cpu, CAVE_TREASURE_SPREAD);
                Lufia2CaveRandomBelow(memory, cpu, 0x94dau);
                cpu->carry = 0;
                OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_COLUMN));
                OpAdcValue(cpu, CAVE_TREASURE_COLUMN_OFFSET);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
                LoadA8(cpu, CAVE_TREASURE_SPREAD);
                Lufia2CaveRandomBelow(memory, cpu, 0x94e6u);
                cpu->carry = 0;
                OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_CURSOR_ROW));
                OpAdcValue(cpu, CAVE_TREASURE_ROW_OFFSET);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
                Lufia2CaveNearStartOrPlaced(memory, cpu, 0x94f0u);
                if (!cpu->carry)
                    Lufia2CaveAddObject(memory, cpu, 0x94f5u);
                OpPullX(memory, cpu);                          /* 94F8 */
                OpDex(cpu);
            } while (!cpu->negative);
            OpLdx(cpu, CAVE_TREASURE_CHESTS); /* 94FC */
            do {
                OpPushX(memory, cpu);
                OpLda(memory, cpu, OpAbs(cpu, CAVE_TREASURE_CELL));
                Lufia2CaveAddChest(memory, cpu, 0x9503u);
                OpPullX(memory, cpu);
                OpDex(cpu);
            } while (!cpu->zero);
            break;
        }
        OpIny(cpu);                                            /* 950C */
        OpCpy(cpu, CAVE_TREASURE_LAST_CELL);
        if (cpu->carry)
            break;
    }
    Lufia2CaveClearVisited(memory, cpu, 0x9512u);              /* 9512 */
}

/* $83:9515-$83:959C: four to seven objects at random cells, then a chest in
 * each unmarked 2x2 room until the chest limit is reached. */
static void CaveObjectsAndChests(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, CAVE_EXTRA_ROOM_OBJECTS); /* 9515 */
    Lufia2CaveRandomBelow(memory, cpu, 0x9517u);
    cpu->carry = 0;
    OpAdcValue(cpu, CAVE_MIN_ROOM_OBJECTS);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_OBJECT_TARGET));
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_OBJECT_TARGET + 1u));
    OpLdx(cpu, 0x0000u);
    do {
        LoadA8(cpu, CAVE_OBJECT_JITTER); /* 9524 */
        Lufia2CaveRandomBelow(memory, cpu, 0x9526u);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
        LoadA8(cpu, CAVE_OBJECT_JITTER);
        Lufia2CaveRandomBelow(memory, cpu, 0x952du);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
        OpWriteX(memory, cpu, OpDp(cpu, CAVE_DP_OBJECT_INDEX), cpu->x);
        Lufia2CavePickCell(memory, cpu, 0x9534u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_OBJECT_INDEX)));
        if (cpu->carry) {
            Lufia2CaveCellPosition(memory, cpu, 0x953bu);      /* 953B */
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_OBJECT_INDEX)));
            cpu->carry = 1;
            OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            OpIncA(cpu);
            OpIncA(cpu);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 1;
            OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            Lufia2CaveNearStartOrPlaced(memory, cpu, 0x954du);
            if (!cpu->carry)
                Lufia2CaveAddObject(memory, cpu, 0x9552u);
        }
        OpInx(cpu);                                            /* 9555 */
        OpCompareIndex(cpu, cpu->x,
                       OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_OBJECT_TARGET)));
    } while (!cpu->carry);
    OpLdx(cpu, CAVE_FIRST_ROOM_CELL); /* 955A */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));              /* 955D */
        if (!cpu->zero && !cpu->negative &&
            (OpCmp(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_RIGHT)),
             cpu->zero) &&
            (OpCmp(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN)),
             cpu->zero) &&
            (OpCmp(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT)),
             cpu->zero)) {
            OpOraValue(cpu, CAVE_CELL_MARKED); /* 9573 */
            OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
            OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_RIGHT));
            OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN));
            OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT));
            OpTxa(cpu);
            Lufia2CaveAddChest(memory, cpu, 0x9582u);
            if (!cpu->carry) {
                OpLda(memory, cpu, OpAbs(cpu, CAVE_CHEST_COUNT));       /* 9587 */
                OpCmpValue(cpu, CAVE_MAX_CHESTS);
                if (cpu->carry)
                    break;
                /* Never ends the loop: $E734 reaches 8 first. */
                OpLda(memory, cpu, OpDp(cpu, CAVE_DP_CHEST_ATTEMPTS));
                OpIncA(cpu);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CHEST_ATTEMPTS));
                OpCmpValue(cpu, CAVE_MAX_CHESTS);
                if (cpu->carry)
                    break;
            }
        }
        OpInx(cpu);                                            /* 9597 */
        OpCpx(cpu, CAVE_GRID_ROWS_END);
    } while (!cpu->carry);
    Lufia2CaveClearVisited(memory, cpu, 0x959du);              /* 959D */
}

/* $83:95A0-$83:9653: pick the block shape of each occupied cell. The eight
 * neighbours that hold the same room value form a bit mask; the corner bits
 * are dropped when an adjacent side is open, and the result indexes the shape
 * table. The first column then gets the border shapes. */
static void CaveShapeCells(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t kNeighbours[8] = {
        CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT, CAVE_ROOM_GRID + CAVE_CELL_RIGHT,
        CAVE_ROOM_GRID + CAVE_CELL_UP_RIGHT,   CAVE_ROOM_GRID + CAVE_CELL_DOWN,
        CAVE_ROOM_GRID + CAVE_CELL_UP,         CAVE_ROOM_GRID + CAVE_CELL_DOWN_LEFT,
        CAVE_ROOM_GRID + CAVE_CELL_LEFT,       CAVE_ROOM_GRID + CAVE_CELL_UP_LEFT,
    };
    static const uint8_t kSide[4] = {0x80u, 0x04u, 0x01u, 0x20u};
    static const uint8_t kCorner[4] = {0x50u, 0x12u, 0x0au, 0x48u};

    LoadA8(cpu, 0x01u);                                        /* 95A0 */
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    Lufia2CaveCellIndex(memory, cpu, 0x95a6u);
    OpTxy(cpu);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));              /* 95AA */
        if (!cpu->zero) {
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_CELL_VALUE));
            OpStz(memory, cpu, OpDp(cpu, CAVE_DP_NEIGHBOUR_BITS));
            OpLda(memory, cpu, OpDp(cpu, CAVE_DP_CELL_VALUE));
            for (unsigned i = 0; i < 8; ++i) {
                if (i == 5)
                    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_CELL_VALUE)); /* 95E0 */
                OpCmp(memory, cpu, OpAbsY(cpu, kNeighbours[i]));
                if (!cpu->zero)
                    cpu->carry = 0;
                OpRolMem8(memory, cpu, OpDp(cpu, CAVE_DP_NEIGHBOUR_BITS));
            }
            for (unsigned i = 0; i < 4; ++i) {
                OpLda(memory, cpu, OpDp(cpu, CAVE_DP_NEIGHBOUR_BITS)); /* 95FA */
                if (i == 0) {
                    if (cpu->negative) {
                        OpAndValue(cpu, kCorner[i]);
                        if (cpu->zero)
                            OpTestBits(memory, cpu, OpDp(cpu, CAVE_DP_NEIGHBOUR_BITS),
                                       0);
                    }
                    continue;
                }
                OpBitValue(cpu, kSide[i]);
                if (!cpu->zero) {
                    OpAndValue(cpu, kCorner[i]);
                    if (cpu->zero)
                        OpTestBits(memory, cpu, OpDp(cpu, CAVE_DP_NEIGHBOUR_BITS), 0);
                }
            }
            TransferDirectToA(cpu);                            /* 9628 */
            OpLda(memory, cpu, OpDp(cpu, CAVE_DP_NEIGHBOUR_BITS));
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, CAVE_SHAPE_TABLE));
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SHAPE_GRID));
        }
        OpIny(cpu);                                            /* 9633 */
        OpCpy(cpu, CAVE_GRID_ROWS_END);
    } while (!cpu->carry);
    OpLdy(cpu, CAVE_FIRST_ROOM_CELL); /* 963C */
    OpLdx(cpu, 0x0000u);
    do {
        OpTxa(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, CAVE_BORDER_SHAPE_BASE);
        OpSta(memory, cpu, OpAbsY(cpu, CAVE_SHAPE_GRID));
        OpTya(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, CAVE_GRID_WIDTH);
        OpTay(cpu);
        OpInx(cpu);
        OpCpx(cpu, CAVE_BORDER_SHAPES);
    } while (!cpu->carry);
}

/* $83:9654-$83:9692: unpack the floor's block set to $7E:4000, clear the tile
 * map and copy the two layer headers into it. */
static void CaveTileMapBase(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x30u);                                         /* 9654 */
    OpLda(memory, cpu, OpAbs(cpu, CAVE_BLOCK_SET));
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_RESOURCE));
    LoadA16(cpu, CAVE_BLOCK_BUFFER);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_RESOURCE_TARGET));
    OpSepWidths(cpu, 0x20u);
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_RESOURCE_TARGET + 2u));
    CaveCallLong(memory, cpu, 0x9666u, Lufia2DecompressResource);
    OpRepWidths(cpu, 0x20u);                                         /* 966A */
    OpStz(memory, cpu, OpAbs(cpu, 0x0000u));
    OpLdx(cpu, 0x0000u);
    OpLdy(cpu, 0x0002u);
    LoadA16(cpu, CAVE_MAP_BYTES - 2u);
    OpMoveNext(memory, cpu, 0x7fu, 0x7fu);
    OpLdx(cpu, CAVE_TEMPLATE_A); /* 967B */
    OpLdy(cpu, 0x0000u);
    LoadA16(cpu, CAVE_TEMPLATE_A_BYTES - 1u);
    OpMoveNext(memory, cpu, 0x7fu, 0x83u);
    OpLdx(cpu, CAVE_TEMPLATE_B); /* 9687 */
    OpLdy(cpu, CAVE_MAP_LAYER_2_HEADER);
    LoadA16(cpu, CAVE_TEMPLATE_B_BYTES - 1u);
    OpMoveNext(memory, cpu, 0x7fu, 0x83u);
}

/* $83:9693-$83:96F4: draw each cell's block, then the link blocks ($39). */
static void CaveDrawCells(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);                                         /* 9693 */
    LoadA8(cpu, CAVE_GRID_WIDTH);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_DRAW_CELL));
    LoadA8(cpu, CAVE_GRID_ROWS);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_DRAW_ROWS));
    OpLdx(cpu, CAVE_FIRST_ROOM_CELL);
    do {
        LoadA8(cpu, CAVE_GRID_WIDTH); /* 96A0 */
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_DRAW_COLUMNS));
        do {
            OpLda(memory, cpu, OpLongX(cpu, CAVE_SHAPE_GRID_LONG));       /* 96A4 */
            OpCmpValue(cpu, CAVE_BLANK_SHAPE);
            if (!cpu->zero) {
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_BLOCK_SHAPE));
                OpPushX(memory, cpu);
                OpLda(memory, cpu, OpDp(cpu, CAVE_DP_DRAW_CELL));
                Lufia2CaveCellPosition(memory, cpu, 0x96b1u);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
                ExchangeAccumulatorBytes(cpu);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
                Lufia2CaveTileOffsetY2(memory, cpu, 0x96b9u);
                Lufia2CaveDrawBlock(memory, cpu, 0x96bcu);
                OpPullX(memory, cpu);
            }
            OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_DRAW_CELL), 1); /* 96C0 */
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_DRAW_COLUMNS), -1);
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_DRAW_ROWS), -1);
    } while (!cpu->zero);
    OpLdx(cpu, 0x0000u);                                       /* 96CB */
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_LINKS)); /* 96CE */
        if (!cpu->zero) {
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero)
                break;
            OpPushX(memory, cpu);                              /* 96D7 */
            OpLda(memory, cpu, OpAbsX(cpu, CAVE_LINKS));
            Lufia2CaveCellPosition(memory, cpu, 0x96dbu);
            cpu->carry = 0;
            OpAdcValue(cpu, CAVE_LINK_BLOCK_ROW);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            ExchangeAccumulatorBytes(cpu);
            OpIncA(cpu);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            Lufia2CaveTileOffsetY2(memory, cpu, 0x96e7u);
            LoadA8(cpu, CAVE_LINK_SHAPE);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_BLOCK_SHAPE));
            Lufia2CaveDrawBlock(memory, cpu, 0x96eeu);
            OpPullX(memory, cpu);
        }
        OpInx(cpu);                                            /* 96F2 */
    }
}

/* JSL $80:BFAA from bank $83; 0 = handoff at $80:BFBC. */
static uint8_t CaveListSearch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    return Lufia2FieldListSearch(memory, cpu, 0x83u, (uint16_t)(site + 3u));
}

/* $83:96F5-$83:9752: read the four tile sets from list 5 into the record
 * table. Returns 0 when the list search hands off. */
static uint8_t CaveReadTileSets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x0000u);                                       /* 96F5 */
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_SET_OFFSET));
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_SET_OFFSET + 1u));
    do {
        OpPushX(memory, cpu);                                  /* 96FC */
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_SET_OFFSET)));
        LoadA8(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE));
        OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE + 1u));
        LoadA8(cpu, CAVE_LIST_TILE_SETS);
        ExchangeAccumulatorBytes(cpu);
        OpTxa(cpu);
        OpLdx(cpu, CAVE_TILE_SET_KEY);
        if (!CaveListSearch(memory, cpu, 0x970eu))
            return 0;
        if (!cpu->carry) {
            OpLda(memory, cpu, OpLongX(cpu, CAVE_LIST_ENTRY_LONG + 1u)); /* 9714 */
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            OpLda(memory, cpu, OpLongX(cpu, CAVE_LIST_ENTRY_LONG + 2u));
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            Lufia2CaveBlockOffsetX(memory, cpu, 0x9720u);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpLongX(cpu, CAVE_BLOCK_BUFFER_LONG + 0x0cu));
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE + 0x100u));
            OpLda(memory, cpu, OpLongX(cpu, CAVE_BLOCK_BUFFER_LONG + 0x0eu));
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE + 0x200u));
            OpLda(memory, cpu, OpLongX(cpu, CAVE_BLOCK_BUFFER_LONG + 0x10u));
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE + 0x300u));
            OpLda(memory, cpu, OpLongX(cpu, CAVE_BLOCK_BUFFER_LONG + 0x0au));
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE));
            OpTyx(cpu);                                        /* 9741 */
            OpSta(memory, cpu, OpLongX(cpu, CAVE_TILE_SET_WORDS_LONG));
            OpIny(cpu);
            OpIny(cpu);
            OpWriteX(memory, cpu, OpDp(cpu, CAVE_DP_SET_OFFSET), cpu->y);
            OpSepWidths(cpu, 0x20u);
        }
        OpPullX(memory, cpu);                                  /* 974C */
        OpInx(cpu);
        OpCpx(cpu, CAVE_TILE_SET_COUNT);
    } while (!cpu->carry);
    return 1;
}

/* $83:9753-$83:97E7: for every occupied cell, replace the tile indices of its
 * 6x6 block that match a set's keys with the set's tiles, keeping the flag
 * bits. */
static void CaveApplyTileSets(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, CAVE_FIRST_ROOM_CELL); /* 9753 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));              /* 9756 */
        if (cpu->zero)
            goto next;
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COUNT));
        OpPushX(memory, cpu);
        OpTxa(cpu);
        Lufia2CaveCellPosition(memory, cpu, 0x9762u);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
        Lufia2CaveTileOffsetY(memory, cpu, 0x976au);
        OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COUNT));
        OpAndValue(cpu, CAVE_CELL_SET_MASK);
        if (!cpu->zero) {
            static const uint8_t kKeys[4] = {0x22u, 0x24u, 0x26u, 0x28u};
            static const uint8_t kTiles[4] = {0x11u, 0x13u, 0x15u, 0x17u};

            ExchangeAccumulatorBytes(cpu);                     /* 9773 */
            LoadA8(cpu, 0x00u);
            OpTax(cpu);
            OpRepWidths(cpu, 0x20u);
            for (unsigned i = 0; i < 4; ++i) {
                OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(CAVE_SET_TABLE + 2u * i)));
                OpSta(memory, cpu, OpDp(cpu, kTiles[i]));
            }
            LoadA16(cpu, CAVE_BLOCK_TILE_SPAN);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_ROW_COUNT));
            for (;;) {
                LoadA16(cpu, CAVE_BLOCK_TILE_SPAN); /* 9792 */
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COUNT));
                for (;;) {
                    OpLda(memory, cpu, OpAbsY(cpu, CAVE_MAP_LAYER_1)); /* 9797 */
                    OpAndValue(cpu, CAVE_TILE_INDEX_MASK);
                    for (unsigned i = 0; i < 4; ++i) {
                        OpCmp(memory, cpu, OpDp(cpu, kKeys[i]));
                        if (cpu->zero) {
                            OpLda(memory, cpu, OpDp(cpu, kTiles[i]));
                            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_BITS)); /* 97BB */
                            OpLda(memory, cpu, OpAbsY(cpu, CAVE_MAP_LAYER_1));
                            OpAndValue(cpu, CAVE_TILE_FLAG_MASK);
                            OpOra(memory, cpu, OpDp(cpu, CAVE_DP_TILE_BITS));
                            OpSta(memory, cpu, OpAbsY(cpu, CAVE_MAP_LAYER_1));
                            break;
                        }
                    }
                    OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COUNT),
                              -1); /* 97C8 */
                    if (cpu->zero)
                        break;
                    OpIny(cpu);
                    OpIny(cpu);
                }
                OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_ROW_COUNT), -1); /* 97D0 */
                if (cpu->zero)
                    break;
                OpTya(cpu);
                cpu->carry = 0;
                OpAdcValue(cpu, CAVE_MAP_ROW_SKIP);
                OpTay(cpu);
                if (cpu->zero)
                    break;
            }
        }
        OpSepWidths(cpu, 0x20u);                                     /* 97DC */
        OpPullX(memory, cpu);
next:
        OpInx(cpu);                                            /* 97DF */
        OpCpx(cpu, CAVE_GRID_ROWS_END);
    } while (!cpu->carry);
}

/* $83:97E8-$83:9868: read the five decoration sets from list 10 into the
 * record table, one 256-byte block per variant. Returns 0 when the list
 * search hands off. */
static uint8_t CaveReadDecorations(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_SET_OFFSET)); /* 97E8 */
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_SET_OFFSET + 1u));
    OpLdx(cpu, CAVE_DECORATION_FIRST_SET);
    do {
        OpPushX(memory, cpu);                                  /* 97EF */
        LoadA8(cpu, CAVE_LIST_DECORATIONS);
        ExchangeAccumulatorBytes(cpu);
        OpTxa(cpu);
        OpLdx(cpu, CAVE_DECORATION_KEY);
        if (!CaveListSearch(memory, cpu, 0x97f7u))
            return 0;
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, CAVE_DP_SET_OFFSET))); /* 97FB */
        if (cpu->carry) {
            OpRepWidths(cpu, 0x20u);                                 /* 97FF */
            TransferDirectToA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE));
            LoadA16(cpu, 0xffffu);
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE + 0x100u));
        } else {
            OpLda(memory, cpu, OpLongX(cpu, CAVE_LIST_ENTRY_LONG + 2u)); /* 980D */
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            OpLda(memory, cpu, OpLongX(cpu, CAVE_LIST_ENTRY_LONG + 3u));
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            OpLda(memory, cpu, OpLongX(cpu, CAVE_LIST_ENTRY_LONG + 8u));
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE));
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_ROW_COUNT));
            OpStz(memory, cpu, OpDp(cpu, CAVE_DP_ROW_COUNT + 1u));
            OpLda(memory, cpu, OpLongX(cpu, CAVE_LIST_ENTRY_LONG + 9u));
            OpDecA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE + 1u));
            Lufia2CaveBlockOffsetX(memory, cpu, 0x982cu);
            OpRepWidths(cpu, 0x20u);
            do {
                OpTya(cpu);                                    /* 9831 */
                OpAdcValue(cpu, CAVE_SET_VARIANT_STRIDE);      /* no CLC */
                OpTay(cpu);
                OpLda(memory, cpu, OpLongX(cpu, CAVE_BLOCK_BUFFER_LONG + 0x0au));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE));
                OpLda(memory, cpu, OpLongX(cpu, CAVE_BLOCK_BUFFER_LONG + 0x6au));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE + 2u));
                OpLda(memory, cpu, OpLongX(cpu, CAVE_BLOCK_BUFFER_LONG + 0xfceu));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE + 4u));
                OpLda(memory, cpu, OpLongX(cpu, CAVE_BLOCK_BUFFER_LONG + 0x102eu));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_SET_TABLE + 6u));
                OpInx(cpu);
                OpInx(cpu);
                OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_ROW_COUNT), -1);
            } while (!cpu->zero);
        }
        OpLda(memory, cpu, OpDp(cpu, CAVE_DP_SET_OFFSET)); /* 9858 */
        cpu->carry = 0;
        OpAdcValue(cpu, CAVE_SET_SIZE);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_SET_OFFSET));
        OpSepWidths(cpu, 0x20u);
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpCpx(cpu, CAVE_DECORATION_FIRST_SET + CAVE_DECORATION_SETS);
    } while (!cpu->carry);
    return 1;
}

/* $83:9869-$83:992F: scatter decorations over every occupied cell's block. A
 * tile that matches a set's source tile is, with odds of 48 in 256, swapped
 * for a random variant of that set, on both map layers and on the row below
 * when the set is two tiles tall. */
static void CaveDecorate(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, CAVE_FIRST_ROOM_CELL); /* 9869 */
    do {
        OpPushX(memory, cpu);                                  /* 986C */
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        if (cpu->zero)
            goto next;
        OpTxa(cpu);                                            /* 9875 */
        Lufia2CaveCellPosition(memory, cpu, 0x9876u);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
        Lufia2CaveTileOffsetY(memory, cpu, 0x987eu);
        OpRepWidths(cpu, 0x20u);
        LoadA16(cpu, CAVE_BLOCK_TILE_SPAN);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_ROW_COUNT));
        for (;;) {
            LoadA16(cpu, CAVE_BLOCK_TILE_SPAN); /* 9888 */
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COUNT));
            for (;;) {
                CaveRandomByte(memory, cpu, 0x988du);          /* 988D */
                OpCmpValue(cpu, CAVE_DECORATION_ODDS);
                if (cpu->carry)
                    goto step;
                OpLda(memory, cpu, OpAbsY(cpu, CAVE_MAP_LAYER_2));
                OpAndValue(cpu, CAVE_TILE_INDEX_MASK);
                if (!cpu->zero)
                    goto step;
                OpLda(memory, cpu, OpAbsY(cpu, CAVE_MAP_LAYER_1)); /* 989E */
                OpAndValue(cpu, CAVE_TILE_INDEX_MASK);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_BITS));
                OpLdx(cpu, 0x0000u);
                for (;;) {
                    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_BITS)); /* 98A9 */
                    OpCmp(memory, cpu, OpAbsX(cpu, CAVE_SET_TABLE + 0x100u));
                    if (cpu->zero)
                        break;
                    OpTxa(cpu);
                    cpu->carry = 0;
                    OpAdcValue(cpu, CAVE_SET_SIZE);
                    OpTax(cpu);
                    OpCpx(cpu, CAVE_DECORATION_SETS * CAVE_SET_SIZE);
                    if (cpu->carry)
                        goto step;
                }
                OpWriteX(memory, cpu, OpDp(cpu, CAVE_DP_TILE_BITS), cpu->x); /* 98BD */
                OpSepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpAbsX(cpu, CAVE_SET_TABLE + 1u));
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_EXTRA_ROWS));
                OpStz(memory, cpu, OpDp(cpu, CAVE_DP_EXTRA_ROWS + 1u));
                OpLda(memory, cpu, OpAbsX(cpu, CAVE_SET_TABLE));
                Lufia2CaveRandomBelow(memory, cpu, 0x98cbu);
                OpIncA(cpu);
                ExchangeAccumulatorBytes(cpu);
                LoadA8(cpu, 0x00u);
                OpRepWidths(cpu, 0x20u);
                cpu->carry = 0;
                OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_TILE_BITS));
                OpTax(cpu);
                OpLda(memory, cpu, OpAbsY(cpu, CAVE_MAP_LAYER_1)); /* 98D8 */
                OpAndValue(cpu, CAVE_TILE_FLAG_MASK);
                OpOra(memory, cpu, OpAbsX(cpu, CAVE_SET_TABLE));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_MAP_LAYER_1));
                OpLda(memory, cpu, OpAbsY(cpu, CAVE_MAP_LAYER_2));
                OpAndValue(cpu, CAVE_TILE_FLAG_MASK);
                OpOra(memory, cpu, OpAbsX(cpu, CAVE_SET_TABLE + 4u));
                OpSta(memory, cpu, OpAbsY(cpu, CAVE_MAP_LAYER_2));
                OpLda(memory, cpu, OpDp(cpu, CAVE_DP_EXTRA_ROWS));
                if (!cpu->zero) {
                    OpLda(
                        memory, cpu,
                        OpAbsY(cpu, CAVE_MAP_LAYER_1 + CAVE_MAP_ROW_BYTES)); /* 98F4 */
                    OpAndValue(cpu, CAVE_TILE_FLAG_MASK);
                    OpOra(memory, cpu, OpAbsX(cpu, CAVE_SET_TABLE + 2u));
                    OpSta(memory, cpu,
                          OpAbsY(cpu, CAVE_MAP_LAYER_1 + CAVE_MAP_ROW_BYTES));
                    OpLda(memory, cpu,
                          OpAbsY(cpu, CAVE_MAP_LAYER_2 + CAVE_MAP_ROW_BYTES));
                    OpAndValue(cpu, CAVE_TILE_FLAG_MASK);
                    OpOra(memory, cpu, OpAbsX(cpu, CAVE_SET_TABLE + 6u));
                    OpSta(memory, cpu,
                          OpAbsY(cpu, CAVE_MAP_LAYER_2 + CAVE_MAP_ROW_BYTES));
                }
step:
    OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COUNT), -1); /* 990C */
    if (cpu->zero)
        break;
    OpIny(cpu);
    OpIny(cpu);
            }
            OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_ROW_COUNT), -1); /* 9915 */
            if (cpu->zero)
                break;
            OpTya(cpu);
            cpu->carry = 0;
            OpAdcValue(cpu, CAVE_MAP_ROW_SKIP);
            OpTay(cpu);
            if (cpu->zero)
                break;
        }
next:
        OpSepWidths(cpu, 0x20u);                                     /* 9924 */
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpCpx(cpu, CAVE_GRID_ROWS_END);
    } while (!cpu->carry);
}

/* Upper tile at cell A/B, then $83:9D46. */
static void CaveMarkTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site,
    uint16_t upper) {
    Lufia2CaveTileAt(memory, cpu, site);
    OpLda(memory, cpu, OpAbs(cpu, upper));
    Lufia2CaveSetUpperTile(memory, cpu, (uint16_t)(site + 6u));
    OpSepWidths(cpu, 0x20u);
}

/* $83:9930-$83:99C7: mark the start, link, stair and chest tiles, then build
 * the map sections and their attributes. */
static void CaveFinish(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, CAVE_START_COLUMN));                   /* 9930 */
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_START_ROW));
    Lufia2CaveTileAt(memory, cpu, 0x9937u);
    TransferDirectToA(cpu);                                    /* 993A */
    Lufia2CaveSetUpperTile(memory, cpu, 0x993bu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_LINK_MARK_A)); /* 9940 */
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, CAVE_LINK_MARK_A));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbs(cpu, CAVE_LINK_MARK_B));
        CaveMarkTile(memory, cpu, 0x994eu, CAVE_LINK_MARK_TILE);
    }
    OpLda(memory, cpu, OpAbs(cpu, CAVE_STAIR_COLUMN));                   /* 9959 */
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_STAIR_ROW));
    CaveMarkTile(memory, cpu, 0x9960u, CAVE_STAIR_TILE);
    OpLdx(cpu, 0x0000u);                                       /* 996B */
    for (;;) {
        OpTxa(cpu);                                            /* 996E */
        OpCmp(memory, cpu, OpAbs(cpu, CAVE_CHEST_COUNT));
        if (cpu->carry)
            break;
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_CHEST_COLUMNS));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_CHEST_ROWS));
        Lufia2CaveTileAt(memory, cpu, 0x997bu);
        OpPushX(memory, cpu);                                  /* 997E */
        OpTxa(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u | CAVE_CHEST_WORDS));
        OpBitValue(cpu, CAVE_CHEST_ITEM_FLAG);
        OpLda(memory, cpu,
              OpAbs(cpu, cpu->zero ? CAVE_CHEST_TILE : CAVE_CHEST_TILE_ITEM));
        Lufia2CaveSetUpperTile(memory, cpu, 0x9993u);
        OpSepWidths(cpu, 0x20u);
        OpPullX(memory, cpu);
        OpInx(cpu);
    }
    OpStz(memory, cpu, OpAbs(cpu, CAVE_SECTION_COUNT)); /* 999C */
    OpStz(memory, cpu, OpAbs(cpu, CAVE_SECTION_COUNT + 1u));
    OpLdx(cpu, 0x0000u);
    OpWriteX(memory, cpu, OpDp(cpu, CAVE_DP_SECTION_INDEX), cpu->x);
    CaveCallLong(memory, cpu, 0x99a7u, Lufia2FieldReadSections);
    OpLdx(cpu, CAVE_SECTION_BYTES); /* 99AB */
    OpWriteX(memory, cpu, OpDp(cpu, CAVE_DP_SECTION_SIZE), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_SECTION_FLAGS));
    OpLda(memory, cpu, CAVE_MAP_RESOURCE_LONG);
    CaveCallLong(memory, cpu, 0x99b8u, Lufia2FieldDecompressMapData);
    CaveCallLong(memory, cpu, 0x99bcu, Lufia2FieldSectionSize);
    OpSepWidths(cpu, 0x20u);
    CaveCallLong(memory, cpu, 0x99c2u, Lufia2FieldPackSectionAttributes);
    PullDataBank(memory, cpu);                                 /* 99C6 */
}

Lufia2ExecutionResult Lufia2CaveBuildFloor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    CaveClearGrid(memory, cpu);
    CaveCollectItems(memory, cpu);
    CaveFirstChests(memory, cpu);
    CaveChestContents(memory, cpu);
    CavePlaceRooms(memory, cpu);
    CaveLinkFloor(memory, cpu);
    CaveDedupeLinks(memory, cpu);
    CavePlaceStart(memory, cpu);
    CaveTreasureRoom(memory, cpu);
    CaveObjectsAndChests(memory, cpu);
    CaveShapeCells(memory, cpu);
    CaveTileMapBase(memory, cpu);
    CaveDrawCells(memory, cpu);
    if (!CaveReadTileSets(memory, cpu))
        return ExecutionHandoff(cpu, BFAA_HANDOFF);
    CaveApplyTileSets(memory, cpu);
    if (!CaveReadDecorations(memory, cpu))
        return ExecutionHandoff(cpu, BFAA_HANDOFF);
    CaveDecorate(memory, cpu);
    CaveFinish(memory, cpu);
    return ExecutionReturned(0x8399c7u);                       /* 99C7 RTS */
}

static Lufia2ExecutionResult CaveChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);

    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

/* $83:9E31. */
Lufia2ExecutionResult Lufia2AncientCaveGenerateFloor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    Lufia2ExecutionResult result;

    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9E31 */
    OpCmp(memory, cpu, 0x000b75u);
    if (cpu->carry)
        OpSta(memory, cpu, 0x000b75u);
    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9E3F */
    OpDecA(cpu);
    OpSta(memory, cpu, 0x004204u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, 0x004205u);
    LoadA8(cpu, 0x0au);
    OpSta(memory, cpu, 0x004206u);
    LoadA8(cpu, 0xffu);                                        /* 9E53 */
    OpSta(memory, cpu, 0x7fe6f1u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, CAVE_OBJECT_COUNT_LONG);
    OpSta(memory, cpu, 0x7fe732u);
    OpSta(memory, cpu, CAVE_CHEST_COUNT_LONG);
    OpSta(memory, cpu, 0x7fe735u);
    OpSta(memory, cpu, 0x7fe733u);
    OpLda(memory, cpu, OpDp(cpu, 0x40u));                      /* 9E6E */
    OpAndValue(cpu, 0x1fu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    do {
        CaveRandomByte(memory, cpu, 0x9e75u);
        OpStepMem(memory, cpu, OpDp(cpu, 0x54u), -1);
    } while (!cpu->zero);
    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9E7D */
    OpCmpValue(cpu, 0x63u);
    if (cpu->zero) {
        LoadA8(cpu, 0x0bu);                                    /* 9E85 */
        OpSta(memory, cpu, WRAM_FIELD_DESTINATION_X);
        LoadA8(cpu, 0x2eu);
        OpSta(memory, cpu, WRAM_FIELD_DESTINATION_Y);
        LoadA8(cpu, 0x40u);
        OpSta(memory, cpu, WRAM_FIELD_DESTINATION_PARAMETERS);
        LoadA8(cpu, 0xf1u);
        OpSta(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_ID));
        LoadA8(cpu, 0x01u);
        OpTestBits(memory, cpu, OpAbs(cpu, WRAM_FIELD_MAP_FLAGS), 0);
        return ExecutionReturned(0x839ea1u);                   /* 9EA1 RTL */
    }
    TransferDirectToA(cpu);                                    /* 9EA2 */
    OpSta(memory, cpu, 0x7fe698u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, 0x004214u);                             /* (floor-1)/10 */
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f4fu));
    OpSta(memory, cpu, 0x7fe697u);
    OpAslA(cpu);
    OpTax(cpu);
    OpRepWidths(cpu, 0x20u);                                         /* 9EB7 */
    OpLda(memory, cpu, OpLongX(cpu, 0x839f65u));
    OpSta(memory, cpu, OpDp(cpu, 0x56u));
    OpLda(memory, cpu, OpLongX(cpu, 0x839f49u));
    OpSta(memory, cpu, 0x7fe69bu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f5fu));
    OpSta(memory, cpu, 0x7fe69du);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f59u));
    OpSta(memory, cpu, 0x7fe699u);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f6bu));
    OpSta(memory, cpu, 0x7fe6a2u);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f7fu));
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9EE5 */
    OpAndValue(cpu, 0x0006u);
    OpSta(memory, cpu, OpDp(cpu, 0x58u));
    OpLsrA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x58u));
    OpAdc(memory, cpu, OpDp(cpu, 0x54u));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f7fu));
    OpSta(memory, cpu, 0x7fe69fu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f80u));
    OpSta(memory, cpu, 0x7fe6a0u);
    OpLda(memory, cpu, 0x7fe697u);                             /* 9F04 */
    OpAslA(cpu);
    OpAdc(memory, cpu, 0x7fe697u);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f73u));
    OpSta(memory, cpu, WRAM_CAVE_MAP_HEADER_POINTER);
    OpSepWidths(cpu, 0x20u);                                         /* 9F16 */
    OpLda(memory, cpu, OpLongX(cpu, 0x839f75u));
    OpSta(memory, cpu, WRAM_CAVE_MAP_HEADER_BANK);
    OpLda(memory, cpu, OpDp(cpu, 0x56u));                      /* music */
    OpCmp(memory, cpu, OpAbs(cpu, 0x099du));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbs(cpu, 0x099du));               /* 9F27 */
        SimulateJslFrame(memory, cpu, 0x83u, 0x9f2du);
        if (!child(child_context, cpu, 0x8093feu, 0x839f2au, 3u))
            return CaveChildUnwound(0x839f2au);
    }
    SimulateJslFrame(memory, cpu, 0x83u, 0x9f31u);             /* 9F2E */
    if (!child(child_context, cpu, 0x83b5d3u, 0x839f2eu, 3u))
        return CaveChildUnwound(0x839f2eu);
    SimulateJsrFrame(memory, cpu, 0x9f34u);                    /* 9F32 */
    result = Lufia2CaveBuildFloor(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9F35 */
    OpIncA(cpu);
    OpSta(memory, cpu, CAVE_FLOOR_LONG);
    return ExecutionReturned(0x839f3eu);                       /* 9F3E RTL */
}
