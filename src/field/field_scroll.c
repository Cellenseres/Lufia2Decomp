/* Field BG scrolling and tile streaming ($8E:BD77). */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "field/field_internal.h"
#include "system/wram.h"

/* Direct-page scratch of the tile streaming routines ($80:F4FD-F7D0). A
 * layer is a grid of 16x16 cells; streaming copies the cells at one screen
 * edge into the layer's tilemap buffer, 32 tile rows by 64 tile columns. */
enum {
    STREAM_LAYER = 0x5du, /* layer index * 2 */
    STREAM_ROW_POINTER = 0x5du,
    STREAM_ROW_POINTER_NEXT = 0x60u,
    STREAM_METATILE_BASE = 0x65u,
    STREAM_LAYER_WIDTH = 0x83u,  /* in cells */
    STREAM_LAYER_HEIGHT = 0x85u, /* in cells */
    STREAM_CELL_X = 0x87u,
    STREAM_CELL_Y = 0x89u,
    STREAM_MAP_ROW_BYTES = 0x8bu,
    STREAM_CELL_BASE = 0x8du,
    STREAM_CELL_ADDRESS = 0x30u,
    STREAM_BUFFER_COLUMN = 0x22u,
    STREAM_BUFFER_ROW = 0x24u,
    STREAM_BUFFER_OFFSET = 0x2du,
    STREAM_BUFFER = 0x2au, /* 24-bit pointer, bank in the next byte */
    STREAM_BUFFER_BANK = 0x2cu,
    STREAM_ROW_TILES_LEFT = 0x26u,
    STREAM_COLUMN_TILES_LEFT = 0x28u,
    STREAM_RUN_LENGTH = 0x10u,
    STREAM_TILE_ROW_STEP = 0x3eu, /* next tile row, less the tile just written */
    STREAM_BUFFER_MASK = 0x07ffu,
    STREAM_CELL_SHARED_FLAGS = 0x3000u,
    STREAM_CELL_METATILE_MASK = 0x03ffu,
    COLUMN_STAGING_TABLE = 0x80f581u, /* per-layer column buffer addresses */
    LAYER_BUFFER_TABLE = 0x838ff0u,   /* per-layer tilemap buffer addresses */
    REDRAW_X = 0x11u,
    REDRAW_Y = 0x13u,
    REDRAW_LAYER = 0x15u,
    REDRAW_ROWS_LEFT = 0x17u,
    REDRAW_X_SLOT_TABLE = 0x80f4edu,
    REDRAW_Y_SLOT_TABLE = 0x80f4f5u,
};

/* $8E:BF88: shift count from $8E:BF93 into $4E. */
static void ScrollShiftCount(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TransferAToX(cpu);                                         /* BF88 */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ebf93u, cpu->x)));
    And16(cpu, 0x00ffu);
    Write16Direct(memory, cpu, 0x4eu, cpu->accumulator);
    SimulateRtsFrame(memory, cpu);
}

/* $8E:BF70: signed shift right by $4E; C=1 skips. */
static void ScrollShiftRight(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0x4eu)));    /* BF70 */
    if (cpu->zero) {
        TransferDirectToA(cpu);                                /* BF85 */
        cpu->carry = 0;
    } else if (cpu->negative) {
        cpu->carry = 1;                                        /* BF76 */
    } else {
        do {
            const uint16_t old = cpu->accumulator;             /* BF78 */

            SetNz16(cpu, old);
            LoadA16(cpu, (uint16_t)((old >> 1) | (old & 0x8000u)));
            cpu->carry = old & 1u;
            LoadY8(cpu, (uint8_t)(cpu->y - 1u));
        } while (!cpu->zero);
        cpu->carry = 0;
    }
    SimulateRtsFrame(memory, cpu);
}

/* $8E:BFDE: shift left by $4E. */
static void ScrollShiftLeft(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadY8(cpu, Read8(memory, DirectAddress(cpu, 0x4eu)));    /* BFDE */
    while (!cpu->zero) {
        /* ORA/CLC/SEC before ASL are dead. */
        AslA16(cpu);                                           /* BFE9 */
        LoadY8(cpu, (uint8_t)(cpu->y - 1u));
    }
    SimulateRtsFrame(memory, cpu);
}

/* Step current toward the target by the speed word. */
static void ScrollStepToward(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t current,
    uint8_t out,
    uint32_t speed) {
    const uint8_t down = cpu->negative;

    LoadA16(cpu, Read16Direct(memory, cpu, current));
    if (down) {
        Subtract16(cpu, Read16Long(memory, speed));
    } else {
        cpu->carry = 0;
        Add16Value(cpu, Read16Long(memory, speed));
    }
    Write16Direct(memory, cpu, out, cpu->accumulator);
}

/* $8E:BE78: layer follows the camera target. */
static void ScrollModeFollow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_SCREEN_EFFECTS, 0)); /* BE78 */
    cpu->zero = (cpu->accumulator & 0x0040u) == 0;
    if (!cpu->zero) {
        LoadXDirect(memory, cpu, STREAM_LAYER); /* BE80 */
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd0ceu, cpu->x)));
        Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
        if (!cpu->zero)
            ScrollStepToward(memory, cpu, 0x54u, 0x58u,
                LongIndexedAddress(0x7fd0deu, cpu->x));
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd0d6u, cpu->x)));
        Write16Direct(memory, cpu, 0x5au, cpu->accumulator);   /* BEA2 */
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x56u));
        if (!cpu->zero)
            ScrollStepToward(memory, cpu, 0x56u, 0x5au,
                LongIndexedAddress(0x7fd0e6u, cpu->x));
        return;
    }
    LoadXDirect(memory, cpu, STREAM_LAYER); /* BEC4 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a8u, 0));
    if (cpu->zero) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_X, 0));
        Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_Y, 0));
        Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    } else {
        const uint32_t speed = AbsoluteIndexedAddress(cpu, 0x05a8u, 0);

        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_X, 0));
        Write16Direct(memory, cpu, 0x58u, cpu->accumulator);   /* BED7 */
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
        if (!cpu->zero) {
            ScrollStepToward(memory, cpu, 0x54u, 0x58u, speed);
        } else {
            LoadA16(cpu,
                    Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_Y, 0));
            Write16Direct(memory, cpu, 0x5au, cpu->accumulator); /* BEF6 */
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x56u));
            if (!cpu->zero) {
                const uint8_t down = cpu->negative;

                ScrollStepToward(memory, cpu, 0x56u, 0x5au, speed);
                if (down)
                    return;                                    /* BF09 */
            }
        }
    }
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_X,
                                       0)); /* BF12 */
    Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
    if (!cpu->zero)
        return;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_Y, 0));
    Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x56u));
    if (!cpu->zero)
        return;
    Write16Absolute(memory, cpu, 0x05a8u, 0x0000u);
}

/* $8E:BF25/BF9B tail: camera + $80 unless equal. */
static void ScrollOffsetTarget(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t current,
    uint8_t out) {
    cpu->carry = 0;
    Add16Value(cpu, 0x0080u);
    Subtract16(cpu, Read16Direct(memory, cpu, current));
    if (cpu->zero)
        return;
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, current));
    Write16Direct(memory, cpu, out, cpu->accumulator);
}

/* Layer nibble of $7F:D021,x. */
static void ScrollLayerNibble(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t high) {
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(
                                        (WRAM_FIELD_LAYER_SECTION_WORD + 1u), cpu->x)));
    if (high) {
        And16(cpu, 0x00f0u);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
    } else {
        And16(cpu, 0x000fu);
    }
}

