/* Glyph buffer and window row upload setup ($80:C56E, C784, C23D). */
#include <stdbool.h>

#include "actor/actor_internal.h"
#include "core/cpu_internal.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/text.h"
#include "system/wram.h"
#include "text/text_internal.h"

/* Slots in the actor tables. */
enum { WINDOW_ACTOR_SLOTS = 40 };

/* Actor state bit that excludes a slot from the search. */
enum { ACTOR_STATE_EXCLUDED = 0x04 };

/* Scratch for the id being looked up. */
enum { DP_WINDOW_ACTOR_ID = 0x54 };

/* $80:BF6F: find the slot of the actor whose id is in A. Slots with the
 * excluded state bit are skipped. On success the slot becomes DP_ACTOR_SLOT,
 * its record offsets are computed and the carry is clear; the carry is set when
 * no slot matches. */
static void TextFindWindowActor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    WramWrite(wram, DP_WINDOW_ACTOR_ID, A8(cpu));
    LoadX16(cpu, 0);
    for (;;) {
        LoadA8(cpu, WramReadAt(wram, WRAM_ACTOR_STATE, cpu->x));
        And8(cpu, ACTOR_STATE_EXCLUDED);
        if (cpu->zero) {
            LoadA8(cpu, WramReadAt(wram, WRAM_ACTOR_ID, cpu->x));
            Compare8(cpu, A8(cpu), WramRead(wram, DP_WINDOW_ACTOR_ID));
            if (cpu->zero) {
                WramWrite16(wram, DP_ACTOR_SLOT, cpu->x);
                SimulateJslFrame(memory, cpu, 0x80u, 0xbf8fu);
                Lufia2ActorRecordOffsets(memory, cpu);
                SimulateRtlFrame(memory, cpu);
                cpu->carry = 0;
                return;
            }
        }
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, WINDOW_ACTOR_SLOTS);
        if (cpu->zero) {
            cpu->carry = 1;
            return;
        }
    }
}

/* Packed window coordinates and the row they turn into. */
enum { DP_WINDOW_COORDINATES = 0x4e, DP_WINDOW_ROW_OFFSET = 0x51 };

/* $80:C557: a packed position (column in the low byte, row in the high byte)
 * to a byte offset in the window tilemap: 64 bytes per row, 2 per column. */
static void TextWindowTileOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    WramWrite16(wram, DP_WINDOW_COORDINATES, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, WramRead(wram, DP_WINDOW_COORDINATES + 1u));
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    LsrA16(cpu);
    LsrA16(cpu);
    WramWrite16(wram, DP_WINDOW_ROW_OFFSET, cpu->accumulator);
    LoadA16(cpu, WramRead16(wram, DP_WINDOW_COORDINATES));
    And16(cpu, 0xffu);
    AslA16(cpu);
    Add16Value(cpu, WramRead16(wram, DP_WINDOW_ROW_OFFSET));
}

/* Border drawing: tile word bit that mirrors the tile, the width of a tilemap
 * row, and the scratch holding the number of middle tiles. */
enum {
    TILE_FLIP_X = 0x4000,
    WINDOW_TILEMAP_ROW_BYTES = 0x40,
    DP_BORDER_MIDDLE_TILES = 0x63
};

/* $80:C52C: one row of the window border at tilemap offset X. A is the first
 * tile, offset by the window's tile base. The row is the left tile, then the
 * middle tiles alternating between a tile and its neighbour, then the left tile
 * again, mirrored. X advances to the next tilemap row. */
