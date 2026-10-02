/* Ancient Cave floor entry ($83:9E31) and builder ($83:9013). */

#include <stdbool.h>

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

/* The cave random helpers take their limit in A and answer in A. */
static uint8_t CaveRandomBelowOf(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint16_t site, uint8_t limit) {
    LoadA8(cpu, limit);
    Lufia2CaveRandomBelow(memory, cpu, site);
    return A8(cpu);
}

static uint8_t CaveRandomMeanOf(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                uint16_t site, uint8_t limit) {
    LoadA8(cpu, limit);
    Lufia2CaveRandomMean(memory, cpu, site);
    return A8(cpu);
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
    const Lufia2Wram dp = WramViewOfCaller(memory, cpu);
    uint8_t cursor_column = 1u;
    uint8_t cursor_row = 4u;
    uint8_t room_count = 0u;
    uint8_t tallest = 0u;
    uint8_t width = 0u;

    OpSepWidths(cpu, 0x20u);                                         /* 91E7 */
    OpSetDataBank(memory, cpu, 0x83u);
    WramWrite(dp, CAVE_DP_CURSOR_COLUMN, cursor_column);
    WramWrite(dp, CAVE_DP_CURSOR_ROW, cursor_row);
    WramWrite(dp, CAVE_DP_ROOM_COUNT, room_count);
    WramWrite(dp, CAVE_DP_TALLEST_ROOM, tallest);
    for (;;) {
        /* A room starts at the cursor column, up to one row above or below
         * the cursor row, and must fit inside the 8-column, 14-row area. */
        const uint8_t column = cursor_column;
        const uint8_t row =
            (uint8_t)(CaveRandomBelowOf(memory, cpu, 0x91ffu, 3u) - 1u + cursor_row);

        WramWrite(dp, CAVE_DP_ROOM_COLUMN, column);
        WramWrite(dp, CAVE_DP_ROOM_ROW, row);
        width = (uint8_t)(CaveRandomMeanOf(memory, cpu, 0x920au, 4u) + 1u);
        WramWrite(dp, CAVE_DP_ROOM_WIDTH, width);
        if ((uint8_t)(width + column) < 9u) {
            const uint8_t height =
                (uint8_t)(CaveRandomMeanOf(memory, cpu, 0x9219u, 4u) + 1u);

            WramWrite(dp, CAVE_DP_ROOM_HEIGHT, height);
            if ((uint8_t)(height + row) < 15u) {
                if (height >= tallest) {
                    tallest = height;
                    WramWrite(dp, CAVE_DP_TALLEST_ROOM, tallest);
                }
                /* A room smaller than three cells in all is widened. */
                if ((uint8_t)(width + height) < 3u) {
                    ++width;
                    WramWrite(dp, CAVE_DP_ROOM_WIDTH, width);
                }
                ++room_count;
                WramWrite(dp, CAVE_DP_ROOM_COUNT, room_count);
                WramWrite(dp, CAVE_DP_FILL_ROOM, room_count);
                if (room_count >= 8u)
                    return;                                    /* 9266 */
                Lufia2CaveFillRoom(memory, cpu, 0x9244u);
            }
        }
        cursor_column = (uint8_t)(cursor_column + width);
        WramWrite(dp, CAVE_DP_CURSOR_COLUMN, cursor_column);
        if (cursor_column < 9u)
            continue;
        /* The row is full: start the next one a few rows down. */
        cursor_column = 1u;
        WramWrite(dp, CAVE_DP_CURSOR_COLUMN, cursor_column);
        cursor_row =
            (uint8_t)(CaveRandomMeanOf(memory, cpu, 0x9258u, 4u) + 2u + cursor_row);
        WramWrite(dp, CAVE_DP_CURSOR_ROW, cursor_row);
        if (cursor_row >= 15u)
            return;
    }
}

/* Opens the cell between a room cell and a marked side cell's diagonal
 * neighbour. An empty diagonal cell is linked together with the cell between
 * it; a marked one just lends its mark to the cell between. */
static void CaveOpenCorridor(Lufia2Wram wram, uint16_t cell, uint8_t marked,
                             int diagonal, int between) {
    uint8_t beyond;

    WramWrite(wram, CAVE_DP_CORRIDOR_ID, marked);
    beyond = WramReadAt(wram, CAVE_ROOM_GRID + diagonal, cell);
    if (beyond == 0u) {
        const uint8_t joined = (uint8_t)(marked | CAVE_CELL_LINKED);

        WramWriteAt(wram, CAVE_ROOM_GRID + diagonal, cell, joined);
        WramWriteAt(wram, CAVE_ROOM_GRID + between, cell, joined);
    } else if ((beyond & CAVE_CELL_MARKED) != 0u) {
        WramWriteAt(wram, CAVE_ROOM_GRID + between, cell, beyond);
    }
}

/* An empty side cell takes the room's number, flagged as linked, when either
 * cell diagonally beside it is occupied. */