/* $8E:BF25: parallax, camera shifted right. */
static void ScrollModeParallax(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, STREAM_LAYER); /* BF25 */
    ScrollLayerNibble(memory, cpu, 1);
    ScrollShiftCount(memory, cpu, 0xbf34u);
    if (!cpu->zero) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_X, 0));
        ScrollShiftRight(memory, cpu, 0xbf3cu);
        if (cpu->carry)
            Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
        else
            ScrollOffsetTarget(memory, cpu, 0x54u, 0x58u);
    }
    LoadXDirect(memory, cpu, STREAM_LAYER); /* BF4D */
    ScrollLayerNibble(memory, cpu, 0);
    ScrollShiftCount(memory, cpu, 0xbf58u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_Y, 0));
    ScrollShiftRight(memory, cpu, 0xbf5eu);
    if (cpu->carry)
        Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    else
        ScrollOffsetTarget(memory, cpu, 0x56u, 0x5au);
}

/* $8E:BF9B: camera shifted left. */
static void ScrollModeScaled(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, STREAM_LAYER); /* BF9B */
    ScrollLayerNibble(memory, cpu, 1);
    ScrollShiftCount(memory, cpu, 0xbfaau);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_X, 0));
    ScrollShiftLeft(memory, cpu, 0xbfb0u);
    ScrollOffsetTarget(memory, cpu, 0x54u, 0x58u);
    /* X still holds the high nibble. */
    ScrollLayerNibble(memory, cpu, 0);                         /* BFBF */
    ScrollShiftCount(memory, cpu, 0xbfc8u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_Y, 0));
    ScrollShiftLeft(memory, cpu, 0xbfceu);
    ScrollOffsetTarget(memory, cpu, 0x56u, 0x5au);
}

/* One axis of $8E:BFEE: offset, wrap at map size. */
static void ScrollWrapAxis(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t current,
    uint8_t out,
    uint32_t size) {
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8ec04du, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, current));
    And16(cpu, 0x7fffu);
    Write16Direct(memory, cpu, out, cpu->accumulator);
    LoadXDirect(memory, cpu, STREAM_LAYER);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(size, cpu->x)));
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, out));
    if (cpu->carry)
        return;
    Subtract16(cpu, Read16Direct(memory, cpu, out));
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    IncrementA16(cpu);
    Write16Direct(memory, cpu, out, cpu->accumulator);
}

/* $8E:BFEE: fixed offsets, wrapped by map size. */
static void ScrollModeWrap(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadXDirect(memory, cpu, STREAM_LAYER); /* BFEE */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(
                                        (WRAM_FIELD_LAYER_SECTION_WORD + 1u), cpu->x)));
    And16(cpu, 0x00f0u);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    ScrollWrapAxis(memory, cpu, 0x54u, 0x58u, WRAM_FIELD_LAYER_WIDTH);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(
                                        (WRAM_FIELD_LAYER_SECTION_WORD + 1u), cpu->x)));
    And16(cpu, 0x000fu);                                       /* C01F */
    AslA16(cpu);
    ScrollWrapAxis(memory, cpu, 0x56u, 0x5au, WRAM_FIELD_LAYER_HEIGHT);
}

/* $80:F81C: divider settle delay; leaves M=0. */
static void StreamDelay(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x00u)));    /* F81C */
    SetAccumulatorWidth(cpu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* A mod divisor via $4204/$4206; negative A wraps. */
static void StreamWrap(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t divisor,
    uint16_t negative_return,
    uint16_t positive_return,
    uint8_t short_cut) {
    cpu->zero = (cpu->accumulator & 0x0800u) == 0;
    if (!cpu->zero) {
        LoadA16(cpu, (uint16_t)(cpu->accumulator | 0xf000u));
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        IncrementA16(cpu);
        Write16Long(memory, SNES_WRDIVL, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Read8(memory, DirectAddress(cpu, divisor)));
        Write8(memory, SNES_WRDIVB, A8(cpu));
        StreamDelay(memory, cpu, negative_return);
        LoadA16(cpu, Read16Direct(memory, cpu, divisor));
        Subtract16(cpu, Read16Long(memory, SNES_RDMPYL));
        return;
    }
    if (short_cut) {
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, divisor));
        if (!cpu->carry)
            return;
    }
    Write16Long(memory, SNES_WRDIVL, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, divisor)));
    Write8(memory, SNES_WRDIVB, A8(cpu));
    StreamDelay(memory, cpu, positive_return);
    LoadA16(cpu, Read16Long(memory, SNES_RDMPYL));
}

/* $80:F734: map cell and buffer offsets for A=x, Y=y. */
static void StreamLocate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LsrA16(cpu);                                               /* F734 */
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    Write16Direct(memory, cpu, STREAM_CELL_ADDRESS, cpu->accumulator);
    Write16Direct(memory, cpu, STREAM_BUFFER_COLUMN, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_WIDTH & 0xffffu), cpu->x));
    Write16Direct(memory, cpu, STREAM_LAYER_WIDTH, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_HEIGHT & 0xffffu), cpu->x));
    Write16Direct(memory, cpu, STREAM_LAYER_HEIGHT, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_COLUMN));
    StreamWrap(memory, cpu, STREAM_LAYER_WIDTH, 0xf762u, 0xf77au, 0);
    Write16Direct(memory, cpu, STREAM_CELL_X, cpu->accumulator); /* F77F */
    Write16Direct(memory, cpu, STREAM_ROW_TILES_LEFT, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_CELL_ADDRESS));
    AslA16(cpu);
    AslA16(cpu);
    And16(cpu, 0x003eu);
    Write16Direct(memory, cpu, STREAM_BUFFER_COLUMN, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    Write16Direct(memory, cpu, STREAM_COLUMN_TILES_LEFT, cpu->accumulator);
    StreamWrap(memory, cpu, STREAM_LAYER_HEIGHT, 0xf7adu, 0xf7c9u, 1);
    Write16Direct(memory, cpu, STREAM_CELL_Y, cpu->accumulator); /* F7CE */
    Write16Direct(memory, cpu, STREAM_COLUMN_TILES_LEFT, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0x00f0u);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Write16Direct(memory, cpu, STREAM_BUFFER_ROW, cpu->accumulator);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_COLUMN));
    Write16Direct(memory, cpu, STREAM_BUFFER_OFFSET, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(LAYER_BUFFER_TABLE, cpu->x)));
    Write16Direct(memory, cpu, STREAM_BUFFER, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, STREAM_COLUMN_TILES_LEFT)));
    Write8(memory, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, STREAM_LAYER_WIDTH)));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    LoadA8(cpu, 0x7eu);
    Write8(memory, DirectAddress(cpu, STREAM_BUFFER_BANK), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_CELL_BASE & 0xffffu), cpu->x));
    Write16Direct(memory, cpu, STREAM_CELL_BASE, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, SNES_RDMPYL));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_ROW_TILES_LEFT));
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_CELL_BASE));
    Write16Direct(memory, cpu, STREAM_CELL_ADDRESS, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_METATILE_BASE & 0xffffu), 0));
    Write16Direct(memory, cpu, STREAM_METATILE_BASE, cpu->accumulator);
    Compare16(cpu, cpu->x, 0x0004u);
    if (cpu->carry) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_ALTERNATE_METATILE_BASE & 0xffffu), 0));
        Write16Direct(memory, cpu, STREAM_METATILE_BASE, cpu->accumulator);
    }
    cpu->carry = 0;
    SimulateRtsFrame(memory, cpu);
}

/* $80:F6AA: X = map cell for ($87, $89). */
static void StreamCellIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 1);                               /* F6AA */
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, STREAM_CELL_Y)));
    Write8(memory, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, STREAM_LAYER_WIDTH)));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_CELL_X));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, SNES_RDMPYL));
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_CELL_BASE));
    TransferAToX(cpu);
    SimulateRtsFrame(memory, cpu);
}