static void TextWindowBorderRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);

    PushIndex(memory, cpu);
    cpu->carry = 0;
    Add16Value(cpu, WramRead16(wram, TEXT_WINDOW_TILE_BASE));
    WramWrite16At(wram, TEXT_WINDOW_TILEMAP, cpu->x, cpu->accumulator);
    IncrementX16(cpu);
    IncrementX16(cpu);
    LoadY16(cpu, WramRead16(wram, DP_BORDER_MIDDLE_TILES));
    IncrementA16(cpu);
    IncrementA16(cpu);
    do {
        WramWrite16At(wram, TEXT_WINDOW_TILEMAP, cpu->x, cpu->accumulator);
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 1u));
        IncrementX16(cpu);
        IncrementX16(cpu);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    And16(cpu, 0xfffeu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    Or16(cpu, TILE_FLIP_X);
    WramWrite16At(wram, TEXT_WINDOW_TILEMAP, cpu->x, cpu->accumulator);
    PullAccumulator16(memory, cpu); /* PHX is deliberately consumed by PLA. */
    cpu->carry = 0;
    Add16Value(cpu, WINDOW_TILEMAP_ROW_BYTES);
    TransferAToX(cpu);
}

/* $80:C33F-C42F: actor-relative placement and speech-tail adjustment. */
static void TextWindowActorPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x09abu, 0);
    SimulateJslFrame(memory, cpu, 0x80u, 0xc345u);
    TextFindWindowActor(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x125cu, 0);
    AslA8(cpu); Adc8(cpu, 2);
    StoreADirect8(memory, cpu, 0x54u);
    LoadXDirect(memory, cpu, DP_SLOT_WORD_OFFSET);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x)));
    Subtract16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_Y, 0));
    And16(cpu, 0xfff0u);
    LsrA16(cpu); LsrA16(cpu); LsrA16(cpu);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x)));
    Subtract16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_X, 0));
    LsrA16(cpu); LsrA16(cpu); LsrA16(cpu);
    Add16Value(cpu, 0);
    StoreADirect16(memory, cpu, 0x5du);
    PullAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, 0x5eu);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->x);
    Compare8(cpu, A8(cpu), 4);
    {
        /* The speech tail goes below the window for facing 4, above it
         * otherwise; each side hands over to the other when it does not fit. */
        bool below = cpu->zero;

        for (;;) {
            if (below) {
                Write8(memory, DirectAddress(cpu, 0x55u), 0); /* C38F */
                LoadA8(cpu, DirectByte(memory, cpu, 0x5eu));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreADirect8(memory, cpu, 0x66u);
                cpu->carry = 0;
                Adc8(cpu, DirectByte(memory, cpu, 0x54u));
                Compare8(cpu, A8(cpu), 0x1cu);
                if (!cpu->carry)
                    break;
            }
            LoadA8(cpu, 0xffu);
            StoreADirect8(memory, cpu, 0x55u); /* C37F */
            LoadA8(cpu, DirectByte(memory, cpu, 0x5eu));
            cpu->carry = 1;
            Sbc8(cpu, DirectByte(memory, cpu, 0x54u));
            cpu->carry = 1;
            Sbc8(cpu, 2);
            StoreADirect8(memory, cpu, 0x66u);
            if (!cpu->negative)
                break;
            below = true;
        }
    }
    /* C39D */
    LoadA8(cpu, DirectByte(memory, cpu, 0x63u));
    LsrA8(cpu);
    cpu->carry = 1; Sbc8(cpu, DirectByte(memory, cpu, 0x5du));
    LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
    StoreADirect8(memory, cpu, 0x65u);
    LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
    if (cpu->negative) {
        LoadA8(cpu, 1); StoreADirect8(memory, cpu, 0x65u);
    } else {
        cpu->carry = 0; Adc8(cpu, DirectByte(memory, cpu, 0x63u));
        cpu->carry = 1; Sbc8(cpu, 0x1du);
        if (cpu->carry) {
            LoadA8(cpu, 0x1du);
            cpu->carry = 1; Sbc8(cpu, DirectByte(memory, cpu, 0x63u));
            StoreADirect8(memory, cpu, 0x65u);
        }
    }
    LoadA8(cpu, 0xe0u); StoreADirect8(memory, cpu, 0x5bu);
    DecrementDirect8(memory, cpu, 0x5eu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    if (!cpu->zero) {
        DecrementDirect8(memory, cpu, 0x5eu);
        DecrementDirect8(memory, cpu, 0x5eu);
    } else {
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x60u); StoreADirect8(memory, cpu, 0x5bu);
        ExchangeAccumulatorBytes(cpu);
    }
    IncrementDirect8(memory, cpu, 0x5du);
    LoadXDirect(memory, cpu, DP_ACTOR_SLOT);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
    BitImmediate8(cpu, 2);
    if (!cpu->zero) {
        IncrementDirect8(memory, cpu, 0x5du);
        IncrementDirect8(memory, cpu, 0x5du);
    }
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_FACING, cpu->x);
    Compare8(cpu, A8(cpu), 6);
    if (cpu->zero) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
        cpu->carry = 1; Sbc8(cpu, 4);
        StoreADirect8(memory, cpu, 0x5du);
        LoadA8(cpu, 0x40u); TestBitsDirect(memory, cpu, 0x5bu, 0);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_UNK_7FE216, cpu->x)));
        BitImmediate8(cpu, 2);
        if (!cpu->zero) {
            DecrementDirect8(memory, cpu, 0x5du);
            DecrementDirect8(memory, cpu, 0x5du);
        }
    }
    LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
    LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x65u));
    if (!cpu->carry) StoreADirect8(memory, cpu, 0x65u);
    else {
        LoadA8(cpu, DirectByte(memory, cpu, 0x65u));
        cpu->carry = 0; Adc8(cpu, DirectByte(memory, cpu, 0x63u));
        LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
        cpu->carry = 1; Sbc8(cpu, DirectByte(memory, cpu, 0x5du));
        if (!cpu->carry) {
            LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            cpu->carry = 0; Adc8(cpu, DirectByte(memory, cpu, 0x65u));
            StoreADirect8(memory, cpu, 0x65u);
        }
    }
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, 0x65u);
    SimulateJsrFrame(memory, cpu, 0xc427u);
    TextWindowTileOffset(memory, cpu); SimulateRtsFrame(memory, cpu);
    StoreADirect16(memory, cpu, 0x60u);
    LoadADirect16(memory, cpu, 0x5du);
    SimulateJsrFrame(memory, cpu, 0xc42eu);
    TextWindowTileOffset(memory, cpu); SimulateRtsFrame(memory, cpu);
    StoreADirect16(memory, cpu, 0x5du);
}

