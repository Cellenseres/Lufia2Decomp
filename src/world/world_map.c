/* World map NMI, streaming and regions. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "lufia2/system.h"
#include "lufia2/world_map.h"
#include "system/wram.h"

/* Direct-page scratch of the tile upload routines. */
enum {
    UPLOAD_DP_CHANNELS = 0x05u,
    UPLOAD_DP_FIRST_SIZE = 0x33u,
    UPLOAD_DP_SECOND_SIZE = 0x35u,
    UPLOAD_DP_ROWS_LEFT = 0x37u,
    UPLOAD_DP_BLOCK_PASS2_LEFT = 0x38u,
    UPLOAD_DP_VRAM_ADDRESS = 0x39u,
    UPLOAD_DP_VRAM_ADDRESS_HIGH = 0x3au,
    UPLOAD_ROW_STRIDE = 0x0100u,
    UPLOAD_BLOCK_STRIDE = 0x0200u,
};

/* World map streaming state and register shadows. */
enum {
    WORLD_MAP_HDMA_ENABLE = 0x11d8u,    /* non-zero: DP $33 enables the HDMA channels */
    WORLD_MAP_UPLOAD_PENDING = 0x11d9u, /* non-zero: the NMI uploads tiles */
    WORLD_MAP_TILE_UPLOAD_READY = 0x1365u,
    WORLD_MAP_MOVE_COUNT = 0x11e3u, /* camera cell changes, saturating at $FF */
    WORLD_MAP_CAMERA_CELL_X = 0x11f2u,
    WORLD_MAP_CAMERA_CELL_Y = 0x11f4u,
    WORLD_MAP_STREAMED_CELL_X = 0x11f6u, /* the cell last streamed */
    WORLD_MAP_STREAMED_CELL_Y = 0x11f7u,
    WORLD_MAP_MODE7_CENTRE = 0x11f8u,   /* four bytes for M7X and M7Y */
    WORLD_MAP_SCROLL_SHADOW = 0x0594u,  /* twelve bytes for BG1-3 scroll */
    WORLD_MAP_PALETTE_INDEX = 0x1701u,  /* pending CGRAM block */
    WORLD_MAP_PALETTE_LENGTH = 0x1702u, /* bytes, 0 when nothing is pending */
    WORLD_MAP_PALETTE_SOURCE = 0x1704u,
    WORLD_MAP_PALETTE_BANK = 0x1706u,
    WORLD_MAP_MODE7_MATRIX = 0x1707u, /* eight bytes for M7A-M7D */
    WORLD_MAP_COLOUR_MATH = 0x170fu,  /* CGADSUB value */
    WORLD_MAP_ROW_PENDING = 0x1710u,  /* set to $FF by the streamer */
    WORLD_MAP_COLUMN_PENDING = 0x1711u,
    WORLD_MAP_ROW_STAGE = WRAM_WORLD_MAP_ROW_STAGE & 0xffffu,    /* source address; low 14 bits are VRAM */
    WORLD_MAP_COLUMN_STAGE = WRAM_WORLD_MAP_COLUMN_STAGE & 0xffffu, /* VRAM address of the column */
    WORLD_MAP_COLOUR_TABLE = WRAM_WORLD_MAP_COLOR_HDMA_TABLE & 0xffffu, /* HDMA source for channel 4 */
    WORLD_MAP_MATRIX_TABLE = 0x1718u, /* HDMA source for channel 0 */
    WORLD_MAP_SHAKE_X = 0x1e50u,      /* sprite chain shake, x then y words */
    WORLD_MAP_SHAKE_Y = 0x1e52u,
    WORLD_MAP_SHAKE_DECAY_X = 0x1e54u, /* shake decay words, 4 and 1 per frame */
    WORLD_MAP_SHAKE_DECAY_Y = 0x1e56u,
    WORLD_MAP_STAGE_BANK = 0x7fu, /* bank of the staged tilemap */
    WORLD_MAP_VRAM_ADDRESS_MASK = 0x3fffu,
    WORLD_MAP_ROW_BYTES = 0x0100u,
    WORLD_MAP_COLUMN_BUFFER = 0xdf00u, /* $7F:DF00, two halves */
    WORLD_MAP_COLUMN_HALF = 0x80u,
};

/* DMA DP $37 rows to VRAM, one per row. */
static void WorldMapUploadRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               bool first_pass) {
    cpu->carry = 0;
    do {
        SetAccumulatorWidth(cpu, 0);
        if (first_pass) {
            LoadADirect16(memory, cpu, UPLOAD_DP_FIRST_SIZE);
            Write16Absolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
        }
        LoadADirect16(memory, cpu, UPLOAD_DP_SECOND_SIZE);
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->accumulator);
        LoadADirect16(memory, cpu, UPLOAD_DP_VRAM_ADDRESS);
        Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
        cpu->carry = 0;
        Add16Value(cpu, UPLOAD_ROW_STRIDE);
        StoreADirect16(memory, cpu, UPLOAD_DP_VRAM_ADDRESS);
        SetAccumulatorWidth(cpu, 1);
        if (first_pass) {
            LoadA8(cpu, DirectByte(memory, cpu, UPLOAD_DP_CHANNELS));
            StoreAAbsolute8(memory, cpu, SNES_MDMAEN, 0);
        } else {
            StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
        }
        DecrementDirect8(memory, cpu, UPLOAD_DP_ROWS_LEFT);
    } while (!cpu->zero);
}