/* One 16x16 metatile from cell X into [$2A],y. */
static void StreamMetatile(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    And16(cpu, STREAM_CELL_SHARED_FLAGS);
    Compare16(cpu, cpu->accumulator, STREAM_CELL_SHARED_FLAGS);
    if (cpu->zero)
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_CELL_BASE & 0xffffu), 0));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    And16(cpu, STREAM_CELL_METATILE_MASK);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_METATILE_BASE));
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER),
                cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0004u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER),
                cpu->accumulator);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, 0x003eu);
    TransferAToY(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0002u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER),
                cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0006u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER),
                cpu->accumulator);
}

/* Advance a wrapped map coordinate; refresh X on wrap. */
static void StreamStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t coordinate,
    uint8_t limit,
    uint16_t return_address) {
    LoadA16(cpu, Read16Direct(memory, cpu, coordinate));
    IncrementA16(cpu);
    Write16Direct(memory, cpu, coordinate, cpu->accumulator);
    if (!cpu->zero) {
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, limit));
        if (!cpu->carry)
            return;
        Write16Direct(memory, cpu, coordinate, 0x0000u);
    }
    StreamCellIndex(memory, cpu, return_address);
}

/* $80:F5ED: 16 metatiles down a column. */
static void StreamColumnTiles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA16(cpu, STREAM_RUN_LENGTH); /* F5ED */
    Write16Direct(memory, cpu, STREAM_COLUMN_TILES_LEFT, cpu->accumulator);
    do {
        PushIndex(memory, cpu);                                /* F5F2 */
        StreamMetatile(memory, cpu);
        LoadA16(cpu, cpu->y);                                  /* F62B */
        cpu->carry = 0;
        Add16Value(cpu, STREAM_TILE_ROW_STEP);
        And16(cpu, STREAM_BUFFER_MASK);
        TransferAToY(cpu);
        PullAccumulator16(memory, cpu);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, STREAM_MAP_ROW_BYTES));
        TransferAToX(cpu);
        StreamStep(memory, cpu, STREAM_CELL_Y, STREAM_LAYER_HEIGHT, 0xf648u);
        OpStepMem(memory, cpu, OpDp(cpu, STREAM_COLUMN_TILES_LEFT), -1); /* F649 */
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* $80:F64E: A metatiles along a row. */
static void StreamRowTiles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Write16Direct(memory, cpu, STREAM_ROW_TILES_LEFT, cpu->accumulator); /* F64E */
    do {
        PushIndex(memory, cpu);                                /* F650 */
        StreamMetatile(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);                  /* F689 */
        IncrementX16(cpu);
        IncrementX16(cpu);
        LoadA16(cpu, cpu->y);
        Subtract16(cpu, STREAM_TILE_ROW_STEP);
        And16(cpu, STREAM_BUFFER_MASK);
        TransferAToY(cpu);
        StreamStep(memory, cpu, STREAM_CELL_X, STREAM_LAYER_WIDTH, 0xf6a4u);
        OpStepMem(memory, cpu, OpDp(cpu, STREAM_ROW_TILES_LEFT), -1); /* F6A5 */
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* PHP; PHB; DB=$7F; REP #$30. */
static void StreamEnter(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadXDirect(memory, cpu, STREAM_LAYER);
}

/* $80:F4FD/F518: stream a column at the right/left edge. */
static void StreamColumn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t right) {
    StreamEnter(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SCROLL_Y, cpu->x)));
    TransferAToY(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SCROLL_X, cpu->x)));
    if (right) {
        cpu->carry = 0;
        Add16Value(cpu, 0x0100u);
    }
    StreamLocate(memory, cpu, 0xf52fu);                        /* F52D */
    PushIndex(memory, cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_LAYER_WIDTH));
    AslA16(cpu);
    Write16Direct(memory, cpu, STREAM_MAP_ROW_BYTES, cpu->accumulator);
    LoadXDirect(memory, cpu, STREAM_CELL_ADDRESS);
    LoadYDirect16(memory, cpu, STREAM_BUFFER_OFFSET);
    StreamColumnTiles(memory, cpu, 0xf53cu);
    cpu->x = PullIndexValue(memory, cpu);                      /* F53D */
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_COLUMN));
    TransferAToY(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_BUFFER));
    Write16Direct(memory, cpu, STREAM_BUFFER_OFFSET, cpu->accumulator);
    StoreXDirect16(memory, cpu, STREAM_CELL_ADDRESS);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(COLUMN_STAGING_TABLE, cpu->x)));
    Write16Long(memory, LongIndexedAddress(WRAM_FIELD_STREAMED_COLUMN_SOURCE, cpu->x),
                cpu->accumulator);
    TransferAToX(cpu);
    LoadA16(cpu, 0x0020u);
    Write16Direct(memory, cpu, STREAM_BUFFER_COLUMN, cpu->accumulator);
    do {
        LoadA16(cpu,
                Read16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER)));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x),
            cpu->accumulator);
        IncrementY16(cpu);
        IncrementY16(cpu);
        LoadA16(cpu,
                Read16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_BUFFER)));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0040u, cpu->x),
            cpu->accumulator);
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, 0x003eu);
        TransferAToY(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, STREAM_BUFFER_COLUMN), -1);
    } while (!cpu->zero);
    LoadXDirect(memory, cpu, STREAM_CELL_ADDRESS); /* F56E */
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_OFFSET));
    Write16Long(memory, LongIndexedAddress(WRAM_FIELD_STREAMED_COLUMN_VRAM, cpu->x),
                cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(COLUMN_STAGING_TABLE, cpu->x)));
    Write16Long(memory, LongIndexedAddress(WRAM_FIELD_STREAMED_COLUMN_SOURCE, cpu->x),
                cpu->accumulator);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $80:F589/F5A2: stream a row at the top/bottom edge. */
static void StreamRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t down) {
    StreamEnter(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SCROLL_Y, cpu->x)));
    if (down) {
        cpu->carry = 0;
        Add16Value(cpu, 0x00f0u);
    } else {
        Subtract16(cpu, 0x0010u);
    }
    And16(cpu, 0xfff0u);
    TransferAToY(cpu);                                         /* F5B9 */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SCROLL_X, cpu->x)));
    StreamLocate(memory, cpu, 0xf5c0u);
    LoadA16(cpu, 0x0040u);                                     /* F5C1 */
    Subtract16(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_COLUMN));
    LsrA16(cpu);
    LsrA16(cpu);
    LoadXDirect(memory, cpu, STREAM_CELL_ADDRESS);
    LoadYDirect16(memory, cpu, STREAM_BUFFER_OFFSET);
    StreamRowTiles(memory, cpu, 0xf5cfu);
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_COLUMN)); /* F5D0 */
    LsrA16(cpu);
    LsrA16(cpu);
    if (!cpu->zero) {
        /* X continues from the first run. */
        LoadYDirect16(memory, cpu, STREAM_BUFFER_ROW);
        StreamRowTiles(memory, cpu, 0xf5dau);
    }
    LoadA16(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_ROW)); /* F5DB */
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_BUFFER));
    LoadXDirect(memory, cpu, STREAM_LAYER);
    Write16Long(memory, LongIndexedAddress(WRAM_FIELD_STREAMED_ROW_SOURCE, cpu->x),
                cpu->accumulator);
    Write16Long(memory, LongIndexedAddress(WRAM_FIELD_STREAMED_ROW_VRAM, cpu->x),
                cpu->accumulator);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $80:F4FD: streams the layer's column at its right edge; returns to $80:F580. */
Lufia2ExecutionResult Lufia2FieldStreamRightColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StreamColumn(memory, cpu, 1);
    return ExecutionReturned(0x80f580u);
}