static void CaveLinkSideCell(Lufia2Wram wram, uint16_t cell, uint8_t room, int side,
                             int upper, int lower) {
    if (WramReadAt(wram, CAVE_ROOM_GRID + upper, cell) != 0u ||
        WramReadAt(wram, CAVE_ROOM_GRID + lower, cell) != 0u)
        WramWriteAt(wram, CAVE_ROOM_GRID + side, cell,
                    (uint8_t)(room | CAVE_CELL_LINKED));
}

/* $83:9266-$83:9387: merge and link rooms, open corridors. */
static void CaveLinkFloor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    uint16_t cell;
    uint8_t passes;

    OpSetDataBank(memory, cpu, 0x7fu);                         /* 9266 */
    wram = WramViewOfCaller(memory, cpu);
    LoadA8(cpu, 0x08u);
    WramWrite(wram, CAVE_DP_MERGE_PASSES, 0x08u);
    do {
        Lufia2CaveMergeRoom(memory, cpu, 0x926eu);             /* 926E */
        passes = (uint8_t)(WramRead(wram, CAVE_DP_MERGE_PASSES) - 1u);
        WramWrite(wram, CAVE_DP_MERGE_PASSES, passes);
    } while (passes != 0u);
    Lufia2CaveCountCells(memory, cpu, 0x9275u);
    Lufia2CavePickCell(memory, cpu, 0x9278u);
    OpWriteX(memory, cpu, OpAbs(cpu, CAVE_START_COLUMN), cpu->x);
    Lufia2CaveLinkRooms(memory, cpu, 0x927eu);

    /* Ties between neighbouring cells: a cell without link flags marks the
     * empty cell beside it, or reaches past the marked side cell to open the
     * cell above or below it. */
    for (cell = 0xffu; cell >= 0x10u; --cell) {
        const uint8_t room = WramReadAt(wram, CAVE_ROOM_GRID, cell);
        const uint8_t row = (uint8_t)(cell & 0xf0u);
        uint8_t right;
        uint8_t left;

        if (room == 0u || (room & CAVE_CELL_FLAGS) != 0u)
            continue;
        WramWrite(wram, CAVE_DP_CELL_VALUE, room);
        right = WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_RIGHT, cell);
        if (right == 0u) {
            CaveLinkSideCell(wram, cell, room, CAVE_CELL_RIGHT, CAVE_CELL_UP_RIGHT,
                             CAVE_CELL_DOWN_RIGHT);
            continue;
        }
        left = WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_LEFT, cell);
        if (left == 0u) {
            CaveLinkSideCell(wram, cell, room, CAVE_CELL_LEFT, CAVE_CELL_UP_LEFT,
                             CAVE_CELL_DOWN_LEFT);
            continue;
        }
        if (WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN, cell) == 0u) {
            /* Nothing below: reach down past a marked side cell. */
            if (row >= 0xe0u)
                continue;
            if ((right & CAVE_CELL_MARKED) != 0u)
                CaveOpenCorridor(wram, cell, right, CAVE_CELL_DOWN_RIGHT,
                                 CAVE_CELL_DOWN);
            else if ((left & CAVE_CELL_MARKED) != 0u)
                CaveOpenCorridor(wram, cell, left, CAVE_CELL_DOWN_LEFT, CAVE_CELL_DOWN);
        } else if (WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_UP, cell) == 0u) {
            /* Nothing above: the same, upwards. */
            if (row < 0x30u)
                continue;
            if ((right & CAVE_CELL_MARKED) != 0u)
                CaveOpenCorridor(wram, cell, right, CAVE_CELL_UP_RIGHT, CAVE_CELL_UP);
            else if ((left & CAVE_CELL_MARKED) != 0u)
                CaveOpenCorridor(wram, cell, left, CAVE_CELL_UP_LEFT, CAVE_CELL_UP);
        }
    }
    Lufia2CaveClearVisited(memory, cpu, 0x936cu);
    Lufia2CaveLinkRooms(memory, cpu, 0x936fu);

    /* Only the cells the second pass reached keep a room number. */
    for (cell = 0xffu; cell >= 0x10u; --cell) {
        const uint8_t room = WramReadAt(wram, CAVE_ROOM_GRID, cell);

        if (room == 0u)
            continue;
        WramWriteAt(
            wram, CAVE_ROOM_GRID, cell,
            (uint8_t)(((room & CAVE_CELL_MARKED) != 0u ? room
                                                       : (uint8_t)wram.direct_page) &
                      CAVE_CELL_ID_MASK));
    }
    cpu->x = 0x000fu;
    OpCpx(cpu, 0x0010u);
}

/* $83:9388-$83:940D: keep one random link per room pair. The link list holds
 * cell numbers; links whose cell and cell below hold the same pair of room
 * numbers join the same two rooms. All but one random member of each such
 * group are cleared, and the list is closed with $FF. The direct page is the
 * zero page here, as on every call of the original. */
