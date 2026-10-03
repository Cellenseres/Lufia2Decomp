/* World map ground plane: the Mode 7 matrix tables of the perspective view. */

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/world_map.h"
#include "system/wram.h"

/* Direct-page work bytes of the table setup. */
enum {
    WORK_VALUE = 0x00u,    /* product, dividend and quotient: 32 bits */
    WORK_QUOTIENT_BYTE = 0x01u,
    WORK_HIGH_WORD = 0x02u,
    WORK_HIGH_BYTE = 0x03u,
    WORK_OPERAND = 0x04u,  /* multiplier or divisor: 16 bits */
    WORK_OPERAND_HIGH = 0x05u,
    WORK_LIMIT = 0x06u,
    ROW_POSITION = 0x22u,  /* 16.16 position of the current scanline */
    ROW_WHOLE = 0x24u,
    ROWS_LEFT = 0x26u,
    ROW_SCALE = 0x4eu,     /* reciprocal of the row depth */
    ROW_SCALE_HIGH = 0x4fu,
    BAND_QUADRANT = 0x56u, /* 0, 2, 4 or 6 selects the sign pattern */
    BAND_QUADRANT_HIGH = 0x57u,
    COSINE_TERM = 0x58u,   /* magnitude byte, then a sign byte */
    COSINE_SIGN = 0x59u,
    SINE_TERM = 0x5au,
    SINE_SIGN = 0x5bu
};

/* Absolute work-RAM fields, reached through the data bank. */
enum {
    PLANE_REFRESH = 0x11dau,       /* cleared once the tables are built */
    PLANE_MODE = 0x11ddu,          /* non-zero: steps follow the tilt table */
    PLANE_START_ROW = 0x11fcu,
    PLANE_TILT = 0x11ffu,
    /* Two HDMA tables of 2 * (1 + 4 * 112) + 1 bytes: Mode 7 A and B in the
     * first, C and D in the second. Each scanline holds one word per
     * register. */
    MATRIX_AB_TABLE = 0x1718u,
    MATRIX_CD_TABLE = 0x1a9bu,
    MATRIX_B_OFFSET = 0x0002u,
    MATRIX_C_OFFSET = 0x0000u,
    MATRIX_D_OFFSET = 0x0002u,
    TABLE_END = 0x0382u,
    BAND_ROWS = 0x0070u,
    BAND_HEADER = 0x80u,           /* HDMA repeat count flag */
    ROW_BYTES = 4u
};

/* Quarter-wave sine table (0..$100 in 1/64 quarter steps) in bank $97 and the
 * reciprocal table that scales a row by its distance, in bank $86. */
#define SINE_QUARTER_TABLE 0x97b226u
#define RECIPROCAL_TABLE 0xd3b7u

/* Quadrant bits of the matrix: the cosine sign flips A and D, the sine sign
 * swaps the signs of B and C and the order in which they are stored. */
enum {
    QUADRANT_NEGATE_COSINE = 1u,
    QUADRANT_SWAP_SINE = 2u
};

static uint8_t SineTableByte(
    const Lufia2Memory *memory, uint16_t index) {
    return Read8(memory, LongIndexedAddress(SINE_QUARTER_TABLE, index));
}

static uint16_t SineTableWord(
    const Lufia2Memory *memory, uint16_t index) {
    return Read16Long(memory, LongIndexedAddress(SINE_QUARTER_TABLE, index));
}

/* The two angle terms are a magnitude byte and a sign byte at $58 and $5A;
 * the value 1.0 is the pair 0, 1. They are built in $00/$01. The routines
 * leave their flags and stack residue to the caller, and the pushed status
 * byte shows them, so the flags are kept as the original left them. */

/* An angle folded into the first quarter of the wave. */
static uint8_t FoldIntoQuarter(uint8_t angle) {
    return (angle & 0x40u) != 0 ? (uint8_t)(0x40u - (angle & 0x3fu)) : angle;
}

/* $86:A4FA: cosine term of angle A into $00. Quarters 1 and 2 are negative.
 * The carry is left set exactly for the last quarter, the overflow clear. */