/* $80:F518: streams the layer's column at its left edge; returns to $80:F580. */
Lufia2ExecutionResult Lufia2FieldStreamLeftColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StreamColumn(memory, cpu, 0);
    return ExecutionReturned(0x80f580u);
}

/* $80:F589: streams the layer's row at its top edge; returns to $80:F5EC. */
Lufia2ExecutionResult Lufia2FieldStreamTopRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StreamRow(memory, cpu, 0);
    return ExecutionReturned(0x80f5ecu);
}

/* Streams the layer's row at its bottom edge; returns to $80:F5EC. */
Lufia2ExecutionResult Lufia2FieldStreamBottomRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StreamRow(memory, cpu, 1);
    return ExecutionReturned(0x80f5ecu);
}

/* $80:F6C6: 16 metatiles into the row buffers. */
static void StreamRowBuffers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA16(cpu, STREAM_RUN_LENGTH); /* F6C6 */
    Write16Direct(memory, cpu, STREAM_ROW_TILES_LEFT, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0xffc0u);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, STREAM_BUFFER));
    Write16Direct(memory, cpu, STREAM_ROW_POINTER, cpu->accumulator);
    cpu->carry = 0;
    Add16Value(cpu, 0x0040u);
    Write16Direct(memory, cpu, STREAM_ROW_POINTER_NEXT, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0x003fu);
    TransferAToY(cpu);
    do {
        PushIndex(memory, cpu);                                /* F6DF */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
        And16(cpu, STREAM_CELL_SHARED_FLAGS);
        Compare16(cpu, cpu->accumulator, STREAM_CELL_SHARED_FLAGS);
        if (cpu->zero)
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_CELL_BASE & 0xffffu), 0));
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
        And16(cpu, STREAM_CELL_METATILE_MASK);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, STREAM_METATILE_BASE));
        TransferAToX(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
        Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_ROW_POINTER),
                    cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0002u, cpu->x));
        Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_ROW_POINTER_NEXT),
                    cpu->accumulator);
        LoadA16(cpu, (uint16_t)(cpu->y + 2u));
        And16(cpu, 0x003fu);
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0004u, cpu->x));
        Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_ROW_POINTER),
                    cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0006u, cpu->x));
        Write16Long(memory, DirectLongIndirectY(memory, cpu, STREAM_ROW_POINTER_NEXT),
                    cpu->accumulator);
        cpu->x = PullIndexValue(memory, cpu);                  /* F715 */
        LoadA16(cpu, (uint16_t)(cpu->y + 2u));
        And16(cpu, 0x003fu);
        TransferAToY(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, STREAM_CELL_X) + 1u));
        Write16Direct(memory, cpu, STREAM_CELL_X, cpu->accumulator);
        if (cpu->zero) {
            StreamCellIndex(memory, cpu, 0xf72eu);
        } else {
            Compare16(cpu, cpu->accumulator,
                      Read16Direct(memory, cpu, STREAM_LAYER_WIDTH));
            if (cpu->carry) {
                Write16Direct(memory, cpu, STREAM_CELL_X, 0x0000u);
                StreamCellIndex(memory, cpu, 0xf72eu);         /* F72C */
            }
        }
        OpStepMem(memory, cpu, OpDp(cpu, STREAM_ROW_TILES_LEFT), -1); /* F72F */
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* $80:F47A: redraw layer X in 16 rows. */
static void RedrawLayerBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushAccumulator8(memory, cpu);                             /* F47A */
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SECTION_WORD,
        cpu->x)));
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 0);                           /* F487 */
        SetIndexWidth(cpu, 0);
        StoreXDirect16(memory, cpu, REDRAW_LAYER);
        LoadA16(cpu,
                Read16Long(memory, LongIndexedAddress(REDRAW_X_SLOT_TABLE, cpu->x)));
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_LAYER_SCROLL_X, cpu->x));
        cpu->carry = 0;
        Add16Value(cpu, 0x0008u);
        Write16Absolute(memory, cpu, (uint16_t)(0x0594u + cpu->y), cpu->accumulator);
        And16(cpu, 0xfff0u);
        Write16Direct(memory, cpu, REDRAW_X, cpu->accumulator);
        LoadA16(cpu,
                Read16Long(memory, LongIndexedAddress(REDRAW_Y_SLOT_TABLE, cpu->x)));
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_LAYER_SCROLL_Y, cpu->x));
        Write16Absolute(memory, cpu, (uint16_t)(0x0596u + cpu->y), cpu->accumulator);
        And16(cpu, 0xfff0u);
        Write16Direct(memory, cpu, REDRAW_Y, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);                           /* F4AF */
        LoadA8(cpu, 0x7eu);
        Write8(memory, DirectAddress(cpu, 0x5fu), A8(cpu));
        Write8(memory, DirectAddress(cpu, 0x62u), A8(cpu));
        LoadA8(cpu, 0x7fu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_WIDTH & 0xffffu), cpu->x));
        AslA16(cpu);
        Subtract16(cpu, 0x0020u);
        Write16Direct(memory, cpu, STREAM_COLUMN_TILES_LEFT, cpu->accumulator);
        LoadA16(cpu, 0x0010u);
        Write16Direct(memory, cpu, REDRAW_ROWS_LEFT, cpu->accumulator);
        do {
            LoadA16(cpu, Read16Direct(memory, cpu, REDRAW_Y)); /* F4CC */
            TransferAToY(cpu);
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            Write16Direct(memory, cpu, REDRAW_Y, cpu->accumulator);
            LoadXDirect(memory, cpu, REDRAW_LAYER);
            LoadA16(cpu, Read16Direct(memory, cpu, REDRAW_X));
            StreamLocate(memory, cpu, 0xf4dbu);
            LoadXDirect(memory, cpu, STREAM_CELL_ADDRESS);
            LoadY16(cpu, Read16Direct(memory, cpu, STREAM_BUFFER_OFFSET));
            StreamRowBuffers(memory, cpu, 0xf4e2u);
            OpStepMem(memory, cpu, OpDp(cpu, REDRAW_ROWS_LEFT), -1);
        } while (!cpu->zero);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* F4E7 */
    PullDataBank(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
}

/* Redraws the layer selected by X and returns to $80:F4EC. */
Lufia2ExecutionResult Lufia2FieldRedrawLayer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    RedrawLayerBody(memory, cpu);
    return ExecutionReturned(0x80f4ecu);
}

/* Redraws layers 3 to 0 (X = 6, 4, 2, 0) and returns to $83:8E75. */
Lufia2ExecutionResult Lufia2FieldRedrawAllLayers(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, 0x0006u);
    do {
        SimulateJslFrame(memory, cpu, 0x83u, 0x8e6fu);
        RedrawLayerBody(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadX16(cpu, (uint16_t)(cpu->x - 2u));
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x838e75u);
}

/* $83:8E66: redraw layers 3 to 0; X ends at $FFFE. */
void Lufia2FieldRedrawLayers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    (void)Lufia2FieldRedrawAllLayers(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:9004: pixel coordinate in A to cell index, A / 16; an entry with the
 * negative flag set uses the direct-page register in place of A. */
Lufia2ExecutionResult Lufia2FieldPixelCellFloor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    (void)memory;
    /* BPL observes the entry flag, which need not match the accumulator. */
    if (cpu->negative)
        TransferDirectToA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    OpLsrA(cpu);
    return ExecutionReturned(0x83900bu);
}

/* $83:9000: like the floor variant but adds 15 first, so it rounds up. */
Lufia2ExecutionResult Lufia2FieldPixelCellCeiling(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    cpu->carry = 0;
    OpAdcValue(cpu, 0x000fu);
    return Lufia2FieldPixelCellFloor(memory, cpu);
}