/* $86:D1E8: tile column upload, rows of $0100 words. */
static void WorldMapColumnUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xd1d8u);
    SetAccumulatorWidth(cpu, 0);                               /* D1E8 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->y));
    Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    StoreAAbsolute8(memory, cpu, SNES_A1B(6), 0);
    LoadA8(cpu, 0x40u);
    StoreADirect8(memory, cpu, UPLOAD_DP_CHANNELS);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    StoreADirect8(memory, cpu, UPLOAD_DP_FIRST_SIZE);
    LoadAAbsolute8(memory, cpu, 0xd26bu, cpu->x);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, UPLOAD_DP_FIRST_SIZE));
    StoreADirect8(memory, cpu, UPLOAD_DP_SECOND_SIZE);
    if (!cpu->zero) {
        LoadA8(cpu, 0xc0u);
        StoreADirect8(memory, cpu, UPLOAD_DP_CHANNELS);
    }
    LoadAAbsolute8(memory, cpu, 0x0002u, cpu->y);              /* D214 */
    StoreADirect8(memory, cpu, UPLOAD_DP_ROWS_LEFT);
    WorldMapUploadRows(memory, cpu, true);
    LoadAAbsolute8(memory, cpu, 0xd26cu, cpu->x);              /* D23C */
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 0x0002u, cpu->y));
    if (!cpu->zero) {
        StoreADirect8(memory, cpu, UPLOAD_DP_ROWS_LEFT);
        LoadAAbsolute8(memory, cpu, 0xd26bu, cpu->x);
        StoreADirect8(memory, cpu, UPLOAD_DP_SECOND_SIZE);
        WorldMapUploadRows(memory, cpu, false);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $86:D271: block upload, two passes of rows $0200 apart. */
static void WorldMapBlockUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned pass;

    SimulateJsrFrame(memory, cpu, 0xd1ddu);
    SetAccumulatorWidth(cpu, 0);                               /* D271 */
    And16(cpu, 0x007fu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->y));
    Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    StoreAAbsolute8(memory, cpu, SNES_A1B(6), 0);
    LoadAAbsolute8(memory, cpu, 0xd2d0u, cpu->x);
    StoreADirect8(memory, cpu, UPLOAD_DP_FIRST_SIZE);
    LoadAAbsolute8(memory, cpu, 0xd2d1u, cpu->x);
    StoreADirect8(memory, cpu, UPLOAD_DP_ROWS_LEFT);
    StoreADirect8(memory, cpu, UPLOAD_DP_BLOCK_PASS2_LEFT);
    for (pass = 0; pass < 2u; ++pass) {
        if (pass)
            IncrementDirect8(memory, cpu, UPLOAD_DP_VRAM_ADDRESS_HIGH); /* D2B0 */
        LoadXDirect16(memory, cpu, UPLOAD_DP_VRAM_ADDRESS);
        cpu->carry = 0;
        do {
            SetAccumulatorWidth(cpu, 0);                       /* D295 */
            LoadADirect16(memory, cpu, UPLOAD_DP_FIRST_SIZE);
            Write16Absolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
            TransferXToA(cpu);
            Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, UPLOAD_BLOCK_STRIDE);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0x40u, SNES_MDMAEN);
            DecrementDirect8(memory, cpu,
                             pass ? UPLOAD_DP_BLOCK_PASS2_LEFT : UPLOAD_DP_ROWS_LEFT);
        } while (!cpu->zero);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $86:D1A1: pending map tile uploads, $1365 entries. */
static void WorldMapTileUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xcf14u);
    LoadA8(cpu, 0x18u);                                        /* D1A1 */
    StoreAAbsolute8(memory, cpu, SNES_BBAD(6), 0);
    StoreAAbsolute8(memory, cpu, SNES_BBAD(7), 0);
    StoreAImmediate8(memory, cpu, 0x01u, SNES_DMAP(6));
    StoreAImmediate8(memory, cpu, 0x09u, SNES_DMAP(7));
    LoadX16(cpu, 0xd1e7u);
    Write16Absolute(memory, cpu, SNES_A1TL(7), cpu->x);
    StoreAImmediate8(memory, cpu, 0x86u, SNES_A1B(7));
    Write8(memory, DirectAddress(cpu, 0x34u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x36u), 0x00u);
    LoadX16(cpu, 0x1367u);
    do {
        PushIndex(memory, cpu);                                /* D1C5 */
        LoadY16(cpu, Read16DirectIndexed(memory, cpu, 0x40u, cpu->x));
        if (!cpu->zero) {
            StoreYDirect16(memory, cpu, 0x39u);
            LoadY16(cpu, Read16DirectIndexed(memory, cpu, 0x00u, cpu->x));
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            if (!cpu->negative)
                WorldMapColumnUpload(memory, cpu);
            else
                WorldMapBlockUpload(memory, cpu);
        }
        cpu->x = PullIndexValue(memory, cpu);                  /* D1DE */
        IncrementX16(cpu);
        IncrementX16(cpu);
        {
            const uint32_t count = AbsoluteIndexedAddress(cpu, 0x1365u, 0);
            const uint8_t left = (uint8_t)(Read8(memory, count) - 1u);

            Write8(memory, count, left);
            SetNz8(cpu, left);
        }
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* Write count colours from the CGRAM buffer at Y. */
static void WorldMapCopyColors(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                               uint8_t count_dp) {
    do {
        LoadAAbsolute8(memory, cpu, WRAM_CGRAM_BUFFER, cpu->y);
        StoreAAbsolute8(memory, cpu, SNES_CGDATA, 0);
        IncrementY16(cpu);
        LoadAAbsolute8(memory, cpu, WRAM_CGRAM_BUFFER, cpu->y);
        StoreAAbsolute8(memory, cpu, SNES_CGDATA, 0);
        IncrementY16(cpu);
        DecrementDirect8(memory, cpu, count_dp);
    } while (!cpu->zero);
}

/* $86:CFC0: $16E7 palette cycles, five bytes each at $16E8. */
static void WorldMapPaletteCycles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, 0x16e8u);
    do {
        PushAccumulator8(memory, cpu);                         /* CFC8 */
        {
            const uint32_t tick = DirectIndexedAddress(cpu, 0x04u, cpu->x);
            const uint8_t value = (uint8_t)(Read8(memory, tick) + 1u);

            Write8(memory, tick, value);
            SetNz8(cpu, value);
            LoadA8(cpu, value);
        }
        Compare8(cpu, A8(cpu),
            Read8(memory, DirectIndexedAddress(cpu, 0x02u, cpu->x)));
        if (cpu->carry) {
            uint32_t cycle;

            Write8(memory, DirectIndexedAddress(cpu, 0x04u, cpu->x), 0x00u);
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x00u, cpu->x)));
            StoreAAbsolute8(memory, cpu, SNES_CGADD, 0);
            cycle = DirectIndexedAddress(cpu, 0x03u, cpu->x);
            LoadA8(cpu, (uint8_t)(Read8(memory, cycle) + 1u));
            Compare8(cpu, A8(cpu),
                Read8(memory, DirectIndexedAddress(cpu, 0x01u, cpu->x)));
            if (cpu->carry)
                LoadA8(cpu, 0x00u);
            Write8(memory, cycle, A8(cpu));                    /* CFE1 */
            StoreADirect8(memory, cpu, 0x38u);
            AslA8(cpu);
            StoreADirect8(memory, cpu, 0x33u);
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x01u, cpu->x)));
            cpu->carry = 1;
            Sbc8(cpu, Read8(memory, cycle));
            StoreADirect8(memory, cpu, 0x37u);
            LoadA8(cpu, 0x00u);
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x00u, cpu->x)));
            AslA8(cpu);
            Adc8(cpu, DirectByte(memory, cpu, 0x33u));
            TransferAToY(cpu);
            WorldMapCopyColors(memory, cpu, 0x37u); /* CFF8 */
            LoadA8(cpu, DirectByte(memory, cpu, 0x38u));
            if (!cpu->zero) {
                LoadA8(cpu, Read8(memory,
                    DirectIndexedAddress(cpu, 0x00u, cpu->x)));
                AslA8(cpu);
                TransferAToY(cpu);
                WorldMapCopyColors(memory, cpu, 0x38u); /* D012 */
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* D024 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0005u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
    } while (!cpu->zero);
}