static void CosineTerm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint8_t angle) {
    const uint16_t steps = FoldIntoQuarter(angle) & 0x7fu;
    const uint16_t magnitude =
        SineTableWord(memory, (uint16_t)((0x40u - steps) << 1));
    const uint8_t negative_high = (uint8_t)((uint16_t)(0u - magnitude) >> 8);
    const uint8_t quarter = angle & 0xc0u;

    Push8(memory, cpu, angle);
    WramWrite16(wram, WORK_VALUE, magnitude);
    (void)Pull8(memory, cpu);
    cpu->carry = quarter == 0xc0u;
    cpu->overflow = false;
    if (quarter != 0x00u && quarter != 0xc0u)
        WramWrite(wram, WORK_QUOTIENT_BYTE, negative_high);
}

/* $86:A4D3: sine term of angle A into $00. Angles from 128 on are negative.
 * The flags come back as they were, except the zero flag. */
static void SineTerm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint8_t angle) {
    const uint16_t steps = FoldIntoQuarter(angle) & 0x7fu;
    const uint16_t magnitude = SineTableWord(memory, (uint16_t)(steps << 1));
    const bool negative = (angle & 0x80u) != 0;

    cpu->zero = !negative;
    Push8(memory, cpu, PackStatus(cpu));
    WramWrite16(wram, WORK_VALUE, magnitude);
    (void)Pull8(memory, cpu);
    if (negative)
        WramWrite(wram, WORK_QUOTIENT_BYTE,
            (uint8_t)((uint16_t)(0u - magnitude) >> 8));
}

/* $86:A52F: 32-bit product of the words at $00 and $04 into $00. */
static void MultiplyWords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    uint16_t sum;
    uint8_t high;

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    WramWrite(wram, SNES_WRMPYA, WramRead(wram, WORK_VALUE));
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, WORK_OPERAND));
    Push8(memory, cpu, WramRead(wram, WORK_QUOTIENT_BYTE));
    WramWrite16(wram, WORK_VALUE, WramRead16(wram, SNES_RDMPYL));
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, WORK_OPERAND_HIGH));
    WramWrite16(wram, WORK_HIGH_WORD, 0);
    sum = WramRead16(wram, WORK_QUOTIENT_BYTE);
    sum = (uint16_t)(sum + WramRead16(wram, SNES_RDMPYL));
    WramWrite16(wram, WORK_QUOTIENT_BYTE, sum);
    WramWrite(wram, SNES_WRMPYA, Pull8(memory, cpu));
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, WORK_OPERAND));
    {
        const uint16_t before = WramRead16(wram, WORK_QUOTIENT_BYTE);
        const uint32_t total = (uint32_t)before + WramRead16(wram, SNES_RDMPYL);

        WramWrite16(wram, WORK_QUOTIENT_BYTE, (uint16_t)total);
        high = (uint8_t)((WramRead(wram, WORK_HIGH_BYTE) << 1) | (total >> 16));
        WramWrite(wram, WORK_HIGH_BYTE, high);
    }
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, WORK_OPERAND_HIGH));
    {
        const uint16_t before = WramRead16(wram, WORK_HIGH_WORD);

        sum = (uint16_t)(before + WramRead16(wram, SNES_RDMPYL));
        WramWrite16(wram, WORK_HIGH_WORD, sum);
    }
    cpu->accumulator = sum;
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $86:A5A9 on the work words: the 32-bit value at $00 divided by the word
 * at $04; the shared division routine of the world map. */
static void DivideWork(Lufia2Wram wram, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(wram.memory, cpu, return_address);
    (void)Lufia2WorldMapDivide32(wram.memory, cpu);
    SimulateRtsFrame(wram.memory, cpu);
}

/* $86:A913: row step from the tilt table, for a non-zero mode. Tilts of 64
 * and up leave the step at zero. */