/* $83:9000 (rounding up) / $83:9004: A / 16, negative as D. */
static void RegionCell(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint8_t round_up) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (round_up)
        (void)Lufia2FieldPixelCellCeiling(memory, cpu);
    else
        (void)Lufia2FieldPixelCellFloor(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* Runs the cell conversion under a pushed JSR frame and hands off to the ROM if
 * the frame comes back changed. */
static Lufia2ExecutionResult CheckedRegionCell(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t frame, uint8_t round_up) {
    uint8_t low, high;
    uint16_t actual;
    SimulateJsrFrame(memory, cpu, frame);
    if (round_up)
        (void)Lufia2FieldPixelCellCeiling(memory, cpu);
    else
        (void)Lufia2FieldPixelCellFloor(memory, cpu);
    low = Pull8(memory, cpu);
    high = Pull8(memory, cpu);
    actual = (uint16_t)(low | ((uint16_t)high << 8));
    if (actual != frame)
        return ExecutionHandoff(cpu, 0x830000u | (uint16_t)(actual + 1u));
    return ExecutionReturned(0x83900bu);
}

/* Read-only $83:9000/9004. */
static uint16_t RegionCellValue(
    const Lufia2CpuState *cpu, uint16_t value, uint8_t round_up) {
    if (round_up)
        value = (uint16_t)(value + 0x000fu);
    if (value & 0x8000u)
        value = cpu->direct_page;
    return (uint16_t)(value >> 4);
}

/* Cells $83:8E85 draws for layer X. */
uint32_t Lufia2FieldRegionCells(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu) {
    const uint16_t origin = Read16Long(memory, WRAM_FIELD_PENDING_OBJECT_X);
    const uint16_t end = (uint16_t)(Read16Long(memory,
        WRAM_FIELD_OBJECT_WIDTH) + origin);
    uint8_t low[2], high[2], first[2], last[2];
    uint16_t size;
    unsigned axis;

    if (cpu->x > 6u)
        return 0xffffffffu;
    {
        const uint16_t x = Read16Long(memory, WRAM_FIELD_LAYER_SCROLL_X + cpu->x);
        const uint16_t y = Read16Long(memory, WRAM_FIELD_LAYER_SCROLL_Y + cpu->x);

        first[0] = (uint8_t)RegionCellValue(cpu, x, 1);
        first[1] = (uint8_t)RegionCellValue(cpu, y, 0);
        last[0] = (uint8_t)RegionCellValue(cpu, (uint16_t)(x + 0x0100u), 1);
        last[1] = (uint8_t)RegionCellValue(cpu, (uint16_t)(y + 0x00ffu), 0);
    }
    low[0] = (uint8_t)origin;
    low[1] = (uint8_t)(origin >> 8);
    high[0] = (uint8_t)end;
    high[1] = (uint8_t)(end >> 8);
    for (axis = 0; axis < 2u; ++axis) {
        if (!((uint8_t)(first[axis] - low[axis]) & 0x80u)) {
            if (!((uint8_t)(first[axis] - high[axis]) & 0x80u))
                return 0;
            low[axis] = first[axis];
        } else {
            if ((uint8_t)(last[axis] - low[axis]) & 0x80u)
                return 0;
            if ((uint8_t)(last[axis] - high[axis]) & 0x80u)
                high[axis] = last[axis];
        }
    }
    size = (uint16_t)(((uint16_t)high[1] << 8 | high[0]) -
                      ((uint16_t)low[1] << 8 | low[0]));
    return (uint32_t)(size & 0xffu) * (size >> 8);
}

enum {
    REGION_VISIBLE_FIRST_X = 0x8f,
    REGION_VISIBLE_FIRST_Y = 0x91,
    REGION_VISIBLE_LAST_X = 0x95,
    REGION_VISIBLE_LAST_Y = 0x96,
    REGION_CLIPPED_FIRST_X = 0x9f,
    REGION_CLIPPED_FIRST_Y = 0xa0,
    REGION_CLIPPED_LAST_X = 0xa1,
    REGION_TILEMAP_ORIGIN = 0x54,
    REGION_LAYER_CELL_BASE = 0x56,
    REGION_SIZE_DELTA = 0x54,
    REGION_HEIGHT_DELTA = 0x55,
    REGION_SOURCE_CELL = 0x54,
    REGION_COLUMNS = 0x56,
    REGION_COLUMNS_REMAINING = 0x58,
    REGION_ROWS_REMAINING = 0x5a,
    REGION_BOTTOM_ROW = 0x5d,
    REGION_BOTTOM_ROW_BANK = 0x5f,
    REGION_TOP_ROW = 0x60,
    REGION_TOP_ROW_BANK = 0x62,
    REGION_SOURCE_ROW_SKIP = 0x63,
    REGION_TILEMAP_COLUMN = 0x65,
};

/* Clips one axis of the region: raises the lower bound to the first cell or
 * lowers the upper bound to the last. Returns 0 when the region lies wholly
 * outside the bounds. */
static uint8_t RegionClipAxis(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t first, uint8_t last, uint8_t minimum, uint8_t maximum) {
    OpLda(memory, cpu, OpDp(cpu, first));
    OpCmp(memory, cpu, OpDp(cpu, minimum));
    if (!cpu->negative) {
        OpCmp(memory, cpu, OpDp(cpu, maximum));
        if (!cpu->negative)
            return 0;
        OpSta(memory, cpu, OpDp(cpu, minimum));
    } else {
        OpLda(memory, cpu, OpDp(cpu, last));
        OpCmp(memory, cpu, OpDp(cpu, minimum));
        if (cpu->negative)
            return 0;
        OpCmp(memory, cpu, OpDp(cpu, maximum));
        if (cpu->negative)
            OpSta(memory, cpu, OpDp(cpu, maximum));
    }
    return 1;
}

/* Writes the four tile words of the metatile for the cell at X into the top
 * and bottom row buffers at Y; cells with both bits $3000 set take the
 * layer's cell base instead. */
static void RegionWriteMetatile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpWriteX(memory, cpu, OpDp(cpu, REGION_SOURCE_CELL), cpu->x);
    OpLda(memory, cpu, OpAbsX(cpu, 0x0000u));
    OpAndValue(cpu, 0x3000u);
    OpCmpValue(cpu, 0x3000u);
    if (cpu->zero)
        OpLdx(cpu, OpReadX(memory, cpu,
            OpAbs(cpu, WRAM_FIELD_LAYER_CELL_BASE & 0xffffu)));
    OpLda(memory, cpu, OpAbsX(cpu, 0x0000u));
    OpAndValue(cpu, 0x03ffu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAdc(memory, cpu, WRAM_FIELD_METATILE_BASE);
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x0000u));
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, REGION_TOP_ROW));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x0004u));
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, REGION_TOP_ROW));
    OpDey(cpu);
    OpDey(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x0002u));
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, REGION_BOTTOM_ROW));
    OpIny(cpu);
    OpIny(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x0006u));
    OpSta(memory, cpu, DirectLongIndirectY(memory, cpu, REGION_BOTTOM_ROW));
}

/* $83:8E79: renders the pending object region into layers 0 and 2, handing off
 * if a callee leaves an unexpected return frame or after 4096 calls. */