static void CaveDedupeLinks(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const uint16_t link_count = WramRead16(wram, CAVE_DP_LINK_COUNT);
    uint16_t index;

    for (index = 0; index < link_count; ++index) {
        const uint8_t first = WramReadAt(wram, CAVE_LINKS, index);

        WramWrite16(wram, CAVE_DP_LINK_INDEX, index);
        if (first != 0u) {
            const uint8_t upper = WramReadAt(wram, CAVE_ROOM_GRID, first);
            const uint8_t lower =
                WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN, first);
            uint16_t group = 1u;
            uint16_t other;

            /* Collect the group: this link first, then every later link
             * between the same two rooms. */
            WramWrite(wram, CAVE_LINK_SCRATCH, first);
            WramWrite(wram, CAVE_DP_LINK_KEY, upper);
            WramWrite(wram, CAVE_DP_LINK_KEY + 1u, lower);
            WramWrite16(wram, CAVE_DP_SCRATCH_COUNT, group);
            for (other = (uint16_t)(index + 1u); other < link_count; ++other) {
                const uint8_t link = WramReadAt(wram, CAVE_LINKS, other);

                if (WramReadAt(wram, CAVE_ROOM_GRID, link) != upper ||
                    WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN, link) != lower)
                    continue;
                WramWriteAt(wram, CAVE_LINK_SCRATCH, group, link);
                ++group;
                WramWrite16(wram, CAVE_DP_SCRATCH_COUNT, group);
            }
            if ((uint8_t)group != 1u) {
                /* Spare one member at random, then clear the rest. */
                const uint8_t spared =
                    CaveRandomBelowOf(memory, cpu, 0x93d6u, (uint8_t)group);
                uint16_t link_slot;

                WramWriteAt(wram, CAVE_LINK_SCRATCH, spared, 0xffu);
                for (link_slot = 0; link_slot < link_count; ++link_slot) {
                    const uint8_t link = WramReadAt(wram, CAVE_LINKS, link_slot);
                    uint16_t member;

                    for (member = 0; member < group; ++member) {
                        if (link == WramReadAt(wram, CAVE_LINK_SCRATCH, member)) {
                            WramWriteAt(wram, CAVE_LINKS, link_slot, 0u);
                            break;
                        }
                    }
                }
            }
        }
        index = WramRead16(wram, CAVE_DP_LINK_INDEX);
    }
    WramWriteAt(wram, CAVE_LINKS, (uint8_t)link_count, 0xffu);
}

/* A random non-empty cell. Returns whether one was found; `cell` is the
 * helper's answer in A either way. */
static bool CavePickCellOf(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t site, uint8_t *cell) {
    Lufia2CavePickCell(memory, cpu, site);
    *cell = A8(cpu);
    return cpu->carry;
}

/* Tile origin of a cell: the helper leaves the column in B and the row in A. */
static void CaveCellOrigin(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t site, uint8_t cell, uint8_t *column, uint8_t *row) {
    LoadA8(cpu, cell);
    Lufia2CaveCellPosition(memory, cpu, site);
    *row = A8(cpu);
    *column = (uint8_t)(cpu->accumulator >> 8);
}

/* $83:940E-$83:949C: the start cell, link marks and stairs. The player
 * starts on a random room, three tiles down and two across from its origin;
 * every room that has a link is marked; the stairs go in another random
 * room, and one floor in sixteen gets a second pair of link marks. */
static void CavePlaceStart(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram low = WramViewLong(memory);
    uint8_t cell;
    uint8_t column;
    uint8_t row;
    uint16_t link;

    Lufia2CaveCountCells(memory, cpu, 0x940eu);                /* 940E */
    CavePickCellOf(memory, cpu, 0x9411u, &cell);
    CaveCellOrigin(memory, cpu, 0x9414u, cell, &column, &row);
    WramWrite(low, WRAM_FIELD_DESTINATION_Y, (uint8_t)(row + 3u));
    WramWrite(wram, CAVE_START_ROW_LONG, (uint8_t)(row + 3u));
    WramWrite(low, WRAM_FIELD_DESTINATION_X, (uint8_t)(column + 2u));
    WramWrite(wram, CAVE_START_COLUMN_LONG, (uint8_t)(column + 2u));
    /* Mark both cells of every linked pair. */
    for (link = 0;; ++link) {
        const uint8_t linked = WramReadAt(wram, CAVE_LINKS, link);

        if (linked == 0xffu)
            break;
        if (linked != 0u) {
            WramWriteAt(
                wram, CAVE_ROOM_GRID, linked,
                (uint8_t)(WramReadAt(wram, CAVE_ROOM_GRID, linked) | CAVE_CELL_MARKED));
            WramWriteAt(
                wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN, linked,
                (uint8_t)(WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN, linked) |
                          CAVE_CELL_MARKED));
        }
    }
    /* The stairs: another room, or after a sweep that clears the visited
     * bits, whichever the second search finds. */
    if (!CavePickCellOf(memory, cpu, 0x944eu, &cell)) {
        Lufia2CaveClearVisited(memory, cpu, 0x9453u);
        CavePickCellOf(memory, cpu, 0x9456u, &cell);
    }
    CaveCellOrigin(memory, cpu, 0x9459u, cell, &column, &row);
    WramWrite(wram, CAVE_STAIR_ROW_LONG, (uint8_t)(row + 3u));
    WramWrite(wram, CAVE_STAIR_COLUMN_LONG, (uint8_t)(column + 2u));
    WramWrite(wram, CAVE_LINK_MARK_A_LONG, 0xffu);
    WramWrite(wram, CAVE_LINK_MARK_B_LONG, 0xffu);
    CaveRandomByte(memory, cpu, 0x9472u);
    if (A8(cpu) < 0x10u && CavePickCellOf(memory, cpu, 0x947au, &cell)) {
        CaveCellOrigin(memory, cpu, 0x947fu, cell, &column, &row);
        WramWrite(wram, CAVE_LINK_MARK_B_LONG, (uint8_t)(row + 3u));
        WramWrite(wram, CAVE_LINK_MARK_A_LONG, (uint8_t)(column + 3u));
    }
    Lufia2CaveClearVisited(memory, cpu, 0x948fu);              /* 948F */
    for (link = 0; link < 0x14u; ++link)
        WramWriteAt(wram, CAVE_OBJECT_COLUMNS_LONG, link, 0u);
}