static void RowStepFromTiltTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint16_t tilt = WramRead16(wram, PLANE_TILT) & 0x00ffu;
    uint16_t sine;
    uint16_t start_row;
    Word16Result remaining;
    uint16_t divisor;

    WramWrite16(wram, WORK_VALUE, 0);
    WramWrite16(wram, WORK_HIGH_WORD, 0);
    if (tilt >= 0x0040u)
        return;
    sine = SineTableWord(memory, (uint16_t)(tilt << 1));
    PushStackWord(memory, cpu, sine);
    WramWrite16(wram, WORK_OPERAND, sine);
    start_row = WramRead16(wram, PLANE_START_ROW);
    WramWrite16(wram, WORK_VALUE, start_row);
    cpu->carry = false;
    SetNz16(cpu, start_row);
    SimulateJsrFrame(memory, cpu, 0xa932u);
    MultiplyWords(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
    sine = PullStackWord(memory, cpu);
    WramWrite(wram, SNES_WRMPYA, (uint8_t)sine);
    WramWrite(wram, SNES_WRMPYB, 0xe0u);
    remaining = Difference16(WramRead16(wram, PLANE_START_ROW),
        WramRead16(wram, WORK_QUOTIENT_BYTE));
    WramWrite16(wram, WORK_HIGH_WORD, remaining.value);
    WramWrite16(wram, WORK_VALUE, 0);
    divisor = WramRead16(wram, SNES_RDMPYH) & 0x00ffu;
    WramWrite16(wram, WORK_OPERAND, divisor);
    cpu->carry = remaining.carry;
    cpu->overflow = remaining.overflow;
    SetNz16(cpu, divisor);
    DivideWork(wram, cpu, 0xa954u);
}

/* $86:A956: row step from the difference of two sine entries. */
static void RowStepFromSineDifference(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint8_t tilt = WramRead(wram, PLANE_TILT);
    const uint16_t start_row = cpu->x;
    uint8_t complement_sine;
    Byte8Result difference;
    uint16_t quotient;
    Word16Result remaining;

    WramWrite16(wram, WORK_HIGH_WORD, 0);
    WramWrite16(wram, WORK_VALUE, 0);
    SetAccumulatorWidth(cpu, 1);
    if (tilt >= 0x40u)
        return;
    WramWrite16(wram, WORK_HIGH_WORD, start_row);
    complement_sine = SineTableByte(memory, (uint16_t)((0x40u - tilt) << 1));
    WramWrite(wram, WORK_LIMIT, complement_sine);
    difference = Difference8(SineTableByte(memory, (uint16_t)(tilt << 1)),
        WramRead(wram, WORK_LIMIT));
    WramWrite(wram, WORK_OPERAND, difference.value);
    WramWrite(wram, WORK_OPERAND_HIGH, 0);
    cpu->carry = difference.carry;
    cpu->overflow = difference.overflow;
    SetNz8(cpu, difference.value);
    DivideWork(wram, cpu, 0xa985u);
    quotient = WramRead16(wram, WORK_QUOTIENT_BYTE);
    WramWrite16(wram, WORK_OPERAND, quotient);
    WramWrite16(wram, WORK_VALUE, 0);
    WramWrite16(wram, WORK_HIGH_WORD, 1u);
    cpu->zero = false;
    cpu->negative = false;
    DivideWork(wram, cpu, 0xa996u);
    SetAccumulatorWidth(cpu, 0);
    remaining = Difference16(WramRead16(wram, PLANE_START_ROW),
        WramRead16(wram, WORK_VALUE));
    WramWrite16(wram, WORK_QUOTIENT_BYTE, remaining.value);
    SetAccumulatorWidth(cpu, 1);
    WramWrite(wram, WORK_VALUE, 0);
    WramWrite(wram, WORK_HIGH_BYTE, 0);
}

/* Flags a band leaves behind: the borrow of its last row step. */
typedef struct BandExit {
    uint8_t carry;
    uint8_t overflow;
    uint16_t table_index;
    uint16_t last_index;
} BandExit;

/* A row's scale for one matrix term: (term * 1/depth) in 8.8 fixed point,
 * through the hardware multiplier (low and high byte of the reciprocal). The
 * accesses keep the order of the unit: both operands, then the product. */
static uint16_t ScaledTerm(Lufia2Wram wram, uint32_t term) {
    uint16_t carried;
    uint8_t scale_high;

    WramWrite(wram, SNES_WRMPYA, WramRead(wram, term));
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, ROW_SCALE));
    carried = WramRead16(wram, SNES_RDMPYH);
    scale_high = WramRead(wram, ROW_SCALE_HIGH);
    WramWrite(wram, SNES_WRMPYB, scale_high);
    return (uint16_t)((carried & 0x00ffu) + WramRead16(wram, SNES_RDMPYL));
}

/* The second term reads the reciprocal's high byte before the product. */
static uint16_t ScaledSecondTerm(
    Lufia2Wram wram, uint32_t term, uint16_t *carried_out) {
    uint16_t carried;
    uint8_t scale_high;

    WramWrite(wram, SNES_WRMPYA, WramRead(wram, term));
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, ROW_SCALE));
    scale_high = WramRead(wram, ROW_SCALE_HIGH);
    carried = WramRead16(wram, SNES_RDMPYH);
    *carried_out = carried;
    WramWrite(wram, SNES_WRMPYB, scale_high);
    return (uint16_t)((carried & 0x00ffu) + WramRead16(wram, SNES_RDMPYL));
}