Lufia2ExecutionResult Lufia2FieldRenderLayerPair(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned calls = 0;

    OpLdx(cpu, 0u);
    for (;;) {
        Lufia2ExecutionResult result;
        uint8_t low, high, bank;
        uint16_t frame;

        if (calls == 4096u)
            return ExecutionHandoff(cpu, 0x838e79u);
        ++calls;
        SimulateJslFrame(memory, cpu, 0x83u, 0x8e7cu);
        result = Lufia2FieldRenderRegion(memory, cpu);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        low = Pull8(memory, cpu);
        high = Pull8(memory, cpu);
        bank = Pull8(memory, cpu);
        frame = (uint16_t)(low | ((uint16_t)high << 8));
        cpu->program_bank = bank;
        if (frame != 0x8e7cu || bank != 0x83u)
            return ExecutionHandoff(cpu,
                ((uint32_t)bank << 16) | (uint16_t)(frame + 1u));
        OpInx(cpu);
        OpInx(cpu);
        Compare16(cpu, cpu->x, 4u);
        if (cpu->zero)
            break;
    }
    return ExecutionReturned(0x838e84u);
}

/* Body of $83:8E85: clip the pending object's rectangle to the visible layer
 * cells and write each cell as four tiles; false when a child call or the
 * cell cap leaves the routine through *early without the epilogue. */
static bool RenderRegionCore(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             Lufia2ExecutionResult *early) {
    unsigned axis, cells_drawn = 0;
    Lufia2ExecutionResult child_result;

    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_PENDING_OBJECT_X & 0xffffu));
    OpSta(memory, cpu, OpDp(cpu, REGION_CLIPPED_FIRST_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_FIELD_OBJECT_WIDTH & 0xffffu));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, REGION_CLIPPED_FIRST_X));
    OpSta(memory, cpu, OpDp(cpu, REGION_CLIPPED_LAST_X));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SCROLL_X));
    child_result = CheckedRegionCell(memory, cpu, 0x8ea1u, 1u);
    if (child_result.flow != LUFIA2_EXECUTION_RETURNED) {
        *early = child_result;
        return false;
    }
    OpSta(memory, cpu, OpDp(cpu, REGION_VISIBLE_FIRST_X));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SCROLL_Y));
    child_result = CheckedRegionCell(memory, cpu, 0x8eaau, 0u);
    if (child_result.flow != LUFIA2_EXECUTION_RETURNED) {
        *early = child_result;
        return false;
    }
    OpSta(memory, cpu, OpDp(cpu, REGION_VISIBLE_FIRST_Y));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SCROLL_X));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0100u);
    child_result = CheckedRegionCell(memory, cpu, 0x8eb7u, 1u);
    if (child_result.flow != LUFIA2_EXECUTION_RETURNED) {
        *early = child_result;
        return false;
    }
    OpSta(memory, cpu, OpDp(cpu, REGION_VISIBLE_LAST_X));
    OpLda(memory, cpu, OpLongX(cpu, WRAM_FIELD_LAYER_SCROLL_Y));
    cpu->carry = 0;
    OpAdcValue(cpu, 0x00ffu);
    child_result = CheckedRegionCell(memory, cpu, 0x8ec4u, 0u);
    if (child_result.flow != LUFIA2_EXECUTION_RETURNED) {
        *early = child_result;
        return false;
    }
    OpSta(memory, cpu, OpDp(cpu, REGION_VISIBLE_LAST_Y));
    OpSepWidths(cpu, 0x20u);
    for (axis = 0; axis < 2u; ++axis) {
        if (!RegionClipAxis(memory, cpu,
                axis ? REGION_VISIBLE_FIRST_Y : REGION_VISIBLE_FIRST_X,
                axis ? REGION_VISIBLE_LAST_Y : REGION_VISIBLE_LAST_X,
                (uint8_t)(REGION_CLIPPED_FIRST_X + axis),
                (uint8_t)(REGION_CLIPPED_LAST_X + axis)))
            return true;
    }
    /* Locate the source cells and the two tilemap rows. */
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, REGION_CLIPPED_FIRST_Y));
    OpAndValue(cpu, 0x0fu);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpLsrA(cpu);
    OpSta(memory, cpu, OpDp(cpu, REGION_TILEMAP_ORIGIN));
    OpLda(memory, cpu, OpDp(cpu, REGION_CLIPPED_FIRST_X));
    OpAndValue(cpu, 0x000fu);
    OpAslA(cpu);
    OpAslA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, REGION_TILEMAP_ORIGIN));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpLongX(cpu, 0x838ff0u));
    OpSta(memory, cpu, OpDp(cpu, REGION_TILEMAP_ORIGIN));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_FIELD_LAYER_CELL_BASE & 0xffffu));
    OpSta(memory, cpu, OpDp(cpu, REGION_LAYER_CELL_BASE));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, REGION_TOP_ROW_BANK));
    OpSta(memory, cpu, OpDp(cpu, REGION_BOTTOM_ROW_BANK));
    OpLda(memory, cpu, OpDp(cpu, REGION_CLIPPED_FIRST_X));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, REGION_CLIPPED_FIRST_Y));
    SimulateJsrFrame(memory, cpu, 0x8f31u);
    (void)Lufia2MapCellOffset(memory, cpu);
    {
        uint8_t low = Pull8(memory, cpu);
        uint8_t high = Pull8(memory, cpu);
        uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
        if (actual != 0x8f31u) {
            *early = ExecutionHandoff(cpu, 0x830000u | (uint16_t)(actual + 1u));
            return false;
        }
    }
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, REGION_LAYER_CELL_BASE));
    OpTax(cpu);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, REGION_TILEMAP_ORIGIN)));
    OpLda(memory, cpu, OpDp(cpu, REGION_CLIPPED_LAST_X));
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, REGION_CLIPPED_FIRST_X)));
    OpSta(memory, cpu, OpDp(cpu, REGION_SIZE_DELTA));
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpDp(cpu, REGION_COLUMNS));
    if (cpu->zero)
        return true;
    OpLda(memory, cpu, OpDp(cpu, REGION_HEIGHT_DELTA));
    OpAndValue(cpu, 0x00ffu);
    OpSta(memory, cpu, OpDp(cpu, REGION_ROWS_REMAINING));
    if (cpu->zero)
        return true;
    OpLda(memory, cpu, WRAM_FIELD_SECTION_WIDTH);
    OpAndValue(cpu, 0x00ffu);
    cpu->carry = 1;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, REGION_COLUMNS)));
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, REGION_SOURCE_ROW_SKIP));
    OpTya(cpu);
    OpAndValue(cpu, 0xffc0u);
    OpSta(memory, cpu, OpDp(cpu, REGION_TOP_ROW));
    OpTya(cpu);
    OpAndValue(cpu, 0x003fu);
    OpSta(memory, cpu, OpDp(cpu, REGION_TILEMAP_COLUMN));
    /* Write each cell as four tiles, preserving the tilemap ring wrap. */
    for (;;) {
        OpLda(memory, cpu, OpDp(cpu, REGION_COLUMNS));
        OpSta(memory, cpu, OpDp(cpu, REGION_COLUMNS_REMAINING));
        OpLda(memory, cpu, OpDp(cpu, REGION_TOP_ROW));
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0040u);
        OpSta(memory, cpu, OpDp(cpu, REGION_BOTTOM_ROW));
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, REGION_TILEMAP_COLUMN)));
        for (;;) {
            if (cells_drawn == 262144u) {
                *early = ExecutionHandoff(cpu, 0x838f7fu);
                return false;
            }
            ++cells_drawn;
            RegionWriteMetatile(memory, cpu);
            OpStepMem(memory, cpu, OpDp(cpu, REGION_COLUMNS_REMAINING), -1);
            if (cpu->zero)
                break;
            OpTya(cpu);
            OpIncA(cpu);
            OpIncA(cpu);
            OpAndValue(cpu, 0x003fu);
            OpTay(cpu);
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, REGION_SOURCE_CELL)));
            OpInx(cpu);
            OpInx(cpu);
        }
        OpStepMem(memory, cpu, OpDp(cpu, REGION_ROWS_REMAINING), -1);
        if (cpu->zero)
            break;
        OpLda(memory, cpu, OpDp(cpu, REGION_SOURCE_CELL));
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, REGION_SOURCE_ROW_SKIP));
        OpAdcValue(cpu, 0x0002u);
        OpTax(cpu);
        OpLda(memory, cpu, OpDp(cpu, REGION_TOP_ROW));
        OpTay(cpu);
        OpAdcValue(cpu, 0x0080u);
        OpAndValue(cpu, 0x07ffu);
        OpSta(memory, cpu, OpDp(cpu, REGION_TOP_ROW));
        OpTya(cpu);
        OpAndValue(cpu, 0xf800u);
        OpOra(memory, cpu, OpDp(cpu, REGION_TOP_ROW));
        OpSta(memory, cpu, OpDp(cpu, REGION_TOP_ROW));
    }
    return true;
}