/* DMA the staged row strip. */
static void WorldMapUploadStreamedRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, WORLD_MAP_ROW_PENDING, 0);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, SNES_VMAIN, 0);
        StoreAImmediate8(memory, cpu, WORLD_MAP_STAGE_BANK, SNES_A1B(7));
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WORLD_MAP_ROW_STAGE, 0));
        Write16Absolute(memory, cpu, SNES_A1TL(7), cpu->accumulator);
        And16(cpu, WORLD_MAP_VRAM_ADDRESS_MASK);
        Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, WORLD_MAP_ROW_BYTES);
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
        StoreZeroAbsolute8(memory, cpu, WORLD_MAP_ROW_PENDING, 0);
    }
}

/* $86:CF48: DMA the staged column strip in two halves. */
static void WorldMapUploadStreamedColumn(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, WORLD_MAP_COLUMN_PENDING, 0); /* CF48 */
    if (!cpu->zero) {
        StoreAImmediate8(memory, cpu, 0x03u, SNES_VMAIN);
        StoreAImmediate8(memory, cpu, WORLD_MAP_STAGE_BANK, SNES_A1B(7));
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WORLD_MAP_COLUMN_STAGE, 0));
        And16(cpu, WORLD_MAP_VRAM_ADDRESS_MASK);
        Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
        IncrementA16(cpu);
        PushAccumulator16(memory, cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, WORLD_MAP_COLUMN_BUFFER);
        Write16Absolute(memory, cpu, SNES_A1TL(7), cpu->x);
        LoadX16(cpu, 0x0080u);
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
        cpu->y = PullIndexValue(memory, cpu);
        Write16Absolute(memory, cpu, SNES_VMADDL, cpu->y);
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
        StoreZeroAbsolute8(memory, cpu, WORLD_MAP_COLUMN_PENDING, 0);
    }
}

/* $86:CF86: restore VRAM increment, DMA the pending palette. */
static void WorldMapUploadPalette(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreAImmediate8(memory, cpu, 0x80u, SNES_VMAIN);          /* CF86 */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WORLD_MAP_PALETTE_LENGTH, 0));
    if (!cpu->zero) {
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->x);
        CopyAbsolute8(memory, cpu, WORLD_MAP_PALETTE_INDEX, SNES_CGADD);
        SetAccumulatorWidth(cpu, 0);
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        Add16Value(cpu,
                   Read16AbsoluteIndexed(memory, cpu, WORLD_MAP_PALETTE_SOURCE, 0));
        Write16Absolute(memory, cpu, SNES_A1TL(7), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        CopyAbsolute8(memory, cpu, WORLD_MAP_PALETTE_BANK, SNES_A1B(7));
        StoreZeroAbsolute8(memory, cpu, SNES_DMAP(7), 0);
        StoreAImmediate8(memory, cpu, 0x22u, SNES_BBAD(7));
        StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
        LoadX16(cpu, 0x0000u);
        Write16Absolute(memory, cpu, WORLD_MAP_PALETTE_LENGTH, cpu->x);
    }
}

/* $86:D032: mode 7 registers or HDMA channels by scene flags. */
static void WorldMapSetupHdma(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t mode7_regs[8] = {SNES_M7A, SNES_M7A, SNES_M7B, SNES_M7B,
                                           SNES_M7C, SNES_M7C, SNES_M7D, SNES_M7D};
    unsigned i;

    Write8(memory, DirectAddress(cpu, 0x33u), 0x00u);          /* D032 */
    LoadAAbsolute8(memory, cpu, 0x11deu, 0);
    if (cpu->zero) {
        for (i = 0; i < 8u; ++i)
            CopyAbsolute8(memory, cpu, (uint16_t)(WORLD_MAP_MODE7_MATRIX + i),
                          mode7_regs[i]);
    } else {
        LoadAAbsolute8(memory, cpu, 0x11dau, 0);               /* D06B */
        if (!cpu->negative) {
            LoadA8(cpu, 0x03u);
            StoreAAbsolute8(memory, cpu, SNES_DMAP(0), 0);
            StoreAAbsolute8(memory, cpu, SNES_DMAP(1), 0);
            StoreAImmediate8(memory, cpu, 0x1bu, SNES_BBAD(0));
            StoreAImmediate8(memory, cpu, 0x1du, SNES_BBAD(1));
            StoreAImmediate8(memory, cpu, 0x00u, SNES_A1B(0));
            StoreAImmediate8(memory, cpu, 0x00u, SNES_A1B(1));
            LoadX16(cpu, WORLD_MAP_MATRIX_TABLE);
            Write16Absolute(memory, cpu, SNES_A1TL(0), cpu->x);
            LoadX16(cpu, 0x1a9bu);
            Write16Absolute(memory, cpu, SNES_A1TL(1), cpu->x);
            LoadA8(cpu, 0x03u);
            TestBitsDirect(memory, cpu, 0x33u, 1);
        }
        LoadAAbsolute8(memory, cpu, 0x11ddu, 0);               /* D09C */
        if (cpu->zero) {
            StoreZeroAbsolute8(memory, cpu, SNES_CGWSEL, 0);
            CopyAbsolute8(memory, cpu, WORLD_MAP_COLOUR_MATH, SNES_CGADSUB);
            StoreZeroAbsolute8(memory, cpu, SNES_DMAP(4), 0);
            StoreAImmediate8(memory, cpu, 0x32u, SNES_BBAD(4));
            StoreZeroAbsolute8(memory, cpu, SNES_A1B(4), 0);
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WORLD_MAP_COLOUR_TABLE, 0));
            Write16Absolute(memory, cpu, SNES_A1TL(4), cpu->x);
            LoadA8(cpu, 0x10u);
            TestBitsDirect(memory, cpu, 0x33u, 1);
        }
    }
    LoadAAbsolute8(memory, cpu, 0x11dfu, 0);                   /* D0BF */
    if (!cpu->zero) {
        LoadA8(cpu, 0x43u);
        StoreAAbsolute8(memory, cpu, SNES_DMAP(4), 0);
        StoreAAbsolute8(memory, cpu, SNES_DMAP(5), 0);
        LoadX16(cpu, 0xde00u);
        Write16Absolute(memory, cpu, SNES_A1TL(4), cpu->x);
        LoadX16(cpu, 0xe000u);
        Write16Absolute(memory, cpu, SNES_A1TL(5), cpu->x);
        LoadA8(cpu, 0x30u);
        TestBitsDirect(memory, cpu, 0x33u, 1);
    }
    LoadAAbsolute8(memory, cpu, WRAM_BATTLE_BACKGROUND_ID, 0); /* D0DC */
    if (!cpu->zero) {
        LoadA8(cpu, 0x41u);
        StoreAAbsolute8(memory, cpu, SNES_DMAP(2), 0);
        StoreAAbsolute8(memory, cpu, SNES_DMAP(3), 0);
        StoreAImmediate8(memory, cpu, 0x26u, SNES_BBAD(2));
        StoreAImmediate8(memory, cpu, 0x28u, SNES_BBAD(3));
        LoadA8(cpu, 0x7fu);
        StoreAAbsolute8(memory, cpu, SNES_A1B(2), 0);
        StoreAAbsolute8(memory, cpu, SNES_DASB(2), 0);
        StoreAAbsolute8(memory, cpu, SNES_A1B(3), 0);
        StoreAAbsolute8(memory, cpu, SNES_DASB(3), 0);
        LoadY16(cpu, 0xd400u);
        LoadAAbsolute8(memory, cpu, 0x11dbu, 0);
        LsrA8(cpu);
        if (cpu->carry)
            LoadY16(cpu, 0xd600u);
        Write16Absolute(memory, cpu, SNES_A1TL(2), cpu->y);
        LoadY16(cpu, 0xd800u);
        LoadAAbsolute8(memory, cpu, 0x11dcu, 0);
        LsrA8(cpu);
        if (cpu->carry)
            LoadY16(cpu, 0xda00u);
        Write16Absolute(memory, cpu, SNES_A1TL(3), cpu->y);
        LoadA8(cpu, 0x0cu);
        TestBitsDirect(memory, cpu, 0x33u, 1);
        LoadY16(cpu, 0x00ffu);
        Write16Absolute(memory, cpu, SNES_WH0, cpu->y);
        Write16Absolute(memory, cpu, SNES_WH2, cpu->y);
    }
}