static uint16_t Negate16(uint16_t value) {
    return (uint16_t)(0u - value);
}

/* Cosine into A and D. */
static void StoreCosine(Lufia2Wram wram, uint16_t y, uint16_t value) {
    WramWrite16At(wram, MATRIX_AB_TABLE, y, value);
    WramWrite16At(wram, MATRIX_CD_TABLE + MATRIX_D_OFFSET, y, value);
}

/* The sine goes to B and its negation to C, or the other way round. */
static void StoreSine(
    Lufia2Wram wram, uint16_t y, uint16_t value, unsigned quadrant) {
    const uint16_t negated = Negate16(value);

    if (quadrant & QUADRANT_SWAP_SINE) {
        WramWrite16At(wram, MATRIX_CD_TABLE + MATRIX_C_OFFSET, y, value);
        WramWrite16At(wram, MATRIX_AB_TABLE + MATRIX_B_OFFSET, y, negated);
    } else {
        WramWrite16At(wram, MATRIX_AB_TABLE + MATRIX_B_OFFSET, y, value);
        WramWrite16At(wram, MATRIX_CD_TABLE + MATRIX_C_OFFSET, y, negated);
    }
}

/* Moves the 16.16 row position down by the step in $00/$02 and returns the
 * flags of the final high-word subtraction. */
static void StepRowPosition(
    Lufia2Wram wram, uint8_t *carry, uint8_t *overflow) {
    const uint16_t low = WramRead16(wram, ROW_POSITION);
    const uint16_t low_step = WramRead16(wram, WORK_VALUE);
    const uint16_t borrow = low < low_step;
    uint16_t high;
    uint16_t high_step;
    uint16_t next;

    WramWrite16(wram, ROW_POSITION, (uint16_t)(low - low_step));
    high = WramRead16(wram, ROW_WHOLE);
    high_step = WramRead16(wram, WORK_HIGH_WORD);
    next = (uint16_t)(high - high_step - borrow);
    *carry = (uint32_t)high >= (uint32_t)high_step + borrow;
    *overflow = (uint8_t)((((high ^ high_step) & (high ^ next)) & 0x8000u) != 0);
    WramWrite16(wram, ROW_WHOLE, next);
}

/* Fills BAND_ROWS scanlines of both tables, from the end downwards. A cosine
 * term with a zero magnitude byte is 0 or exactly 1.0 and skips the
 * multiplier. Returns the table index the band stopped at; `last_index` is
 * the index register the last row left behind. */
static BandExit BuildBand(
    Lufia2Wram wram, uint16_t y, unsigned quadrant) {
    BandExit done;
    const int scaled = (WramRead16(wram, COSINE_TERM) & 0x00ffu) != 0;

    done.last_index = 0;
    do {
        uint16_t cosine;
        uint16_t sine;
        const uint16_t index = (uint16_t)(WramRead16(wram, ROW_WHOLE) << 1);

        done.last_index = index;     /* the scaled path overwrites it */
        y = (uint16_t)(y - ROW_BYTES);
        if (scaled) {
            WramWrite16(wram, ROW_SCALE,
                WramRead16At(wram, RECIPROCAL_TABLE, index));
            cosine = ScaledTerm(wram, COSINE_TERM);
            if (quadrant & QUADRANT_NEGATE_COSINE)
                cosine = Negate16(cosine);
            StoreCosine(wram, y, cosine);
            sine = ScaledSecondTerm(wram, SINE_TERM, &done.last_index);
        } else {
            cosine = WramRead16(wram, COSINE_TERM);
            if (cosine)
                cosine = WramRead16At(wram, RECIPROCAL_TABLE, index);
            if (quadrant & QUADRANT_NEGATE_COSINE)
                cosine = Negate16(cosine);
            StoreCosine(wram, y, cosine);
            sine = WramRead16(wram, SINE_TERM);
            if (sine)
                sine = WramRead16At(wram, RECIPROCAL_TABLE, index);
        }
        StoreSine(wram, y, sine, quadrant);
        StepRowPosition(wram, &done.carry, &done.overflow);
    } while (WramStep16(wram, ROWS_LEFT, -1) != 0);
    done.table_index = y;
    return done;
}

