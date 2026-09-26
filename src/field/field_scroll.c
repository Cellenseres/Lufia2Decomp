/* Field BG scrolling and tile streaming ($8E:BD77). */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "actor/actor_internal.h"
#include "field/field_internal.h"
#include "system/wram.h"

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
        LoadXDirect(memory, cpu, 0x5du);                       /* BE80 */
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
    LoadXDirect(memory, cpu, 0x5du);                           /* BEC4 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a8u, 0));
    if (cpu->zero) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0));
        Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
        Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    } else {
        const uint32_t speed = AbsoluteIndexedAddress(cpu, 0x05a8u, 0);

        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0));
        Write16Direct(memory, cpu, 0x58u, cpu->accumulator);   /* BED7 */
        Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
        if (!cpu->zero) {
            ScrollStepToward(memory, cpu, 0x54u, 0x58u, speed);
        } else {
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
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
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0)); /* BF12 */
    Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x54u));
    if (!cpu->zero)
        return;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
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
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd021u, cpu->x)));
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
    LoadXDirect(memory, cpu, 0x5du);                           /* BF25 */
    ScrollLayerNibble(memory, cpu, 1);
    ScrollShiftCount(memory, cpu, 0xbf34u);
    if (!cpu->zero) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0));
        ScrollShiftRight(memory, cpu, 0xbf3cu);
        if (cpu->carry)
            Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
        else
            ScrollOffsetTarget(memory, cpu, 0x54u, 0x58u);
    }
    LoadXDirect(memory, cpu, 0x5du);                           /* BF4D */
    ScrollLayerNibble(memory, cpu, 0);
    ScrollShiftCount(memory, cpu, 0xbf58u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
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
    LoadXDirect(memory, cpu, 0x5du);                           /* BF9B */
    ScrollLayerNibble(memory, cpu, 1);
    ScrollShiftCount(memory, cpu, 0xbfaau);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0));
    ScrollShiftLeft(memory, cpu, 0xbfb0u);
    ScrollOffsetTarget(memory, cpu, 0x54u, 0x58u);
    /* X still holds the high nibble. */
    ScrollLayerNibble(memory, cpu, 0);                         /* BFBF */
    ScrollShiftCount(memory, cpu, 0xbfc8u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
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
    LoadXDirect(memory, cpu, 0x5du);
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
    LoadXDirect(memory, cpu, 0x5du);                           /* BFEE */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd021u, cpu->x)));
    And16(cpu, 0x00f0u);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    ScrollWrapAxis(memory, cpu, 0x54u, 0x58u, 0x7fd010u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd021u, cpu->x)));
    And16(cpu, 0x000fu);                                       /* C01F */
    AslA16(cpu);
    ScrollWrapAxis(memory, cpu, 0x56u, 0x5au, 0x7fd018u);
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
    Write16Direct(memory, cpu, 0x30u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x22u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd010u, cpu->x));
    Write16Direct(memory, cpu, 0x83u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd018u, cpu->x));
    Write16Direct(memory, cpu, 0x85u, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x22u));
    StreamWrap(memory, cpu, 0x83u, 0xf762u, 0xf77au, 0);
    Write16Direct(memory, cpu, 0x87u, cpu->accumulator);       /* F77F */
    Write16Direct(memory, cpu, 0x26u, cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x30u));
    AslA16(cpu);
    AslA16(cpu);
    And16(cpu, 0x003eu);
    Write16Direct(memory, cpu, 0x22u, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    Write16Direct(memory, cpu, 0x28u, cpu->accumulator);
    StreamWrap(memory, cpu, 0x85u, 0xf7adu, 0xf7c9u, 1);
    Write16Direct(memory, cpu, 0x89u, cpu->accumulator);       /* F7CE */
    Write16Direct(memory, cpu, 0x28u, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0x00f0u);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Write16Direct(memory, cpu, 0x24u, cpu->accumulator);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x22u));
    Write16Direct(memory, cpu, 0x2du, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x838ff0u, cpu->x)));
    Write16Direct(memory, cpu, 0x2au, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x28u)));
    Write8(memory, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x83u)));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    LoadA8(cpu, 0x7eu);
    Write8(memory, DirectAddress(cpu, 0x2cu), A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd008u, cpu->x));
    Write16Direct(memory, cpu, 0x8du, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, SNES_RDMPYL));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x26u));
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x8du));
    Write16Direct(memory, cpu, 0x30u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd03cu, 0));
    Write16Direct(memory, cpu, 0x65u, cpu->accumulator);
    Compare16(cpu, cpu->x, 0x0004u);
    if (cpu->carry) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd040u, 0));
        Write16Direct(memory, cpu, 0x65u, cpu->accumulator);
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
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x89u)));
    Write8(memory, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0x83u)));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x87u));
    cpu->carry = 0;
    Add16Value(cpu, Read16Long(memory, SNES_RDMPYL));
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x8du));
    TransferAToX(cpu);
    SimulateRtsFrame(memory, cpu);
}