/* $86:D12C: enable HDMA, copy mode 7 centre and scroll. */
static void WorldMapFinishRegisters(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t scroll_regs[6] = {SNES_BG1HOFS, SNES_BG1VOFS, SNES_BG2HOFS,
                                            SNES_BG2VOFS, SNES_BG3HOFS, SNES_BG3VOFS};
    unsigned i;

    LoadAAbsolute8(memory, cpu, WORLD_MAP_HDMA_ENABLE, 0); /* D12C */
    if (!cpu->zero) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x33u));
        StoreAAbsolute8(memory, cpu, SNES_HDMAEN, 0);
    }
    for (i = 0; i < 4u; ++i)                                   /* D136 */
        CopyAbsolute8(memory, cpu, (uint16_t)(WORLD_MAP_MODE7_CENTRE + i),
                      (uint16_t)(SNES_M7X + (i >> 1)));
    LoadAAbsolute8(memory, cpu, WORLD_MAP_UPLOAD_PENDING, 0);
    if (cpu->zero) {
        for (i = 0; i < 12u; ++i)
            CopyAbsolute8(memory, cpu, (uint16_t)(WORLD_MAP_SCROLL_SHADOW + i),
                          scroll_regs[i >> 1]);
    }
}

/* $86:CEF6: world map NMI via the $00:0067 vector. */
Lufia2ExecutionResult Lufia2WorldMapNmiUploads(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x86d1a0u;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu)); /* CEF6 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, 0x86u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StoreAImmediate8(memory, cpu, 0x8fu, SNES_INIDISP);
    StoreZeroAbsolute8(memory, cpu, SNES_HDMAEN, 0);
    LoadAAbsolute8(memory, cpu, WORLD_MAP_UPLOAD_PENDING, 0);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, WORLD_MAP_TILE_UPLOAD_READY, 0);
        if (!cpu->zero)
            WorldMapTileUploads(memory, cpu);
    }
    StoreZeroAbsolute8(memory, cpu, SNES_DMAP(7), 0); /* CF15 */
    StoreAImmediate8(memory, cpu, 0x18u, SNES_BBAD(7));
    WorldMapUploadStreamedRow(memory, cpu);
    WorldMapUploadStreamedColumn(memory, cpu);
    WorldMapUploadPalette(memory, cpu);
    LoadAAbsolute8(memory, cpu, WRAM_WORLD_MAP_SKYLINE_BAND_COUNT & 0xffffu, 0); /* CFC0 */
    if (!cpu->zero)
        WorldMapPaletteCycles(memory, cpu);
    WorldMapSetupHdma(memory, cpu);
    WorldMapFinishRegisters(memory, cpu);
    StoreZeroAbsolute8(memory, cpu, WORLD_MAP_UPLOAD_PENDING, 0); /* D19B */
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return result;
}

/* DP fields of the map streaming routines. */
enum {
    STREAM_DP_BLOCK_POINTER = 0x08u,
    STREAM_DP_CELL_OFFSET = 0x0bu,
    STREAM_DP_SUBCELL = 0x13u,
    STREAM_DP_COUNT_LEFT = 0x26u,
    STREAM_DP_CELL_X = 0x58u,
    STREAM_DP_CELL_Y = 0x5au,
    STREAM_ROW_VRAM = 0x001712u,
    STREAM_COLUMN_VRAM = 0x001714u,
};

/* $86:ADEE: $0B = map cell offset of ($58, $5A). */
static void WorldMapCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_Y); /* ADEE */
    And16(cpu, 0x003fu);
    ExchangeAccumulatorBytes(cpu);
    LsrA16(cpu);
    StoreADirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_X);
    And16(cpu, 0x003fu);
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_DP_CELL_OFFSET));
    AslA16(cpu);
    Add16Value(cpu, 0x0000u);
    StoreADirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
    SimulateRtsFrame(memory, cpu);
}