/* ROL of a byte in memory through the carry. */
static void RotateByteIn(Lufia2Wram wram, uint32_t location, unsigned bit) {
    WramWrite(wram, location, (uint8_t)((WramRead(wram, location) << 1) | bit));
}

/* $86:A894: HDMA matrix tables of the world map ground plane for the current
 * view angle: two bands of 112 scanlines, each a rotation scaled by the
 * reciprocal of the row depth. */
Lufia2ExecutionResult Lufia2WorldMapPlane(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    unsigned quadrant;
    unsigned band;
    uint8_t angle;
    BandExit done = {0, 0, 0, 0};

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86a894u);
    wram = WramViewOfCaller(memory, cpu);
    angle = WramRead(wram, WRAM_WORLD_MAP_VIEW_ANGLE);
    SimulateJsrFrame(memory, cpu, 0xa899u);
    CosineTerm(memory, cpu, wram, angle);
    SimulateRtsFrame(memory, cpu);
    WramWrite16(wram, COSINE_TERM, WramRead16(wram, WORK_VALUE));
    SetNz8(cpu, angle);
    SimulateJsrFrame(memory, cpu, 0xa8a3u);
    SineTerm(memory, cpu, wram, angle);
    SimulateRtsFrame(memory, cpu);
    WramWrite16(wram, SINE_TERM, WramRead16(wram, WORK_VALUE));
    cpu->y = TABLE_END;
    WramWriteAt(wram, MATRIX_AB_TABLE, cpu->y, 0);
    WramWriteAt(wram, MATRIX_CD_TABLE, cpu->y, 0);
    /* The sign bytes pick one of four sign patterns for the bands. */
    WramWrite(wram, BAND_QUADRANT, 0);
    WramWrite(wram, BAND_QUADRANT_HIGH, 0);
    RotateByteIn(wram, BAND_QUADRANT, WramRead(wram, SINE_SIGN) >> 7);
    RotateByteIn(wram, BAND_QUADRANT, WramRead(wram, COSINE_SIGN) >> 7);
    RotateByteIn(wram, BAND_QUADRANT, 0);
    SetAccumulatorWidth(cpu, 0);
    WramWrite16(wram, ROW_POSITION, 0);
    cpu->x = WramRead16(wram, PLANE_START_ROW);
    WramWrite16(wram, ROW_WHOLE, cpu->x);
    if ((WramRead16(wram, PLANE_MODE) & 0x00ffu) != 0) {
        SimulateJsrFrame(memory, cpu, 0xa8d7u);
        RowStepFromTiltTable(memory, cpu, wram);
    } else {
        SimulateJsrFrame(memory, cpu, 0xa8dcu);
        RowStepFromSineDifference(memory, cpu, wram);
    }
    SimulateRtsFrame(memory, cpu);
    for (band = 0; band < 2u; ++band) {
        uint16_t rows;
        uint8_t header;

        SetAccumulatorWidth(cpu, 0);
        PushStackWord(memory, cpu, BAND_ROWS);
        WramWrite16(wram, ROWS_LEFT, BAND_ROWS);
        quadrant = (WramRead16(wram, BAND_QUADRANT) >> 1) & 3u;
        SimulateJsrFrame(memory, cpu, band ? 0xa902u : 0xa8e9u);
        done = BuildBand(wram, cpu->y, quadrant);
        SimulateRtsFrame(memory, cpu);
        rows = PullStackWord(memory, cpu);
        /* Each band ends with a repeat-count header byte in both tables. */
        header = (uint8_t)(rows | BAND_HEADER);
        cpu->y = (uint16_t)(done.table_index - 1u);
        WramWriteAt(wram, MATRIX_AB_TABLE, cpu->y, header);
        WramWriteAt(wram, MATRIX_CD_TABLE, cpu->y, header);
        cpu->x = done.last_index;
        cpu->accumulator = (uint16_t)((rows & 0xff00u) | header);
        cpu->carry = done.carry;
        cpu->overflow = done.overflow;
        SetNz8(cpu, header);
        SetAccumulatorWidth(cpu, 1);
    }
    WramWrite(wram, PLANE_REFRESH, 0);
    return ExecutionReturned(0x86a912u);
}