/* $83:8E85: renders the pending object region into layer X, wrapping the core
 * with the saved data bank and index registers. */
Lufia2ExecutionResult Lufia2FieldRenderRegion(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    Lufia2ExecutionResult early;

    PushDataBank(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    OpSetDataBank(memory, cpu, 0x7fu);
    if (!RenderRegionCore(memory, cpu, &early))
        return early;
    OpSepWidths(cpu, 0x20u);
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x838fefu);
}

/* Body of $83:8E85: clip region $7F:D046 to the visible cells of layer X and
 * write each cell as four tiles; returns early where the ROM jumps to its
 * epilogue. */
static void RedrawRegionCore(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned axis;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd046u, 0));
    Write16Direct(memory, cpu, 0x9fu, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd04cu, 0));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x9fu));
    Write16Direct(memory, cpu, 0xa1u, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SCROLL_X, cpu->x)));
    RegionCell(memory, cpu, 0x8ea1u, 1);
    Write16Direct(memory, cpu, DP_PROBE_X, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SCROLL_Y, cpu->x)));
    RegionCell(memory, cpu, 0x8eaau, 0);
    Write16Direct(memory, cpu, DP_PROBE_Y, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SCROLL_X, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0x0100u);
    RegionCell(memory, cpu, 0x8eb7u, 1);
    Write16Direct(memory, cpu, 0x95u, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SCROLL_Y, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0x00ffu);
    RegionCell(memory, cpu, 0x8ec4u, 0);
    Write16Direct(memory, cpu, 0x96u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    /* Clip x ($9F-$A1), then y ($A0-$A2), to the visible cells. */
    for (axis = 0; axis < 2u; ++axis) {
        const uint8_t low = (uint8_t)(0x9fu + axis);
        const uint8_t high = (uint8_t)(0xa1u + axis);

        LoadA8(cpu, DirectByte(memory, cpu, axis ? 0x91u : 0x8fu));
        Compare8(cpu, A8(cpu), DirectByte(memory, cpu, low));
        if (!cpu->negative) {
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, high));
            if (!cpu->negative)
                return;
            Write8(memory, DirectAddress(cpu, low), A8(cpu));
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, axis ? 0x96u : 0x95u));
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, low));
            if (cpu->negative)
                return;
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, high));
            if (cpu->negative)
                Write8(memory, DirectAddress(cpu, high), A8(cpu));
        }
    }
    TransferDirectToA(cpu);                                    /* 8F02 */
    LoadA8(cpu, DirectByte(memory, cpu, 0xa0u));
    And8(cpu, 0x0fu);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    LsrA16(cpu);
    Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x9fu));
    And16(cpu, 0x000fu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(LAYER_BUFFER_TABLE, cpu->x)));
    Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_CELL_BASE & 0xffffu), cpu->x));
    Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7eu);
    Write8(memory, DirectAddress(cpu, 0x62u), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x5fu), A8(cpu));
    LoadA8(cpu, DirectByte(memory, cpu, 0x9fu));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0xa0u));
    SimulateJsrFrame(memory, cpu, 0x8f31u);
    Lufia2MapCellOffset(memory, cpu);                          /* $83:F9F7 */
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* 8F32 */
    LoadA16(cpu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x56u));
    TransferAToX(cpu);
    LoadY16(cpu, Read16Direct(memory, cpu, 0x54u));
    LoadA16(cpu, Read16Direct(memory, cpu, 0xa1u));
    Subtract16(cpu, Read16Direct(memory, cpu, 0x9fu));
    Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
    And16(cpu, 0x00ffu);
    Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
    if (cpu->zero)
        return;
    LoadA16(cpu, Read16Direct(memory, cpu, 0x55u));
    And16(cpu, 0x00ffu);
    Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    if (cpu->zero)
        return;
    LoadA16(cpu, Read16Long(memory, WRAM_FIELD_SECTION_WIDTH));
    And16(cpu, 0x00ffu);
    Subtract16(cpu, Read16Direct(memory, cpu, 0x56u));
    AslA16(cpu);
    Write16Direct(memory, cpu, 0x63u, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0xffc0u);
    Write16Direct(memory, cpu, 0x60u, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0x003fu);
    Write16Direct(memory, cpu, 0x65u, cpu->accumulator);
    for (;;) {
        LoadA16(cpu, Read16Direct(memory, cpu, 0x56u));       /* 8F71 */
        Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x60u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
        LoadY16(cpu, Read16Direct(memory, cpu, 0x65u));
        for (;;) {
            StoreXDirect16(memory, cpu, 0x54u);                /* 8F7F */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
            And16(cpu, 0x3000u);
            Compare16(cpu, cpu->accumulator, 0x3000u);
            if (cpu->zero)
                LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, (WRAM_FIELD_LAYER_CELL_BASE & 0xffffu), 0));
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
            And16(cpu, 0x03ffu);
            AslA16(cpu);
            AslA16(cpu);
            AslA16(cpu);
            Add16Value(cpu, Read16Long(memory, WRAM_FIELD_METATILE_BASE));
            TransferAToX(cpu);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
            Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x60u), cpu->accumulator);
            LoadY16(cpu, (uint16_t)(cpu->y + 2u));
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0004u, cpu->x));
            Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x60u), cpu->accumulator);
            LoadY16(cpu, (uint16_t)(cpu->y - 2u));
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0002u, cpu->x));
            Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x5du), cpu->accumulator);
            LoadY16(cpu, (uint16_t)(cpu->y + 2u));
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0006u, cpu->x));
            Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x5du), cpu->accumulator);
            OpStepMem(memory, cpu, OpDp(cpu, 0x58u), -1);
            if (cpu->zero)
                break;
            LoadA16(cpu, (uint16_t)(cpu->y + 2u));             /* 8FBB */
            And16(cpu, 0x003fu);
            TransferAToY(cpu);
            LoadXDirect(memory, cpu, 0x54u);
            IncrementX16(cpu);
            IncrementX16(cpu);
        }
        OpStepMem(memory, cpu, OpDp(cpu, 0x5au), -1);                 /* 8FC8 */
        if (cpu->zero)
            break;
        LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x63u));
        Add16Value(cpu, 0x0002u);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x60u));
        TransferAToY(cpu);
        Add16Value(cpu, 0x0080u);
        And16(cpu, 0x07ffu);
        Write16Direct(memory, cpu, 0x60u, cpu->accumulator);
        LoadA16(cpu, cpu->y);
        And16(cpu, 0xf800u);
        LoadA16(cpu, (uint16_t)(cpu->accumulator | Read16Direct(memory, cpu, 0x60u)));
        Write16Direct(memory, cpu, 0x60u, cpu->accumulator);
    }
}