/* $86:AE05: $08 = map block pointer for ($58, $5A). */
static void WorldMapBlockPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_X); /* AE05 */
    And16(cpu, 0x00ffu);
    LsrA16(cpu);
    StoreADirect16(memory, cpu, STREAM_DP_BLOCK_POINTER);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_Y);
    And16(cpu, 0x00ffu);
    LsrA16(cpu);
    ExchangeAccumulatorBytes(cpu);
    LsrA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_DP_BLOCK_POINTER));
    AslA16(cpu);
    Add16Value(cpu, 0x4040u);
    StoreADirect16(memory, cpu, STREAM_DP_BLOCK_POINTER);
    SimulateRtsFrame(memory, cpu);
}

/* Metatile index for the current cell: [$DF] or [$E3] table. */
static void WorldMapMetatile(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16Long(memory,
                            ((uint32_t)cpu->data_bank << 16) +
                                Read16Direct(memory, cpu,
                                             STREAM_DP_BLOCK_POINTER))); /* LDA ($08) */
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x00u));
    TransferAToY(cpu);
    LoadA16(cpu, Read16IndirectLongY(
        memory, cpu, cpu->negative ? 0xe3u : 0xdfu));
    StoreADirect16(memory, cpu, 0x00u);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x00u));
    TransferAToY(cpu);
    LoadXDirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
}

/* $86:ACFE: stream one map column into $7F:DF00/$7F:DF80. */
static void WorldMapStreamColumn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x99eeu);
    PushAndSetDataBank(memory, cpu, 0x7fu);                    /* ACFE */
    SetAccumulatorWidth(cpu, 0);
    WorldMapCellOffset(memory, cpu, 0xad07u);
    WorldMapBlockPointer(memory, cpu, 0xad0au);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
    And16(cpu, 0xc07fu);
    Write16Long(memory, STREAM_COLUMN_VRAM, cpu->accumulator);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_X);
    And16(cpu, 0x0001u);
    AslA16(cpu);
    StoreADirect16(memory, cpu, STREAM_DP_SUBCELL);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_Y);
    PushAccumulator16(memory, cpu);
    And16(cpu, 0x003fu);
    AslA16(cpu);
    StoreADirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
    LoadA16(cpu, 0x0040u);
    StoreADirect16(memory, cpu, STREAM_DP_COUNT_LEFT);
    do {
        LoadADirect16(memory, cpu, STREAM_DP_CELL_Y); /* AD2A */
        And16(cpu, 0x0001u);
        AslA16(cpu);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, STREAM_DP_SUBCELL));
        StoreADirect16(memory, cpu, 0x00u);
        WorldMapMetatile(memory, cpu);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xe7u));
        StoreAAbsolute16(memory, cpu, WORLD_MAP_COLUMN_BUFFER, cpu->x);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xeau));
        StoreAAbsolute16(memory, cpu, (WORLD_MAP_COLUMN_BUFFER + WORLD_MAP_COLUMN_HALF),
                         cpu->x);
        LoadADirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
        IncrementA16(cpu);
        IncrementA16(cpu);
        And16(cpu, 0x007fu);
        StoreADirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
        SetAccumulatorWidth(cpu, 1);
        IncrementDirect8(memory, cpu, STREAM_DP_CELL_Y);
        if (cpu->zero) {
            SetAccumulatorWidth(cpu, 0);                       /* AD70 */
            WorldMapBlockPointer(memory, cpu, 0xad74u);
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, STREAM_DP_CELL_Y));
            LsrA8(cpu);
            if (!cpu->carry)
                IncrementDirect8(memory, cpu, 0x09u);
        }
        SetAccumulatorWidth(cpu, 0);                           /* AD75 */
        {
            const uint16_t left =
                (uint16_t)(Read16Direct(memory, cpu, STREAM_DP_COUNT_LEFT) - 1u);

            Write16Direct(memory, cpu, STREAM_DP_COUNT_LEFT, left);
            SetNz16(cpu, left);
        }
    } while (!cpu->zero);
    PullAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, STREAM_DP_CELL_X);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $86:AC6C: stream one map row into the $7F buffer at $0B. */
static void WorldMapStreamRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9a1fu);
    PushAndSetDataBank(memory, cpu, 0x7fu);                    /* AC6C */
    SetAccumulatorWidth(cpu, 0);
    WorldMapCellOffset(memory, cpu, 0xac75u);
    WorldMapBlockPointer(memory, cpu, 0xac78u);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
    And16(cpu, 0xff80u);
    Write16Long(memory, STREAM_ROW_VRAM, cpu->accumulator);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_Y);
    And16(cpu, 0x0001u);
    AslA16(cpu);
    AslA16(cpu);
    StoreADirect16(memory, cpu, STREAM_DP_SUBCELL);
    LoadADirect16(memory, cpu, STREAM_DP_CELL_X);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, 0x0040u);
    StoreADirect16(memory, cpu, STREAM_DP_COUNT_LEFT);
    do {
        LoadADirect16(memory, cpu, STREAM_DP_CELL_X); /* AC93 */
        And16(cpu, 0x0001u);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, STREAM_DP_SUBCELL));
        StoreADirect16(memory, cpu, 0x00u);
        WorldMapMetatile(memory, cpu);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xe7u));
        SetAccumulatorWidth(cpu, 1);
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        ExchangeAccumulatorBytes(cpu);
        StoreAAbsolute8(memory, cpu, 0x0080u, cpu->x);
        LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0xeau));
        StoreAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0xedu));
        StoreAAbsolute8(memory, cpu, 0x0081u, cpu->x);
        SetAccumulatorWidth(cpu, 0);                           /* ACCB */
        LoadADirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
        IncrementA16(cpu);
        IncrementA16(cpu);
        cpu->zero = (cpu->accumulator & 0x007fu) == 0;
        if (cpu->zero)
            Subtract16(cpu, 0x0080u);
        StoreADirect16(memory, cpu, STREAM_DP_CELL_OFFSET);
        SetAccumulatorWidth(cpu, 1);
        IncrementDirect8(memory, cpu, STREAM_DP_CELL_X);
        SetAccumulatorWidth(cpu, 0);
        if (cpu->zero) {
            WorldMapBlockPointer(memory, cpu, 0xace6u);
        } else {
            LoadADirect16(memory, cpu, STREAM_DP_CELL_X); /* ACEA */
            LsrA16(cpu);
            if (!cpu->carry) {
                uint16_t pointer = Read16Direct(memory, cpu, STREAM_DP_BLOCK_POINTER);

                pointer = (uint16_t)(pointer + 2u);
                Write16Direct(memory, cpu, STREAM_DP_BLOCK_POINTER, pointer);
                SetNz16(cpu, pointer);
            }
        }
        {
            const uint16_t left =
                (uint16_t)(Read16Direct(memory, cpu, STREAM_DP_COUNT_LEFT) - 1u);

            Write16Direct(memory, cpu, STREAM_DP_COUNT_LEFT, left);
            SetNz16(cpu, left);
        }
    } while (!cpu->zero);
    PullAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, STREAM_DP_CELL_X);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* Edge ahead of the move: position +$1F or -$1F. */