/* One 16x16 metatile from cell X into [$2A],y. */
static void StreamMetatile(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    And16(cpu, 0x3000u);
    Compare16(cpu, cpu->accumulator, 0x3000u);
    if (cpu->zero)
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd008u, 0));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    And16(cpu, 0x03ffu);
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x65u));
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au), cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0004u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au), cpu->accumulator);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, 0x003eu);
    TransferAToY(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0002u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au), cpu->accumulator);
    IncrementY16(cpu);
    IncrementY16(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0006u, cpu->x));
    Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au), cpu->accumulator);
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
    LoadA16(cpu, 0x0010u);                                     /* F5ED */
    Write16Direct(memory, cpu, 0x28u, cpu->accumulator);
    do {
        PushIndex(memory, cpu);                                /* F5F2 */
        StreamMetatile(memory, cpu);
        LoadA16(cpu, cpu->y);                                  /* F62B */
        cpu->carry = 0;
        Add16Value(cpu, 0x003eu);
        And16(cpu, 0x07ffu);
        TransferAToY(cpu);
        PullAccumulator16(memory, cpu);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x8bu));
        TransferAToX(cpu);
        StreamStep(memory, cpu, 0x89u, 0x85u, 0xf648u);
        Decrement16Direct(memory, cpu, 0x28u);                 /* F649 */
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* $80:F64E: A metatiles along a row. */
static void StreamRowTiles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Write16Direct(memory, cpu, 0x26u, cpu->accumulator);       /* F64E */
    do {
        PushIndex(memory, cpu);                                /* F650 */
        StreamMetatile(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);                  /* F689 */
        IncrementX16(cpu);
        IncrementX16(cpu);
        LoadA16(cpu, cpu->y);
        Subtract16(cpu, 0x003eu);
        And16(cpu, 0x07ffu);
        TransferAToY(cpu);
        StreamStep(memory, cpu, 0x87u, 0x83u, 0xf6a4u);
        Decrement16Direct(memory, cpu, 0x26u);                 /* F6A5 */
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
    LoadXDirect(memory, cpu, 0x5du);
}

/* $80:F4FD/F518: stream a column at the right/left edge. */
static void StreamColumn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t right) {
    StreamEnter(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x001226u, cpu->x)));
    TransferAToY(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x00121eu, cpu->x)));
    if (right) {
        cpu->carry = 0;
        Add16Value(cpu, 0x0100u);
    }
    StreamLocate(memory, cpu, 0xf52fu);                        /* F52D */
    PushIndex(memory, cpu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x83u));
    AslA16(cpu);
    Write16Direct(memory, cpu, 0x8bu, cpu->accumulator);
    LoadXDirect(memory, cpu, 0x30u);
    LoadYDirect16(memory, cpu, 0x2du);
    StreamColumnTiles(memory, cpu, 0xf53cu);
    cpu->x = PullIndexValue(memory, cpu);                      /* F53D */
    LoadA16(cpu, Read16Direct(memory, cpu, 0x22u));
    TransferAToY(cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x2au));
    Write16Direct(memory, cpu, 0x2du, cpu->accumulator);
    StoreXDirect16(memory, cpu, 0x30u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f581u, cpu->x)));
    Write16Long(memory, LongIndexedAddress(0x001236u, cpu->x), cpu->accumulator);
    TransferAToX(cpu);
    LoadA16(cpu, 0x0020u);
    Write16Direct(memory, cpu, 0x22u, cpu->accumulator);
    do {
        LoadA16(cpu, Read16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au)));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x),
            cpu->accumulator);
        IncrementY16(cpu);
        IncrementY16(cpu);
        LoadA16(cpu, Read16Long(memory, DirectLongIndirectY(memory, cpu, 0x2au)));
        Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0040u, cpu->x),
            cpu->accumulator);
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, 0x003eu);
        TransferAToY(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Decrement16Direct(memory, cpu, 0x22u);
    } while (!cpu->zero);
    LoadXDirect(memory, cpu, 0x30u);                           /* F56E */
    LoadA16(cpu, Read16Direct(memory, cpu, 0x2du));
    Write16Long(memory, LongIndexedAddress(0x001246u, cpu->x), cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f581u, cpu->x)));
    Write16Long(memory, LongIndexedAddress(0x001236u, cpu->x), cpu->accumulator);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $80:F589/F5A2: stream a row at the top/bottom edge. */