/* $83:8E85: redraw region $7F:D046 in layer X; M=1. */
void Lufia2FieldRedrawRegion(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                             uint8_t return_bank, uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    PushDataBank(memory, cpu); /* 8E85 */
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    LoadA8(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    RedrawRegionCore(memory, cpu);
    SetAccumulatorWidth(cpu, 1);                               /* 8FEA */
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* JSL from $8E:BD77 into a $80 tile streamer. */
static void ScrollStream(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint16_t entry) {
    SimulateJslFrame(memory, cpu, 0x8eu, return_address);
    switch (entry) {
    case 0xf4fdu: StreamColumn(memory, cpu, 1); break;
    case 0xf518u: StreamColumn(memory, cpu, 0); break;
    case 0xf589u: StreamRow(memory, cpu, 0); break;
    default: StreamRow(memory, cpu, 1); break;
    }
    SimulateRtlFrame(memory, cpu);
}

/* JSR ($BE6E,x); 0 for an unknown mode. */
static uint8_t ScrollLayerMode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (cpu->x > 8u)
        return 0;
    SimulateJsrFrame(memory, cpu, 0xbdd9u);
    switch (cpu->x) {
    case 0: ScrollModeFollow(memory, cpu); break;
    case 2: break;                                             /* BF24 */
    case 4: ScrollModeParallax(memory, cpu); break;
    case 6: ScrollModeWrap(memory, cpu); break;
    default: ScrollModeScaled(memory, cpu); break;
    }
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* dispatches counts completed BDD7 layer calls. */
static Lufia2ExecutionResult ScrollBoundary(
    Lufia2ExecutionResult result,
    Lufia2CpuState *cpu,
    uint32_t pc) {
    result.flow = LUFIA2_EXECUTION_BOUNDARY;
    result.pc = cpu->resume_pc = pc;
    return result;
}

/* $8E:BD77: per-frame field scroll. Takes the camera scroll from the screen
 * effect override or from the camera actor, then runs each active layer's
 * scroll mode and streams any column or row that came into view. */
Lufia2ExecutionResult Lufia2FieldScrollUpdate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x8ebe6du;
    result.dispatches = 0;
    /* The field loop always calls with X8. */
    if (!cpu->index_is_8_bit)
        return ScrollBoundary(result, cpu, 0x8ebd77u);
    LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);       /* BD77 */
    BitImmediate8(cpu, 0x08u);
    SetAccumulatorWidth(cpu, 0);
    if (!cpu->zero) {
        LoadA16(cpu, Read16Long(memory, 0x7fd08bu));
        Write16Absolute(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_X, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7fd08du));
        Write16Absolute(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_Y, cpu->accumulator);
    } else {
        LoadX8(cpu,
               Read8(memory, AbsoluteIndexedAddress(cpu, WRAM_FIELD_CAMERA_ACTOR, 0)));
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_X, cpu->x)));
        Subtract16(cpu, 0x0080u);
        Write16Absolute(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_X, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(WRAM_ACTOR_FINE_Y, cpu->x)));
        Subtract16(cpu, 0x0070u);
        Write16Absolute(memory, cpu, WRAM_FIELD_CAMERA_SCROLL_Y, cpu->accumulator);
    }
    SetAccumulatorWidth(cpu, 1);                               /* BDA9 */
    LoadX8(cpu, 0x00u);
    do {
        TransferDirectToA(cpu);                                /* BDAD */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SECTION_WORD,
            cpu->x)));
        if (!cpu->negative) {
            SetAccumulatorWidth(cpu, 0);                       /* BDB7 */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_LAYER_SCROLL_X, cpu->x));
            Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
            Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_LAYER_SCROLL_Y, cpu->x));
            Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
            Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            PushIndex(memory, cpu);
            Write8(memory, DirectAddress(cpu, 0x5du), (uint8_t)cpu->x);
            Write8(memory, DirectAddress(cpu, 0x5eu), 0x00u);
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(WRAM_FIELD_LAYER_SECTION_WORD,
                cpu->x)));
            AslA8(cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 0);
            if (!ScrollLayerMode(memory, cpu))
                return ScrollBoundary(result, cpu, 0x8ebdd7u);
            ++result.dispatches;
            LoadXDirect(memory, cpu, STREAM_LAYER); /* BDDA */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_FIELD_LAYER_SCROLL_X, cpu->x));
            And16(cpu, 0x000fu);
            if (cpu->zero) {
                LoadA16(cpu, Read16Direct(memory, cpu, 0x58u));
                Subtract16(cpu, Read16Direct(memory, cpu, 0x54u));
                if (!cpu->zero) {
                    uint8_t right = 1;

                    if (cpu->negative) {
                        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
                        Compare16(cpu, cpu->accumulator, 0x0100u);
                        right = cpu->carry;
                    }
                    if (right)
                        ScrollStream(memory, cpu, 0xbdfeu, 0xf4fdu); /* BDFB */
                    else
                        ScrollStream(memory, cpu, 0xbdf8u, 0xf518u); /* BDF5 */
                }
            }
            LoadXDirect(memory, cpu, STREAM_LAYER); /* BDFF */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x58u));
            Write16Absolute(memory, cpu,
                (uint16_t)(WRAM_FIELD_LAYER_SCROLL_X + cpu->x), cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, 0x0008u);
            PushAccumulator16(memory, cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f4edu, cpu->x)));
            TransferAToY(cpu);
            PullAccumulator16(memory, cpu);
            cpu->carry = 0;
            Add16Value(cpu, Read16Long(memory, 0x7fd081u));
            Write16Absolute(memory, cpu,
                (uint16_t)(0x0594u + cpu->y), cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x5au));    /* BE19 */
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x56u));
            if (!cpu->zero) {
                const uint16_t from = Read16Direct(memory, cpu, 0x56u);

                if (cpu->negative) {
                    LoadA16(cpu, from);                        /* BE21 */
                    LoadA16(cpu, (uint16_t)(from ^ Read16Direct(memory, cpu, 0x5au)));
                    And16(cpu, 0x0010u);
                    if (!cpu->zero)
                        ScrollStream(memory, cpu, 0xbe2du, 0xf589u); /* BE2A */
                } else {
                    uint8_t stream;

                    LoadA16(cpu, from);                        /* BE30 */
                    And16(cpu, 0x000fu);
                    stream = cpu->zero;
                    if (!stream) {
                        LoadA16(cpu, from);
                        LoadA16(cpu, (uint16_t)(from ^ Read16Direct(memory, cpu, 0x5au)));
                        And16(cpu, 0x0010u);
                        if (!cpu->zero) {
                            LoadA16(cpu, from);
                            And16(cpu, 0x0001u);
                            stream = !cpu->zero;
                        }
                    }
                    if (stream)
                        ScrollStream(memory, cpu, 0xbe4au, 0xf5a2u); /* BE47 */
                }
            }
            LoadXDirect(memory, cpu, STREAM_LAYER); /* BE4B */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x5au));
            Write16Absolute(memory, cpu,
                (uint16_t)(WRAM_FIELD_LAYER_SCROLL_Y + cpu->x), cpu->accumulator);
            PushAccumulator16(memory, cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f4f5u, cpu->x)));
            TransferAToY(cpu);
            PullAccumulator16(memory, cpu);
            cpu->carry = 0;
            Add16Value(cpu, Read16Long(memory, 0x7fd083u));
            Write16Absolute(memory, cpu,
                (uint16_t)(0x0596u + cpu->y), cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            cpu->x = PullIndexValue(memory, cpu);              /* BE63 */
        }
        LoadX8(cpu, (uint8_t)(cpu->x + 2u));                   /* BE64 */
        Compare8(cpu, (uint8_t)cpu->x, 0x06u);
    } while (!cpu->zero);
    return result;
}