static void WorldMapEdge(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t position) {
    Compare8(cpu, A8(cpu), 0x80u);
    LoadAAbsolute8(memory, cpu, position, 0);
    if (!cpu->carry)
        Adc8(cpu, 0x1fu);
    else
        Sbc8(cpu, 0x1fu);
}

/* $86:99BF: stream the map edges the camera moved across. */
Lufia2ExecutionResult Lufia2WorldMapStreamEdges(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x869a43u;
    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = 0x8699bfu;
        return result;
    }
    Write8(memory, DirectAddress(cpu, 0x59u), 0x00u);          /* 99BF */
    Write8(memory, DirectAddress(cpu, 0x5bu), 0x00u);
    LoadAAbsolute8(memory, cpu, WORLD_MAP_CAMERA_CELL_Y, 0);
    cpu->carry = 1;
    Sbc8(cpu, 0x20u);
    StoreADirect8(memory, cpu, STREAM_DP_CELL_Y);
    LoadAAbsolute8(memory, cpu, WORLD_MAP_CAMERA_CELL_X, 0);
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, WORLD_MAP_STREAMED_CELL_X, 0));
    if (!cpu->zero) {
        WorldMapEdge(memory, cpu, WORLD_MAP_CAMERA_CELL_X);
        StoreADirect8(memory, cpu, STREAM_DP_CELL_X); /* 99EA */
        WorldMapStreamColumn(memory, cpu);
        StoreAImmediate8(memory, cpu, 0xffu, WORLD_MAP_COLUMN_PENDING);
    }
    LoadAAbsolute8(memory, cpu, WORLD_MAP_CAMERA_CELL_X, 0); /* 99F4 */
    cpu->carry = 1;
    Sbc8(cpu, 0x20u);
    StoreADirect8(memory, cpu, STREAM_DP_CELL_X);
    LoadAAbsolute8(memory, cpu, WORLD_MAP_CAMERA_CELL_Y, 0);
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, WORLD_MAP_STREAMED_CELL_Y, 0));
    if (!cpu->zero) {
        WorldMapEdge(memory, cpu, WORLD_MAP_CAMERA_CELL_Y);
        StoreADirect8(memory, cpu, STREAM_DP_CELL_Y); /* 9A1B */
        WorldMapStreamRow(memory, cpu);
        StoreAImmediate8(memory, cpu, 0xffu, WORLD_MAP_ROW_PENDING);
    }
    LoadAAbsolute8(memory, cpu, WORLD_MAP_CAMERA_CELL_X, 0); /* 9A25 */
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, WORLD_MAP_STREAMED_CELL_X, 0));
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, WORLD_MAP_CAMERA_CELL_Y, 0);
        Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, WORLD_MAP_STREAMED_CELL_Y, 0));
    }
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, WORLD_MAP_MOVE_COUNT, 0); /* 9A35 */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->carry)
            StoreAAbsolute8(memory, cpu, WORLD_MAP_MOVE_COUNT, 0);
    }
    SimulateJsrFrame(memory, cpu, 0x9a42u);                    /* 9A44 */
    CopyAbsolute8(memory, cpu, WORLD_MAP_CAMERA_CELL_X, WORLD_MAP_STREAMED_CELL_X);
    CopyAbsolute8(memory, cpu, WORLD_MAP_CAMERA_CELL_Y, WORLD_MAP_STREAMED_CELL_Y);
    SimulateRtsFrame(memory, cpu);
    return result;
}

enum {
    REGION_INDEX = 0x09ebu,
    REGION_POINT_X = 0x58u,
    REGION_POINT_Y = 0x5au,
    REGION_BANK_SCRATCH = 0x10u,
    REGION_ENTRY_SIZE = 9u,
    REGION_ENTRY_MIN_X = 1u,
    REGION_ENTRY_MIN_Y = 2u,
    REGION_ENTRY_MAX_X = 3u,
    REGION_ENTRY_MAX_Y = 4u,
};

/* DB at the map region list, X at entry one. */
static void WorldMapRegionList(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, REGION_INDEX, 0));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xce36u, cpu->y));
    AslA16(cpu);
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0xce36u, cpu->y));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0xcffcbeu, cpu->x)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StoreADirect8(memory, cpu, REGION_BANK_SCRATCH);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xcffcbcu, cpu->x)));
    TransferAToX(cpu);
    cpu->carry = 0;
}

/* Is point ($58, $5A) inside region X? */
static bool WorldMapRegionContains(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint8_t bounds[4] = {REGION_ENTRY_MIN_X, REGION_ENTRY_MAX_X,
                                      REGION_ENTRY_MIN_Y, REGION_ENTRY_MAX_Y};
    bool inside = true;

    for (unsigned i = 0; i < 4u && inside; ++i) {
        if (i == 0)
            LoadA8(cpu, DirectByte(memory, cpu, REGION_POINT_X));
        else if (i == 2)
            LoadA8(cpu, DirectByte(memory, cpu, REGION_POINT_Y));
        Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, bounds[i], cpu->x));
        inside = (i & 1u) ? !cpu->carry : cpu->carry;
    }
    return inside;
}

/* $86:9EDD: world map region holding ($58, $5A); carry clear = hit. */
Lufia2ExecutionResult Lufia2WorldMapRegionSearch(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    uint32_t entries;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x869eddu);
    PushDataBank(memory, cpu); /* 9EDD */
    SimulateJsrFrame(memory, cpu, 0x9ee0u);
    WorldMapRegionList(memory, cpu); /* 9F35 */
    SimulateRtsFrame(memory, cpu);
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0006u, cpu->x));
    TransferAToX(cpu);                                         /* 9EE4 */
    for (entries = 0;; ++entries) {
        /* A list without an end marker spins the ROM. */
        if (entries == 0x10000u) {
            SetAccumulatorWidth(cpu, 1);
            return ExecutionHandoff(cpu, 0x869ee7u);
        }
        SetAccumulatorWidth(cpu, 1);                           /* 9EE5 */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        if (cpu->negative) {
            cpu->carry = 1;                                    /* 9F10 */
            break;
        }
        if (WorldMapRegionContains(memory, cpu))
            break;
        SetAccumulatorWidth(cpu, 0);                           /* 9F04 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, REGION_ENTRY_SIZE);
        TransferAToX(cpu);
    }
    PullDataBank(memory, cpu);                                 /* 9F11 */
    return ExecutionReturned(0x869f12u);
}