/* $80:C305: build the window frame; M1X0, DB = $7E. */
Lufia2ExecutionResult Lufia2TextBuildWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Write8(memory, DirectAddress(cpu, 0x5du), 0);
    Write8(memory, DirectAddress(cpu, 0x5eu), 0);
    LoadAAbsolute8(memory, cpu, 0x125bu, 0);
    Write8(memory, 0x7fd089u, A8(cpu));
    StoreADirect8(memory, cpu, 0x63u);
    Write8(memory, DirectAddress(cpu, 0x64u), 0);
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    StoreADirect8(memory, cpu, 0x56u);
    Write8(memory, DirectAddress(cpu, 0x57u), 0);
    TransferDirectToA(cpu); Write8(memory, 0x7fd08au, A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x099cu, 0); And8(cpu, 4);
    if (!cpu->zero) IncrementDirect8(memory, cpu, 0x63u);
    LoadAAbsolute8(memory, cpu, 0x1255u, 0);
    if (!cpu->zero) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x63u)); And8(cpu, 1);
        if (!cpu->zero) IncrementDirect8(memory, cpu, 0x63u);
    }
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x125du, 0));
    if (!cpu->zero) {
        Write16Direct(memory, cpu, 0x60u, cpu->y);
        LoadA8(cpu, 0xfcu);                                    /* C431 M1 */
        Sbc8(cpu, Read8(memory, LongIndexedAddress(0x059e8du, cpu->x)));
    } else {
        TextWindowActorPosition(memory, cpu);
        LoadA16(cpu, 0xfffcu);                                /* C431 M0 */
        StoreAAbsolute16(memory, cpu, 0x059eu, 0);
    }
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7eu); PushAccumulator8(memory, cpu); PullDataBank(memory, cpu);
    TransferDirectToA(cpu); LoadAAbsolute8(memory, cpu, 0x125cu, 0);
    SetAccumulatorWidth(cpu, 0); StoreADirect16(memory, cpu, 0x58u);
    LoadADirect16(memory, cpu, 0x60u);
    Write16Long(memory, 0x7fd085u, cpu->accumulator);
    TransferAToX(cpu);
    LoadA16(cpu, 0x20d6u);
    SimulateJsrFrame(memory, cpu, 0xc451u);
    TextWindowBorderRow(memory, cpu); SimulateRtsFrame(memory, cpu);
    LoadA16(cpu, 0x20dau); cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1255u, 0));
    StoreADirect16(memory, cpu, 0x65u);
    do {                                                     /* C45B */
        PushIndex(memory, cpu);
        LoadADirect16(memory, cpu, 0x65u);
        StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
        IncrementA16(cpu); StoreAAbsolute16(memory, cpu, 0x3040u, cpu->x);
        IncrementX16(cpu); IncrementX16(cpu);
        LoadY16(cpu, Read16Direct(memory, cpu, 0x63u));
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x099cu, 0));
        cpu->zero = (cpu->accumulator & 4u) == 0;
        if (!cpu->zero) {
            LoadA16(cpu, 0x20d7u);
            StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
            StoreAAbsolute16(memory, cpu, 0x3040u, cpu->x);
            IncrementX16(cpu); IncrementX16(cpu);
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        }
        LoadA16(cpu, 0x20d7u);
        do {
            StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
            StoreAAbsolute16(memory, cpu, 0x3040u, cpu->x);
            IncrementX16(cpu); IncrementX16(cpu);
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->zero);
        LoadADirect16(memory, cpu, 0x65u); Or16(cpu, 0x4000u);
        StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
        IncrementA16(cpu); StoreAAbsolute16(memory, cpu, 0x3040u, cpu->x);
        PullAccumulator16(memory, cpu);
        cpu->carry = 0; Add16Value(cpu, 0x80u); TransferAToX(cpu);
        Decrement16Direct(memory, cpu, 0x58u);
    } while (!cpu->zero);
    LoadA16(cpu, 0xa0d6u);
    SimulateJsrFrame(memory, cpu, 0xc4a6u);
    TextWindowBorderRow(memory, cpu); SimulateRtsFrame(memory, cpu);
    LoadY16(cpu, Read16Direct(memory, cpu, 0x5du));
    if (!cpu->zero) {
        LoadX16(cpu, 0);
        LoadADirect16(memory, cpu, 0x5au);
        if (cpu->negative) LoadX16(cpu, 0x0cu);
        AslA16(cpu);
        if (cpu->negative) {
            TransferXToA(cpu); cpu->carry = 0;
            Add16Value(cpu, 6); TransferAToX(cpu);
        }
        SetAccumulatorWidth(cpu, 1); cpu->carry = 0;
        /* Six consecutive ADCs deliberately propagate carry between tiles. */
        static const uint16_t destinations[6] = {0x3000,0x3002,0x3040,0x3042,0x3080,0x3082};
        for (unsigned i = 0; i < 6u; ++i) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80c514u + i, cpu->x)));
            Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x1255u, 0)));
            StoreAAbsolute8(memory, cpu, destinations[i], cpu->y);
        }
        LoadA8(cpu, DirectByte(memory, cpu, 0x5bu));
        for (unsigned i = 0; i < 6u; ++i)
            StoreAAbsolute8(memory, cpu, (uint16_t)(destinations[i] + 1u), cpu->y);
    }
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x80c513u);
}