static void StreamRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t down) {
    StreamEnter(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x001226u, cpu->x)));
    if (down) {
        cpu->carry = 0;
        Add16Value(cpu, 0x00f0u);
    } else {
        Subtract16(cpu, 0x0010u);
    }
    And16(cpu, 0xfff0u);
    TransferAToY(cpu);                                         /* F5B9 */
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x00121eu, cpu->x)));
    StreamLocate(memory, cpu, 0xf5c0u);
    LoadA16(cpu, 0x0040u);                                     /* F5C1 */
    Subtract16(cpu, Read16Direct(memory, cpu, 0x22u));
    LsrA16(cpu);
    LsrA16(cpu);
    LoadXDirect(memory, cpu, 0x30u);
    LoadYDirect16(memory, cpu, 0x2du);
    StreamRowTiles(memory, cpu, 0xf5cfu);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x22u));            /* F5D0 */
    LsrA16(cpu);
    LsrA16(cpu);
    if (!cpu->zero) {
        /* X continues from the first run. */
        LoadYDirect16(memory, cpu, 0x24u);
        StreamRowTiles(memory, cpu, 0xf5dau);
    }
    LoadA16(cpu, Read16Direct(memory, cpu, 0x24u));            /* F5DB */
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x2au));
    LoadXDirect(memory, cpu, 0x5du);
    Write16Long(memory, LongIndexedAddress(0x00122eu, cpu->x), cpu->accumulator);
    Write16Long(memory, LongIndexedAddress(0x00123eu, cpu->x), cpu->accumulator);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $80:F6C6: 16 metatiles from cell X into the row buffers [$5D]
   and [$60] at Y, wrapping in 64 bytes. */
static void StreamRowBuffers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadA16(cpu, 0x0010u);                                     /* F6C6 */
    Write16Direct(memory, cpu, 0x26u, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0xffc0u);
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x2au));
    Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
    cpu->carry = 0;
    Add16Value(cpu, 0x0040u);
    Write16Direct(memory, cpu, 0x60u, cpu->accumulator);
    LoadA16(cpu, cpu->y);
    And16(cpu, 0x003fu);
    TransferAToY(cpu);
    do {
        PushIndex(memory, cpu);                                /* F6DF */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
        And16(cpu, 0x3000u);
        Compare16(cpu, cpu->accumulator, 0x3000u);
        if (cpu->zero)
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd008u, 0));
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
        And16(cpu, 0x03ffu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, 0x65u));
        TransferAToX(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
        Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x5du), cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0002u, cpu->x));
        Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x60u), cpu->accumulator);
        LoadA16(cpu, (uint16_t)(cpu->y + 2u));
        And16(cpu, 0x003fu);
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0004u, cpu->x));
        Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x5du), cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0006u, cpu->x));
        Write16Long(memory, DirectLongIndirectY(memory, cpu, 0x60u), cpu->accumulator);
        cpu->x = PullIndexValue(memory, cpu);                  /* F715 */
        LoadA16(cpu, (uint16_t)(cpu->y + 2u));
        And16(cpu, 0x003fu);
        TransferAToY(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, 0x87u) + 1u));
        Write16Direct(memory, cpu, 0x87u, cpu->accumulator);
        if (cpu->zero) {
            StreamCellIndex(memory, cpu, 0xf72eu);
        } else {
            Compare16(cpu, cpu->accumulator, Read16Direct(memory, cpu, 0x83u));
            if (cpu->carry) {
                Write16Direct(memory, cpu, 0x87u, 0x0000u);
                StreamCellIndex(memory, cpu, 0xf72eu);         /* F72C */
            }
        }
        Decrement16Direct(memory, cpu, 0x26u);                 /* F72F */
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* $80:F47A: redraw layer X in 16 rows unless $7F:D020,x is $FF;
   registers and flags are restored. */