/* Polar offset fields: angle and radius to x, y. */
enum {
    POLAR_ANGLE = 0x1248u,
    POLAR_RADIUS = 0x1249u,
    POLAR_DP_QUADRANT = 0x10u,
    POLAR_DP_SCALE_FACTOR = 0x50u,
    POLAR_DP_PRODUCT_LOW = 0x51u,
    POLAR_DP_ACCUMULATED = 0x52u,
    POLAR_DP_FIRST_PART = 0x0eu,
    POLAR_DP_RADIUS = 0x4eu,
    POLAR_DP_QUADRANT_ANGLE = 0x05u,
    POLAR_DP_OFFSET_X = 0x08u,
    POLAR_DP_OFFSET_Y = 0x0bu,
    POLAR_SCALE_TABLE = 0x97b226u,
};

/* $86:A583: $52 += $50 * $4E (16-bit) via $4202; low byte in $51. */
static void WorldScaleStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, DirectByte(memory, cpu, POLAR_DP_SCALE_FACTOR)); /* A583 */
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    LoadA8(cpu, DirectByte(memory, cpu, POLAR_DP_RADIUS));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x4fu));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    StoreXDirect16(memory, cpu, POLAR_DP_PRODUCT_LOW);
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    Write8(memory, DirectAddress(cpu, 0x53u), 0x00u);
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, POLAR_DP_ACCUMULATED);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
    StoreADirect16(memory, cpu, POLAR_DP_ACCUMULATED);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* A16 negated into a word at dp; its sign byte at dp + 2. */
static void WorldStoreNegated(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t from,
    uint8_t to) {
    LoadADirect16(memory, cpu, from);
    LoadA16(cpu, (uint16_t)~cpu->accumulator);
    IncrementA16(cpu);
    StoreADirect16(memory, cpu, to);
    SetAccumulatorWidth(cpu, 1);
    if (!cpu->zero)
        LoadA8(cpu, 0xffu);
    StoreADirect8(memory, cpu, (uint8_t)(to + 2u));
}

/* $86:A417: polar step. */
static void WorldPolarOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, POLAR_RADIUS, 0)); /* A417 */
    StoreXDirect16(memory, cpu, POLAR_DP_RADIUS);
    LoadAAbsolute8(memory, cpu, POLAR_ANGLE, 0);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    StoreADirect8(memory, cpu, POLAR_DP_QUADRANT); /* quadrant */
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, POLAR_ANGLE, 0);
    And8(cpu, 0x3fu);
    if (cpu->zero) {
        StoreXDirect16(memory, cpu, POLAR_DP_ACCUMULATED); /* A462 */
        LoadX16(cpu, 0x0000u);
        StoreXDirect16(memory, cpu, POLAR_DP_FIRST_PART);
    } else {
        StoreADirect8(memory, cpu, POLAR_DP_QUADRANT_ANGLE); /* A431 */
        AslA8(cpu);
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(POLAR_SCALE_TABLE, cpu->x)));
        StoreADirect8(memory, cpu, POLAR_DP_SCALE_FACTOR);
        WorldScaleStep(memory, cpu, 0xa43du);
        LoadXDirect16(memory, cpu, POLAR_DP_ACCUMULATED);
        StoreXDirect16(memory, cpu, POLAR_DP_FIRST_PART);
        LoadA8(cpu, 0x00u);
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x40u);
        cpu->carry = 1;
        Sbc8(cpu, DirectByte(memory, cpu, POLAR_DP_QUADRANT_ANGLE));
        AslA8(cpu);
        TransferAToX(cpu);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(POLAR_SCALE_TABLE, cpu->x)));
        StoreADirect8(memory, cpu, POLAR_DP_SCALE_FACTOR);
        WorldScaleStep(memory, cpu, 0xa454u);
    }
    LoadA8(cpu, DirectByte(memory, cpu, POLAR_DP_QUADRANT)); /* A455 */
    AslA8(cpu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x0006u);
    TransferAToX(cpu);
    SimulateJsrFrame(memory, cpu, 0xa460u);                    /* JSR ($A46B,x) */
    switch (cpu->x) {
    case 0:                                                    /* A473 */
        WorldStoreNegated(memory, cpu, POLAR_DP_FIRST_PART, POLAR_DP_OFFSET_X);
        SetAccumulatorWidth(cpu, 0);
        WorldStoreNegated(memory, cpu, POLAR_DP_ACCUMULATED, POLAR_DP_OFFSET_Y);
        break;
    case 2:                                                    /* A496 */
        LoadADirect16(memory, cpu, POLAR_DP_FIRST_PART);
        StoreADirect16(memory, cpu, POLAR_DP_OFFSET_Y);
        WorldStoreNegated(memory, cpu, POLAR_DP_ACCUMULATED, POLAR_DP_OFFSET_X);
        Write8(memory, DirectAddress(cpu, 0x0du), 0x00u);
        break;
    case 4:                                                    /* A4AD */
        LoadADirect16(memory, cpu, POLAR_DP_FIRST_PART);
        StoreADirect16(memory, cpu, POLAR_DP_OFFSET_X);
        LoadADirect16(memory, cpu, POLAR_DP_ACCUMULATED);
        StoreADirect16(memory, cpu, POLAR_DP_OFFSET_Y);
        SetAccumulatorWidth(cpu, 1);
        Write8(memory, DirectAddress(cpu, 0x0au), 0x00u);
        Write8(memory, DirectAddress(cpu, 0x0du), 0x00u);
        break;
    default:                                                   /* A4BC */
        LoadADirect16(memory, cpu, POLAR_DP_ACCUMULATED);
        StoreADirect16(memory, cpu, POLAR_DP_OFFSET_X);
        WorldStoreNegated(memory, cpu, POLAR_DP_FIRST_PART, POLAR_DP_OFFSET_Y);
        Write8(memory, DirectAddress(cpu, 0x0au), 0x00u);
        break;
    }
    SimulateRtsFrame(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* One chain link. */
static void WorldChainSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t follow) {
    if (follow) {
        LoadAAbsolute8(memory, cpu, 0x1e54u, cpu->x);          /* E967 */
        cpu->carry = 0;
        Adc8(cpu, DirectByte(memory, cpu, 0x09u));
        StoreAAbsolute8(memory, cpu, 0x1e58u, cpu->x);
    } else {
        LoadAAbsolute8(memory, cpu, 0x1e58u, cpu->x);          /* E99F */
    }
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, 0x1e5au, cpu->x));
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, 0x15u));
    StoreAAbsolute8(memory, cpu, 0x0100u, cpu->x);             /* OAM x */
    if (follow) {
        LoadAAbsolute8(memory, cpu, 0x1e55u, cpu->x);
        Adc8(cpu, DirectByte(memory, cpu, 0x0cu));
        StoreAAbsolute8(memory, cpu, 0x1e59u, cpu->x);
        cpu->carry = 0;
    } else {
        LoadAAbsolute8(memory, cpu, 0x1e59u, cpu->x);
    }
    Adc8(cpu, AbsoluteByte(memory, cpu, 0x1e5bu, cpu->x));
    cpu->carry = 0;
    Adc8(cpu, DirectByte(memory, cpu, 0x17u));
    Compare8(cpu, A8(cpu), 0xf8u);
    if (!cpu->carry) {
        Compare8(cpu, A8(cpu), 0xe0u);
        if (cpu->carry) {
            LoadA8(cpu, 0x3cu);
            StoreAAbsolute8(memory, cpu, 0x0102u, cpu->x);
        }
    }
    StoreAAbsolute8(memory, cpu, 0x0101u, cpu->x);             /* OAM y */
}