/* Fill words for the glyph buffer, indexed by the glyph attribute. */
#define TEXT_GLYPH_FILL_TABLE 0x80c7beu

/* $80:C784: reset the glyph state and fill the whole glyph buffer with the
 * attribute's background word. */
Lufia2ExecutionResult Lufia2TextClearGlyphBuffer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram caller = WramViewOfCaller(memory, cpu);
    Lufia2Wram buffer;
    uint16_t fill;

    Push8(memory, cpu, PackStatus(cpu));                       /* C784 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    WramWrite16(caller, TEXT_GLYPH, 0);
    WramWrite16(caller, TEXT_GLYPH_STATE, 0);
    LoadA16(cpu, TEXT_GLYPH_BUFFER);
    WramWrite16(caller, TEXT_GLYPH_DESTINATION, cpu->accumulator);
    WramWrite16(caller, TEXT_LINE_START, cpu->accumulator);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    buffer = WramViewInBank(memory, cpu, cpu->data_bank);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, TEXT_GLYPH_ATTRIBUTE));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(TEXT_GLYPH_FILL_TABLE, cpu->x)));
    fill = cpu->accumulator;
    LoadX16(cpu, TEXT_GLYPH_BUFFER_BYTES - 4u);
    do {                                                     /* C7AF */
        WramWrite16At(buffer, TEXT_GLYPH_BUFFER, cpu->x, fill);
        WramWrite16At(buffer, TEXT_GLYPH_BUFFER + 2u, cpu->x, fill);
        LoadX16(cpu, (uint16_t)(cpu->x - 4u));
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80c7bdu);
}