/* $83:949D-$83:9514: one floor in sixteen gets a 2x2 treasure room. The first
 * 2x2 block of identical cells (searched from the second row, then thinned out
 * by a coin flip per candidate) becomes the room: thirteen objects are
 * scattered within eight tiles of its corner and eight chests are added. */
static void CaveTreasureRoom(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t cell;

    CaveRandomByte(memory, cpu, 0x949du);                      /* 949D */
    if (A8(cpu) >= CAVE_TREASURE_ROOM_ODDS)
        return;
    WramWrite(wram, CAVE_DP_SCRATCH_COUNT + 1u, CAVE_TREASURE_MARK);
    for (cell = CAVE_FIRST_ROOM_CELL; cell < CAVE_TREASURE_LAST_CELL; ++cell) {
        const uint8_t room = WramReadAt(wram, CAVE_ROOM_GRID, cell);

        if (room != 0u &&
            WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_RIGHT, cell) == room &&
            WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN, cell) == room &&
            WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT, cell) == room &&
            (CaveRandomByte(memory, cpu, 0x94c0u), A8(cpu) < CAVE_TREASURE_CELL_ODDS)) {
            uint8_t column;
            uint8_t row;
            int index;

            WramWrite(wram, CAVE_TREASURE_CELL, (uint8_t)cell);
            CaveCellOrigin(memory, cpu, 0x94ccu, (uint8_t)cell, &column, &row);
            WramWrite(wram, CAVE_DP_CURSOR_ROW, row);
            WramWrite(wram, CAVE_DP_CURSOR_COLUMN, column);
            for (index = CAVE_TREASURE_OBJECTS - 1; index >= 0; --index) {
                WramWrite(wram, CAVE_DP_TILE_COLUMN,
                          (uint8_t)(CaveRandomBelowOf(memory, cpu, 0x94dau,
                                                      CAVE_TREASURE_SPREAD) +
                                    WramRead(wram, CAVE_DP_CURSOR_COLUMN) +
                                    CAVE_TREASURE_COLUMN_OFFSET));
                WramWrite(wram, CAVE_DP_TILE_ROW,
                          (uint8_t)(CaveRandomBelowOf(memory, cpu, 0x94e6u,
                                                      CAVE_TREASURE_SPREAD) +
                                    WramRead(wram, CAVE_DP_CURSOR_ROW) +
                                    CAVE_TREASURE_ROW_OFFSET));
                Lufia2CaveNearStartOrPlaced(memory, cpu, 0x94f0u);
                if (!cpu->carry)
                    Lufia2CaveAddObject(memory, cpu, 0x94f5u);
            }
            for (index = CAVE_TREASURE_CHESTS; index > 0; --index) {
                LoadA8(cpu, WramRead(wram, CAVE_TREASURE_CELL));
                Lufia2CaveAddChest(memory, cpu, 0x9503u);
            }
            break;
        }
    }
    Lufia2CaveClearVisited(memory, cpu, 0x9512u);              /* 9512 */
}

/* $83:9515-$83:959C: four to seven objects at random cells, then a chest in
 * each unmarked 2x2 room until the chest limit is reached. */