static void RedrawLayer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x83u, 0x8e6fu);
    PushAccumulator8(memory, cpu);                             /* F47A */
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd020u, cpu->x)));
    Compare8(cpu, A8(cpu), 0xffu);
    if (!cpu->zero) {
        SetAccumulatorWidth(cpu, 0);                           /* F487 */
        SetIndexWidth(cpu, 0);
        StoreXDirect16(memory, cpu, 0x15u);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f4edu, cpu->x)));
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x121eu, cpu->x));
        cpu->carry = 0;
        Add16Value(cpu, 0x0008u);
        Write16Absolute(memory, cpu, (uint16_t)(0x0594u + cpu->y), cpu->accumulator);
        And16(cpu, 0xfff0u);
        Write16Direct(memory, cpu, 0x11u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80f4f5u, cpu->x)));
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1226u, cpu->x));
        Write16Absolute(memory, cpu, (uint16_t)(0x0596u + cpu->y), cpu->accumulator);
        And16(cpu, 0xfff0u);
        Write16Direct(memory, cpu, 0x13u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);                           /* F4AF */
        LoadA8(cpu, 0x7eu);
        Write8(memory, DirectAddress(cpu, 0x5fu), A8(cpu));
        Write8(memory, DirectAddress(cpu, 0x62u), A8(cpu));
        LoadA8(cpu, 0x7fu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd010u, cpu->x));
        AslA16(cpu);
        Subtract16(cpu, 0x0020u);
        Write16Direct(memory, cpu, 0x28u, cpu->accumulator);
        LoadA16(cpu, 0x0010u);
        Write16Direct(memory, cpu, 0x17u, cpu->accumulator);
        do {
            LoadA16(cpu, Read16Direct(memory, cpu, 0x13u));    /* F4CC */
            TransferAToY(cpu);
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            Write16Direct(memory, cpu, 0x13u, cpu->accumulator);
            LoadXDirect(memory, cpu, 0x15u);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x11u));
            StreamLocate(memory, cpu, 0xf4dbu);
            LoadXDirect(memory, cpu, 0x30u);
            LoadY16(cpu, Read16Direct(memory, cpu, 0x2du));
            StreamRowBuffers(memory, cpu, 0xf4e2u);
            Decrement16Direct(memory, cpu, 0x17u);
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
    SimulateRtlFrame(memory, cpu);
}