/* Window row drawing: scratch words, the attribute bits of the window tiles
 * and the tilemap offsets of the two rows written. */
enum {
    DP_WINDOW_TILE = 0x54,
    DP_WINDOW_TILEMAP_OFFSET = 0x56,
    WINDOW_TILE_ATTRIBUTES = 0x2100,
    WINDOW_ROW_ONE = TEXT_WINDOW_TILEMAP + 0x42,
    WINDOW_ROW_TWO = TEXT_WINDOW_TILEMAP + 0x82,
    WINDOW_UPLOAD_ROWS = TEXT_WINDOW_TILEMAP + 0x40,
    WINDOW_CENTERED_FLAG = 0x0004
};

/* $80:C5DD: write the next two rows of window tiles. The first tile number comes
 * from the row counter and the tile base, the tilemap position from the window
 * origin (shifted by two when bit 2 of the window state is set), and each pass
 * writes one tile in each of two consecutive tilemap rows for as many columns
 * as the window is wide. The LSR carry is kept for the ADC that follows. */
static void TextWriteWindowRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram work = WramViewLong(memory);

    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, WramRead(work, TEXT_WINDOW_ROW_COUNT));
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    LsrA16(cpu);
    WramWrite16(wram, DP_WINDOW_TILE, cpu->accumulator);
    Add16Value(cpu, WramRead16(work, TEXT_WINDOW_ROW_ORIGIN));
    TransferAToX(cpu);
    WramWrite16(wram, DP_WINDOW_TILEMAP_OFFSET, cpu->accumulator);
    LoadA16(cpu, WramRead16(wram, TEXT_WINDOW_STATE));
    cpu->zero = (cpu->accumulator & WINDOW_CENTERED_FLAG) == 0;
    if (!cpu->zero) {
        IncrementX16(cpu);
        IncrementX16(cpu);
    }
    LoadA16(cpu, WramRead16(work, TEXT_WINDOW_ROW_WIDTH));
    And16(cpu, 0x00ffu);
    TransferAToY(cpu);
    LoadADirect16(memory, cpu, DP_WINDOW_TILE);
    LsrA16(cpu);
    Or16(cpu, WINDOW_TILE_ATTRIBUTES);
    WramWrite16(wram, DP_WINDOW_TILE, cpu->accumulator);
    do {                                                     /* C60B */
        WramWrite16At(work, 0x7e0000u | WINDOW_ROW_ONE, cpu->x, cpu->accumulator);
        IncrementA16(cpu);
        WramWrite16At(work, 0x7e0000u | WINDOW_ROW_TWO, cpu->x, cpu->accumulator);
        IncrementA16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
}

/* Upload channels for the window: glyph tiles on channel 2, the tilemap rows on
 * channel 1. Each channel has a VRAM destination word at $0079 + 2 * channel
 * and a request byte at $75 + channel, which holds the channel's bit and bit 6
 * for the NMI to act on. */