static void CaveObjectsAndChests(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    uint16_t target;
    uint16_t index;
    uint16_t cell;

    target =
        (uint8_t)(CaveRandomBelowOf(memory, cpu, 0x9517u, CAVE_EXTRA_ROOM_OBJECTS) +
                  CAVE_MIN_ROOM_OBJECTS);
    WramWrite16(wram, CAVE_DP_OBJECT_TARGET, target);
    for (index = 0;;) {
        uint8_t room_cell;
        uint8_t column;
        uint8_t row;

        /* A random offset within the room, then the room's own origin. */
        WramWrite(wram, CAVE_DP_TILE_COLUMN,
                  CaveRandomBelowOf(memory, cpu, 0x9526u, CAVE_OBJECT_JITTER));
        WramWrite(wram, CAVE_DP_TILE_ROW,
                  CaveRandomBelowOf(memory, cpu, 0x952du, CAVE_OBJECT_JITTER));
        WramWrite16(wram, CAVE_DP_OBJECT_INDEX, index);
        if (CavePickCellOf(memory, cpu, 0x9534u, &room_cell)) {
            CaveCellOrigin(memory, cpu, 0x953bu, room_cell, &column, &row);
            WramWrite(wram, CAVE_DP_TILE_ROW,
                      (uint8_t)(row + WramRead(wram, CAVE_DP_TILE_ROW) + 1u + 2u));
            WramWrite(wram, CAVE_DP_TILE_COLUMN,
                      (uint8_t)(column + WramRead(wram, CAVE_DP_TILE_COLUMN) + 1u));
            Lufia2CaveNearStartOrPlaced(memory, cpu, 0x954du);
            if (!cpu->carry)
                Lufia2CaveAddObject(memory, cpu, 0x9552u);
        }
        index = (uint16_t)(WramRead16(wram, CAVE_DP_OBJECT_INDEX) + 1u);
        if (index >= WramRead16(wram, CAVE_DP_OBJECT_TARGET))
            break;
    }
    /* A chest in each room that is a plain 2x2 block, marking it as used. */
    for (cell = CAVE_FIRST_ROOM_CELL; cell < CAVE_GRID_ROWS_END; ++cell) {
        const uint8_t room = WramReadAt(wram, CAVE_ROOM_GRID, cell);

        if (room != 0u && !(room & 0x80u) &&
            WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_RIGHT, cell) == room &&
            WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN, cell) == room &&
            WramReadAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT, cell) == room) {
            const uint8_t marked = (uint8_t)(room | CAVE_CELL_MARKED);

            WramWriteAt(wram, CAVE_ROOM_GRID, cell, marked);
            WramWriteAt(wram, CAVE_ROOM_GRID + CAVE_CELL_RIGHT, cell, marked);
            WramWriteAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN, cell, marked);
            WramWriteAt(wram, CAVE_ROOM_GRID + CAVE_CELL_DOWN_RIGHT, cell, marked);
            LoadA8(cpu, (uint8_t)cell);
            Lufia2CaveAddChest(memory, cpu, 0x9582u);
            if (!cpu->carry) {
                uint8_t attempts;

                if (WramRead(wram, CAVE_CHEST_COUNT) >= CAVE_MAX_CHESTS)
                    break;
                /* Never ends the loop: the chest count reaches 8 first. */
                attempts = (uint8_t)(WramRead(wram, CAVE_DP_CHEST_ATTEMPTS) + 1u);
                WramWrite(wram, CAVE_DP_CHEST_ATTEMPTS, attempts);
                if (attempts >= CAVE_MAX_CHESTS)
                    break;
            }
        }
    }
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

/* Search a map-header list for `key` in set number `set` of list `list`.
 * Returns 0 when the search hands off. Otherwise `missing` says whether the
 * key was absent and `entry` is the record's offset in the list buffer. */
static uint8_t CaveFindListEntry(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                                 uint16_t site, uint8_t list, uint8_t set, uint16_t key,
                                 bool *missing, uint16_t *entry) {
    cpu->x = set;
    LoadA8(cpu, list);
    ExchangeAccumulatorBytes(cpu);
    OpTxa(cpu);
    OpLdx(cpu, key);
    if (!CaveListSearch(memory, cpu, site))
        return 0;
    *missing = cpu->carry;
    *entry = cpu->x;
    return 1;
}

/* $83:96F5-$83:9752: read the four tile sets from list 5 into the record
 * table. Each set found takes one word from the map block buffer at its
 * position for each of four tile columns; sets that are missing keep their
 * $FF marks. Returns 0 when the list search hands off. */