/* $86:E8CE: world map sprite chain. */
Lufia2ExecutionResult Lufia2WorldSpriteChain(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned axis;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86e8ceu);
    SetAccumulatorWidth(cpu, 0);                               /* E8CE */
    for (axis = 0; axis < 2u; ++axis) {
        const uint16_t shake = axis ? WORLD_MAP_SHAKE_Y : WORLD_MAP_SHAKE_X;

        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, shake, 0));
        if (!cpu->zero) {
            Subtract16(cpu, Read16AbsoluteIndexed(memory, cpu, (uint16_t)(shake + 4u),
                                                  0)); /* decay word of the axis */
            if (!cpu->carry)
                LoadA16(cpu, 0x0000u);
        }
        StoreAAbsolute16(memory, cpu, shake, 0);
        ExchangeAccumulatorBytes(cpu);
        And16(cpu, 0x00ffu);
        StoreADirect16(memory, cpu, axis ? 0x17u : 0x15u);
    }
    LoadA16(cpu,
            Read16AbsoluteIndexed(memory, cpu, WORLD_MAP_SHAKE_DECAY_X, 0)); /* E8FE */
    Subtract16(cpu, 0x0004u);
    StoreAAbsolute16(memory, cpu, WORLD_MAP_SHAKE_DECAY_X, 0);
    StepAbsolute16(memory, cpu, WORLD_MAP_SHAKE_DECAY_Y, -1);
    LoadADirect16(memory, cpu, DP_FRAME_COUNTER);
    Subtract16(cpu, 0x04aau);
    SetAccumulatorWidth(cpu, 1);
    if (cpu->carry) {
        PushAccumulator8(memory, cpu);                         /* E915 */
        LoadA8(cpu, (uint8_t)~A8(cpu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreADirect8(memory, cpu, 0x17u);
        LoadA8(cpu, Pull8(memory, cpu));
        LsrA8(cpu);
        StoreADirect8(memory, cpu, 0x00u);
        LsrA8(cpu);
        StoreADirect8(memory, cpu, 0x15u);
        LsrA8(cpu);
        cpu->carry = 0;
        Adc8(cpu, DirectByte(memory, cpu, 0x15u));
        StoreADirect8(memory, cpu, 0x15u);
        LoadAAbsolute8(memory, cpu, 0x1211u, 0);
        cpu->carry = 1;
        Sbc8(cpu, AbsoluteByte(memory, cpu, 0x1219u, 0));
        cpu->carry = 1;
        Sbc8(cpu, DirectByte(memory, cpu, 0x00u));
    } else {
        LoadAAbsolute8(memory, cpu, 0x1211u, 0);               /* E934 */
        cpu->carry = 1;
        Sbc8(cpu, AbsoluteByte(memory, cpu, 0x1219u, 0));
    }
    cpu->carry = 0;                                            /* E93B */
    Adc8(cpu, 0x80u);
    StoreAAbsolute8(memory, cpu, 0x1248u, 0);                  /* angle */
    LoadA8(cpu, 0x05u);
    SimulateJslFrame(memory, cpu, 0x86u, 0xe946u);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, 0x1248u, 0));
    StoreAAbsolute8(memory, cpu, 0x1248u, 0);
    LoadA8(cpu, 0x02u);
    SimulateJslFrame(memory, cpu, 0x86u, 0xe953u);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    cpu->carry = 0;
    Adc8(cpu, 0x03u);
    StoreAAbsolute8(memory, cpu, 0x124au, 0);                  /* radius */
    StoreZeroAbsolute8(memory, cpu, 0x1249u, 0);
    StoreZeroAbsolute8(memory, cpu, (WRAM_FIELD_STREAMED_COLUMN_VRAM + 1u), 0);
    WorldPolarOffset(memory, cpu, 0xe962u);
    SetIndexWidth(cpu, 1);                                     /* E963 */
    LoadX8(cpu, 0x54u);
    do {
        WorldChainSprite(memory, cpu, 1);
        LoadX8(cpu, (uint8_t)(cpu->x - 1u));
        LoadX8(cpu, (uint8_t)(cpu->x - 1u));
        LoadX8(cpu, (uint8_t)(cpu->x - 1u));
        LoadX8(cpu, (uint8_t)(cpu->x - 1u));
    } while (!cpu->zero);
    WorldChainSprite(memory, cpu, 0);                          /* head */
    LoadAAbsolute8(memory, cpu, 0x0102u, cpu->x);              /* E9C5 */
    Compare8(cpu, A8(cpu), 0x1fu);
    if (!cpu->zero)
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0x01u));
    StoreAAbsolute8(memory, cpu, 0x0102u, cpu->x);
    SetIndexWidth(cpu, 0);
    return ExecutionReturned(0x86e9d3u);                       /* RTS */
}