enum {
    WINDOW_TILE_CHANNEL = 2,
    WINDOW_MAP_CHANNEL = 1,
    DMA_MODE_WORD = 1,
    DMA_VRAM_DATA = 0x18,
    GLYPH_UPLOAD_BYTES = 0x0400,
    GLYPH_VRAM_BASE = 0x1800,
    MAP_UPLOAD_BYTES = 0x0080,
    MAP_VRAM_BASE = 0x0820,
    DMA_REQUEST = 0x40,
    WORK_RAM_BANK = 0x7e
};

/* Window preparation: the first upload covers the whole glyph buffer's first
 * rows, and the BG3 scroll is parked at -4. */
enum {
    WINDOW_FIRST_UPLOAD_BYTES = 0x0c00,
    WINDOW_SCROLL_RESET = 0xfffc,
    WINDOW_UPLOAD_FLAG = 0x08,
    WINDOW_STATE_MASK = 0x10,
    DP_WINDOW_UPLOAD_FLAGS = 0x74
};

#define WINDOW_VRAM_DESTINATION(channel) (0x0079u + 2u * (unsigned)(channel))
#define WINDOW_UPLOAD_REQUEST(channel) (0x75u + (unsigned)(channel))

/* $80:C56E: write a pair of window rows, then queue their uploads: the glyph
 * tiles of the row's first tile and the two tilemap rows. Y is kept. */
Lufia2ExecutionResult Lufia2TextQueueWindowRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram work = WramViewLong(memory);

    Push8(memory, cpu, PackStatus(cpu));                       /* C56E */
    PushY(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xc572u);
    TextWriteWindowRow(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, DP_WINDOW_TILE));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    WramWrite16(wram, DP_WINDOW_TILE, cpu->accumulator);
    AslA16(cpu);
    Add16Value(cpu, TEXT_GLYPH_BUFFER);
    StoreAAbsolute16(memory, cpu, SNES_A1TL(WINDOW_TILE_CHANNEL), 0);
    LoadA16(cpu, WramRead16(wram, DP_WINDOW_TILE));
    Add16Value(cpu, GLYPH_VRAM_BASE);
    StoreAAbsolute16(memory, cpu, WINDOW_VRAM_DESTINATION(WINDOW_TILE_CHANNEL), 0);
    LoadA16(cpu, WramRead16(wram, DP_WINDOW_TILEMAP_OFFSET));
    cpu->carry = 0;
    Add16Value(cpu, WINDOW_UPLOAD_ROWS);
    StoreAAbsolute16(memory, cpu, SNES_A1TL(WINDOW_MAP_CHANNEL), 0);
    LoadA16(cpu, WramRead16(wram, DP_WINDOW_TILEMAP_OFFSET));
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, MAP_VRAM_BASE);
    StoreAAbsolute16(memory, cpu, WINDOW_VRAM_DESTINATION(WINDOW_MAP_CHANNEL), 0);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DMA_MODE_WORD);
    StoreAAbsolute8(memory, cpu, SNES_DMAP(WINDOW_TILE_CHANNEL), 0);
    StoreAAbsolute8(memory, cpu, SNES_DMAP(WINDOW_MAP_CHANNEL), 0);
    LoadA8(cpu, WORK_RAM_BANK);
    StoreAAbsolute8(memory, cpu, SNES_A1B(WINDOW_TILE_CHANNEL), 0);
    LoadA8(cpu, WORK_RAM_BANK);
    StoreAAbsolute8(memory, cpu, SNES_A1B(WINDOW_MAP_CHANNEL), 0);
    LoadA8(cpu, DMA_VRAM_DATA);
    StoreAAbsolute8(memory, cpu, SNES_BBAD(WINDOW_TILE_CHANNEL), 0);
    StoreAAbsolute8(memory, cpu, SNES_BBAD(WINDOW_MAP_CHANNEL), 0);
    LoadX16(cpu, GLYPH_UPLOAD_BYTES);
    Write16Absolute(memory, cpu, SNES_DASL(WINDOW_TILE_CHANNEL), cpu->x);
    LoadX16(cpu, MAP_UPLOAD_BYTES);
    Write16Absolute(memory, cpu, SNES_DASL(WINDOW_MAP_CHANNEL), cpu->x);
    LoadA8(cpu, DMA_REQUEST | (1u << WINDOW_MAP_CHANNEL));
    WramWrite(wram, WINDOW_UPLOAD_REQUEST(WINDOW_MAP_CHANNEL), A8(cpu));
    LoadA8(cpu, DMA_REQUEST | (1u << WINDOW_TILE_CHANNEL));
    WramWrite(wram, WINDOW_UPLOAD_REQUEST(WINDOW_TILE_CHANNEL), A8(cpu));
    LoadA8(cpu, WramRead(work, TEXT_WINDOW_ROW_COUNT));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    WramWrite(work, TEXT_WINDOW_ROW_COUNT, A8(cpu));
    cpu->y = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80c5dcu);
}