static uint8_t CaveReadTileSets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram work = WramViewLong(memory);
    uint16_t set_offset = 0u;
    uint8_t set;

    WramWrite16(wram, CAVE_DP_SET_OFFSET, set_offset);
    for (set = 0; set < CAVE_TILE_SET_COUNT; ++set) {
        uint16_t entry;
        uint16_t block;
        uint16_t corner;
        bool missing;

        set_offset = WramRead16(wram, CAVE_DP_SET_OFFSET);
        WramWriteAt(wram, CAVE_SET_TABLE, set_offset, 0xffu);
        WramWriteAt(wram, CAVE_SET_TABLE + 1u, set_offset, 0xffu);
        if (!CaveFindListEntry(memory, cpu, 0x970eu, CAVE_LIST_TILE_SETS, set,
                               CAVE_TILE_SET_KEY, &missing, &entry))
            return 0;
        if (missing)
            continue;
        WramWrite(wram, CAVE_DP_TILE_COLUMN,
                  WramReadAt(work, CAVE_LIST_ENTRY_LONG + 1u, entry));
        WramWrite(wram, CAVE_DP_TILE_ROW,
                  WramReadAt(work, CAVE_LIST_ENTRY_LONG + 2u, entry));
        Lufia2CaveBlockOffsetX(memory, cpu, 0x9720u);
        block = cpu->x;
        WramWrite16At(wram, CAVE_SET_TABLE + 0x100u, set_offset,
                      WramRead16At(work, CAVE_BLOCK_BUFFER_LONG + 0x0cu, block));
        WramWrite16At(wram, CAVE_SET_TABLE + 0x200u, set_offset,
                      WramRead16At(work, CAVE_BLOCK_BUFFER_LONG + 0x0eu, block));
        WramWrite16At(wram, CAVE_SET_TABLE + 0x300u, set_offset,
                      WramRead16At(work, CAVE_BLOCK_BUFFER_LONG + 0x10u, block));
        corner = WramRead16At(work, CAVE_BLOCK_BUFFER_LONG + 0x0au, block);
        WramWrite16At(wram, CAVE_SET_TABLE, set_offset, corner);
        WramWrite16At(work, CAVE_TILE_SET_WORDS_LONG, set_offset, corner);
        set_offset = (uint16_t)(set_offset + 2u);
        WramWrite16(wram, CAVE_DP_SET_OFFSET, set_offset);
    }
    return 1;
}

/* $83:9753-$83:97E7: for every occupied cell, replace the tile indices of its
 * 6x6 block that match a set's keys with the set's tiles, keeping the flag
 * bits. */
static void CaveApplyTileSets(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    /* The tile numbers the sets replace are kept as words at $22..$28 by
     * CaveReadTileSets; the replacements are copied to $11..$17 below. */
    static const uint8_t kKeys[4] = {0x22u, 0x24u, 0x26u, 0x28u};
    static const uint8_t kTiles[4] = {0x11u, 0x13u, 0x15u, 0x17u};
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    /* The map layers start below $100, so they need the full address. */
    const uint32_t layer = ((uint32_t)cpu->data_bank << 16) | CAVE_MAP_LAYER_1;
    uint16_t cell;

    for (cell = CAVE_FIRST_ROOM_CELL; cell < CAVE_GRID_ROWS_END; ++cell) {
        const uint8_t room = WramReadAt(wram, CAVE_ROOM_GRID, cell);
        uint8_t column;
        uint8_t row;
        uint8_t set;

        if (room == 0u)
            continue;
        WramWrite(wram, CAVE_DP_TILE_COUNT, room);
        CaveCellOrigin(memory, cpu, 0x9762u, (uint8_t)cell, &column, &row);
        WramWrite(wram, CAVE_DP_TILE_ROW, row);
        WramWrite(wram, CAVE_DP_TILE_COLUMN, column);
        Lufia2CaveTileOffsetY(memory, cpu, 0x976au);
        set = WramRead(wram, CAVE_DP_TILE_COUNT) & CAVE_CELL_SET_MASK;
        if (set != 0u) {
            /* The room's set number picks a 256-byte variant of the set
             * table; replace its four tile numbers wherever they occur in
             * the room's block of the first map layer. */
            const uint16_t variant = (uint16_t)((uint16_t)set << 8);
            uint16_t tile = cpu->y;
            unsigned i;

            for (i = 0; i < 4; ++i)
                WramWrite16(wram, kTiles[i],
                            WramRead16At(wram, CAVE_SET_TABLE + 2u * i, variant));
            WramWrite16(wram, CAVE_DP_ROW_COUNT, CAVE_BLOCK_TILE_SPAN);
            for (;;) {
                WramWrite16(wram, CAVE_DP_TILE_COUNT, CAVE_BLOCK_TILE_SPAN);
                for (;;) {
                    const uint16_t index =
                        WramRead16At(wram, layer, tile) & CAVE_TILE_INDEX_MASK;

                    for (i = 0; i < 4; ++i) {
                        if (index == WramRead16(wram, kKeys[i])) {
                            WramWrite16(wram, CAVE_DP_TILE_BITS,
                                        WramRead16(wram, kTiles[i]));
                            WramWrite16At(
                                wram, layer, tile,
                                (uint16_t)((WramRead16At(wram, layer, tile) &
                                            CAVE_TILE_FLAG_MASK) |
                                           WramRead16(wram, CAVE_DP_TILE_BITS)));
                            break;
                        }
                    }
                    if (WramStep16(wram, CAVE_DP_TILE_COUNT, -1) == 0u)
                        break;
                    tile = (uint16_t)(tile + 2u);
                }
                if (WramStep16(wram, CAVE_DP_ROW_COUNT, -1) == 0u)
                    break;
                tile = (uint16_t)(tile + CAVE_MAP_ROW_SKIP);
                if (tile == 0u)
                    break;
            }
        }
    }
}

/* $83:97E8-$83:9868: read the five decoration sets from list 10 into the
 * record table, one 256-byte block per variant. A set that is missing gets a
 * zero word and an $FFFF key. Returns 0 when the list search hands off. */