/* $83:8E66: redraw layers 3 to 0; X ends at $FFFE. */
void Lufia2FieldRedrawLayers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* 8E66 */
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, 0x0006u);
    do {
        RedrawLayer(memory, cpu);
        LoadX16(cpu, (uint16_t)(cpu->x - 2u));                 /* 8E70 */
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $83:9000 (rounding up) / $83:9004: A / 16, negative as D. */
static void RegionCell(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address,
    uint8_t round_up) {
    SimulateJsrFrame(memory, cpu, return_address);
    if (round_up) {
        cpu->carry = 0;                                        /* 9000 */
        Add16Value(cpu, 0x000fu);
    }
    if (cpu->negative)                                         /* 9004 */
        TransferDirectToA(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    SimulateRtsFrame(memory, cpu);
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

/* Read-only: the cells $83:8E85 draws for layer X; ~0 when X is no
   layer, whose buffer could cover the direct page. */
uint32_t Lufia2FieldRegionCells(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu) {
    const uint16_t origin = Read16Long(memory, 0x7fd046u);
    const uint16_t end = (uint16_t)(Read16Long(memory, 0x7fd04cu) + origin);
    uint8_t low[2], high[2], first[2], last[2];
    uint16_t size;
    unsigned axis;

    if (cpu->x > 6u)
        return 0xffffffffu;
    {
        const uint16_t x = Read16Long(memory, 0x00121eu + cpu->x);
        const uint16_t y = Read16Long(memory, 0x001226u + cpu->x);

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

/* $83:8E85: redraw the cells of region $7F:D046 (x, y) + $D04C
   (w, h) that are visible in layer X into its row buffers; M=1. */
void Lufia2FieldRedrawRegion(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    unsigned axis;

    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    PushDataBank(memory, cpu);                                 /* 8E85 */
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    LoadA8(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd046u, 0));
    Write16Direct(memory, cpu, 0x9fu, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd04cu, 0));
    cpu->carry = 0;
    Add16Value(cpu, Read16Direct(memory, cpu, 0x9fu));
    Write16Direct(memory, cpu, 0xa1u, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x00121eu, cpu->x)));
    RegionCell(memory, cpu, 0x8ea1u, 1);
    Write16Direct(memory, cpu, 0x8fu, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x001226u, cpu->x)));
    RegionCell(memory, cpu, 0x8eaau, 0);
    Write16Direct(memory, cpu, 0x91u, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x00121eu, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0x0100u);
    RegionCell(memory, cpu, 0x8eb7u, 1);
    Write16Direct(memory, cpu, 0x95u, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x001226u, cpu->x)));
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
                goto done;
            Write8(memory, DirectAddress(cpu, low), A8(cpu));
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, axis ? 0x96u : 0x95u));
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, low));
            if (cpu->negative)
                goto done;
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
    Add16Value(cpu, Read16Long(memory, LongIndexedAddress(0x838ff0u, cpu->x)));
    Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd008u, cpu->x));
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
        goto done;
    LoadA16(cpu, Read16Direct(memory, cpu, 0x55u));
    And16(cpu, 0x00ffu);
    Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
    if (cpu->zero)
        goto done;
    LoadA16(cpu, Read16Long(memory, 0x0005b9u));
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
                LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xd008u, 0));
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->x));
            And16(cpu, 0x03ffu);
            AslA16(cpu);
            AslA16(cpu);
            AslA16(cpu);
            Add16Value(cpu, Read16Long(memory, 0x7fd03cu));
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
            Decrement16Direct(memory, cpu, 0x58u);
            if (cpu->zero)
                break;
            LoadA16(cpu, (uint16_t)(cpu->y + 2u));             /* 8FBB */
            And16(cpu, 0x003fu);
            TransferAToY(cpu);
            LoadXDirect(memory, cpu, 0x54u);
            IncrementX16(cpu);
            IncrementX16(cpu);
        }
        Decrement16Direct(memory, cpu, 0x5au);                 /* 8FC8 */
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
done:
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
        Write16Absolute(memory, cpu, 0x05a4u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7fd08du));
        Write16Absolute(memory, cpu, 0x05a6u, cpu->accumulator);
    } else {
        LoadX8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0734u, 0)));
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
        Subtract16(cpu, 0x0080u);
        Write16Absolute(memory, cpu, 0x05a4u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
        Subtract16(cpu, 0x0070u);
        Write16Absolute(memory, cpu, 0x05a6u, cpu->accumulator);
    }
    SetAccumulatorWidth(cpu, 1);                               /* BDA9 */
    LoadX8(cpu, 0x00u);
    do {
        TransferDirectToA(cpu);                                /* BDAD */
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd020u, cpu->x)));
        if (!cpu->negative) {
            SetAccumulatorWidth(cpu, 0);                       /* BDB7 */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x121eu, cpu->x));
            Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
            Write16Direct(memory, cpu, 0x58u, cpu->accumulator);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1226u, cpu->x));
            Write16Direct(memory, cpu, 0x56u, cpu->accumulator);
            Write16Direct(memory, cpu, 0x5au, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            PushIndex(memory, cpu);
            Write8(memory, DirectAddress(cpu, 0x5du), (uint8_t)cpu->x);
            Write8(memory, DirectAddress(cpu, 0x5eu), 0x00u);
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fd020u, cpu->x)));
            AslA8(cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 0);
            if (!ScrollLayerMode(memory, cpu))
                return ScrollBoundary(result, cpu, 0x8ebdd7u);
            ++result.dispatches;
            LoadXDirect(memory, cpu, 0x5du);                   /* BDDA */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x121eu, cpu->x));
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
            LoadXDirect(memory, cpu, 0x5du);                   /* BDFF */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x58u));
            Write16Absolute(memory, cpu,
                (uint16_t)(0x121eu + cpu->x), cpu->accumulator);
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
            LoadXDirect(memory, cpu, 0x5du);                   /* BE4B */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x5au));
            Write16Absolute(memory, cpu,
                (uint16_t)(0x1226u + cpu->x), cpu->accumulator);
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