/* $80:C23D: place the window or stop at C2A1. */
Lufia2ExecutionResult Lufia2TextPrepareWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 1);
    TestBitsAbsolute8(memory, cpu, WRAM_UNK_7E09A9, 0);
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xc246u);
    Lufia2TextClearGlyphBuffer(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    PushDataBank(memory, cpu);
    LoadAAbsolute8(memory, cpu, WRAM_WINDOW_MODE, 0);
    BitImmediate8(cpu, 2);
    if (cpu->zero) {
        SimulateJsrFrame(memory, cpu, 0xc27bu);
        Lufia2TextBuildWindow(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadA8(cpu, WINDOW_STATE_MASK);
        TestBitsAbsolute8(memory, cpu, TEXT_WINDOW_STATE, 1);
        TransferDirectToA(cpu);
        Write8(memory, TEXT_WINDOW_ROW_COUNT, A8(cpu));
        Write8(memory, TEXT_WINDOW_ROW_MARK, A8(cpu));
        LoadX16(cpu, WINDOW_SCROLL_RESET);
        Write16Absolute(memory, cpu, TEXT_BG3_VOFS_SHADOW, cpu->x);
        PullDataBank(memory, cpu);
        cpu->y = PullIndexValue(memory, cpu);
        LoadA8(cpu, 1);
        TestBitsAbsolute8(memory, cpu, WRAM_UNK_7E09A9, 1);
        LoadA8(cpu, WINDOW_UPLOAD_FLAG);
        StoreADirect8(memory, cpu, DP_WINDOW_UPLOAD_FLAGS);
        LoadA8(cpu, 1);
        TestBitsAbsolute8(memory, cpu, TEXT_WINDOW_STATE, 1);
        return ExecutionReturned(0x80c2a0u);
    }
    LoadA8(cpu, DMA_MODE_WORD);
    StoreAAbsolute8(memory, cpu, SNES_DMAP(0), 0);
    LoadX16(cpu, TEXT_GLYPH_BUFFER);
    Write16Absolute(memory, cpu, SNES_A1TL(0), cpu->x);
    LoadX16(cpu, GLYPH_VRAM_BASE);
    Write16Absolute(memory, cpu, WINDOW_VRAM_DESTINATION(0), cpu->x);
    LoadA8(cpu, WORK_RAM_BANK);
    StoreAAbsolute8(memory, cpu, SNES_A1B(0), 0);
    LoadA8(cpu, DMA_VRAM_DATA);
    StoreAAbsolute8(memory, cpu, SNES_BBAD(0), 0);
    LoadX16(cpu, WINDOW_FIRST_UPLOAD_BYTES);
    Write16Absolute(memory, cpu, SNES_DASL(0), cpu->x);
    LoadA8(cpu, DMA_REQUEST | 1u);
    StoreADirect8(memory, cpu, WINDOW_UPLOAD_REQUEST(0));
    SimulateJsrFrame(memory, cpu, 0xc276u);
    return ExecutionHandoff(cpu, 0x80c2a1u);
}