static uint8_t CaveReadDecorations(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram work = WramViewLong(memory);
    uint16_t set_offset = 0u;
    uint8_t set;

    WramWrite16(wram, CAVE_DP_SET_OFFSET, set_offset);
    for (set = CAVE_DECORATION_FIRST_SET;
         set < CAVE_DECORATION_FIRST_SET + CAVE_DECORATION_SETS; ++set) {
        uint16_t entry;
        bool missing;

        cpu->x = set; /* The index lives on the stack, where a runaway
                         record can overwrite it. */
        OpPushX(memory, cpu);
        if (!CaveFindListEntry(memory, cpu, 0x97f7u, CAVE_LIST_DECORATIONS, set,
                               CAVE_DECORATION_KEY, &missing, &entry))
            return 0;
        set_offset = WramRead16(wram, CAVE_DP_SET_OFFSET);
        if (missing) {
            WramWrite16At(wram, CAVE_SET_TABLE, set_offset, 0u);
            WramWrite16At(wram, CAVE_SET_TABLE + 0x100u, set_offset, 0xffffu);
        } else {
            const uint8_t rows = WramReadAt(work, CAVE_LIST_ENTRY_LONG + 8u, entry);
            uint32_t carry;
            uint16_t block;
            uint16_t variant = set_offset;

            WramWrite(wram, CAVE_DP_TILE_COLUMN,
                      WramReadAt(work, CAVE_LIST_ENTRY_LONG + 2u, entry));
            WramWrite(wram, CAVE_DP_TILE_ROW,
                      WramReadAt(work, CAVE_LIST_ENTRY_LONG + 3u, entry));
            WramWriteAt(wram, CAVE_SET_TABLE, set_offset, rows);
            WramWrite16(wram, CAVE_DP_ROW_COUNT, rows);
            WramWriteAt(
                wram, CAVE_SET_TABLE + 1u, set_offset,
                (uint8_t)(WramReadAt(work, CAVE_LIST_ENTRY_LONG + 9u, entry) - 1u));
            Lufia2CaveBlockOffsetX(memory, cpu, 0x982cu);
            block = cpu->x;
            /* One variant per row, the first step continuing the carry the
             * block offset left behind. */
            carry = cpu->carry;
            do {
                const uint32_t next =
                    (uint32_t)variant + CAVE_SET_VARIANT_STRIDE + carry;

                variant = (uint16_t)next;
                carry = next >> 16;
                WramWrite16At(
                    wram, CAVE_SET_TABLE, variant,
                    WramRead16At(work, CAVE_BLOCK_BUFFER_LONG + 0x0au, block));
                WramWrite16At(
                    wram, CAVE_SET_TABLE + 2u, variant,
                    WramRead16At(work, CAVE_BLOCK_BUFFER_LONG + 0x6au, block));
                WramWrite16At(
                    wram, CAVE_SET_TABLE + 4u, variant,
                    WramRead16At(work, CAVE_BLOCK_BUFFER_LONG + 0xfceu, block));
                WramWrite16At(
                    wram, CAVE_SET_TABLE + 6u, variant,
                    WramRead16At(work, CAVE_BLOCK_BUFFER_LONG + 0x102eu, block));
                block = (uint16_t)(block + 2u);
            } while (WramStep16(wram, CAVE_DP_ROW_COUNT, -1) != 0u);
        }
        set_offset = (uint16_t)(WramRead16(wram, CAVE_DP_SET_OFFSET) + CAVE_SET_SIZE);
        WramWrite16(wram, CAVE_DP_SET_OFFSET, set_offset);
        OpPullX(memory, cpu);
        set = (uint8_t)cpu->x;
    }
    return 1;
}

/* $83:9869-$83:992F: scatter decorations over every occupied cell's block. A
 * tile that matches a set's source tile is, with odds of 48 in 256, swapped
 * for a random variant of that set, on both map layers and on the row below
 * when the set is two tiles tall. */
static void CaveDecorate(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    /* The map layers start below $100, so they need the full address. */
    const uint32_t layer1 = ((uint32_t)cpu->data_bank << 16) | CAVE_MAP_LAYER_1;
    const uint32_t layer2 = ((uint32_t)cpu->data_bank << 16) | CAVE_MAP_LAYER_2;
    uint16_t cell;

    for (cell = CAVE_FIRST_ROOM_CELL; cell < CAVE_GRID_ROWS_END; ++cell) {
        /* The cell number waits on the stack while the helpers run. */
        cpu->x = cell;
        OpPushX(memory, cpu);
        LoadA8(cpu, WramReadAt(wram, CAVE_ROOM_GRID, cell));
        if (cpu->accumulator & 0xffu) {
            uint8_t column;
            uint8_t row;
            uint16_t tile;

            CaveCellOrigin(memory, cpu, 0x9876u, (uint8_t)cell, &column, &row);
            WramWrite(wram, CAVE_DP_TILE_ROW, row);
            WramWrite(wram, CAVE_DP_TILE_COLUMN, column);
            Lufia2CaveTileOffsetY(memory, cpu, 0x987eu);
            tile = cpu->y;
            OpRepWidths(cpu, 0x20u);
            WramWrite16(wram, CAVE_DP_ROW_COUNT, CAVE_BLOCK_TILE_SPAN);
            for (;;) {
                LoadA16(cpu, CAVE_BLOCK_TILE_SPAN);
                WramWrite16(wram, CAVE_DP_TILE_COUNT, CAVE_BLOCK_TILE_SPAN);
                for (;;) {
                    uint16_t index;
                    uint16_t set;

                    /* The accumulator keeps whatever the previous tile left
                     * in its high byte, and the 16-bit odds comparison
                     * sees it, so every path below leaves it as the code
                     * did. */
                    cpu->y = tile;
                    CaveRandomByte(memory, cpu, 0x988du);
                    if (cpu->accumulator >= CAVE_DECORATION_ODDS)
                        goto step;
                    index = WramRead16At(wram, layer2, tile) & CAVE_TILE_INDEX_MASK;
                    if (index != 0u) {
                        cpu->accumulator = index;
                        goto step;
                    }
                    index = WramRead16At(wram, layer1, tile) & CAVE_TILE_INDEX_MASK;
                    WramWrite16(wram, CAVE_DP_TILE_BITS, index);
                    for (set = 0;
                         index != WramRead16At(wram, CAVE_SET_TABLE + 0x100u, set);
                         set = (uint16_t)(set + CAVE_SET_SIZE)) {
                        if (set + CAVE_SET_SIZE >=
                            CAVE_DECORATION_SETS * CAVE_SET_SIZE) {
                            cpu->accumulator = (uint16_t)(set + CAVE_SET_SIZE);
                            cpu->x = cpu->accumulator;
                            goto step;
                        }
                    }
                    cpu->x = set;
                    {
                        /* A random variant of the set: its first byte is the
                         * variant count, its second the extra rows. */
                        uint8_t variant;

                        WramWrite16(wram, CAVE_DP_TILE_BITS, set);
                        OpSepWidths(cpu, 0x20u);
                        WramWrite(wram, CAVE_DP_EXTRA_ROWS,
                                  WramReadAt(wram, CAVE_SET_TABLE + 1u, set));
                        WramWrite(wram, CAVE_DP_EXTRA_ROWS + 1u, 0u);
                        variant = (uint8_t)(CaveRandomBelowOf(
                                                memory, cpu, 0x98cbu,
                                                WramReadAt(wram, CAVE_SET_TABLE, set)) +
                                            1u);
                        OpRepWidths(cpu, 0x20u);
                        set = (uint16_t)((variant << 8) + set);
                        cpu->x = set;
                    }
                    WramWrite16At(wram, layer1, tile,
                                  (uint16_t)((WramRead16At(wram, layer1, tile) &
                                              CAVE_TILE_FLAG_MASK) |
                                             WramRead16At(wram, CAVE_SET_TABLE, set)));
                    WramWrite16At(
                        wram, layer2, tile,
                        (uint16_t)((WramRead16At(wram, layer2, tile) &
                                    CAVE_TILE_FLAG_MASK) |
                                   WramRead16At(wram, CAVE_SET_TABLE + 4u, set)));
                    if (WramRead16(wram, CAVE_DP_EXTRA_ROWS) == 0u) {
                        cpu->accumulator = 0u;
                    } else {
                        WramWrite16At(
                            wram, layer1 + CAVE_MAP_ROW_BYTES, tile,
                            (uint16_t)((WramRead16At(wram, layer1 + CAVE_MAP_ROW_BYTES,
                                                     tile) &
                                        CAVE_TILE_FLAG_MASK) |
                                       WramRead16At(wram, CAVE_SET_TABLE + 2u, set)));
                        cpu->accumulator =
                            (uint16_t)((WramRead16At(wram, layer2 + CAVE_MAP_ROW_BYTES,
                                                     tile) &
                                        CAVE_TILE_FLAG_MASK) |
                                       WramRead16At(wram, CAVE_SET_TABLE + 6u, set));
                        WramWrite16At(wram, layer2 + CAVE_MAP_ROW_BYTES, tile,
                                      cpu->accumulator);
                    }
step:
    if (WramStep16(wram, CAVE_DP_TILE_COUNT, -1) == 0u)
        break;
    tile = (uint16_t)(tile + 2u);
}
if (WramStep16(wram, CAVE_DP_ROW_COUNT, -1) == 0u)
    break;
tile = (uint16_t)(tile + CAVE_MAP_ROW_SKIP);
cpu->accumulator = tile;
if (tile == 0u)
    break;
            }
            cpu->y = tile;
        }
        OpSepWidths(cpu, 0x20u);
        OpPullX(memory, cpu);
    }
    cpu->x = CAVE_GRID_ROWS_END;
    OpCpx(cpu, CAVE_GRID_ROWS_END);
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
