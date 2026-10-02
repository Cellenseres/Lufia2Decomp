/* World map ground plane: the Mode 7 matrix tables of the perspective view. */

#include "core/cpu_internal.h"
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
 * the value 1.0 is the pair 0, 1. The routines keep their original shape
 * because they leave flags, work bytes and stack residue for the caller. */

/* $86:A4FA: cosine term of angle A into $00. */
static void CosineTerm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    const uint8_t angle = A8(cpu);
    uint8_t quadrant;

    Push8(memory, cpu, angle);
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero) {
        And8(cpu, 0x3fu);
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        cpu->carry = 0;
        Adc8(cpu, 0x40u);
    }
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x007fu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    IncrementA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0040u);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, SineTableWord(memory, cpu->x));
    WramWrite16(wram, WORK_VALUE, cpu->accumulator);
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    IncrementA16(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    And8(cpu, 0xc0u);
    quadrant = A8(cpu);
    if (quadrant == 0x00u)
        return;
    Compare8(cpu, quadrant, 0xc0u);
    if (cpu->zero)
        return;
    ExchangeAccumulatorBytes(cpu);
    WramWrite(wram, WORK_QUOTIENT_BYTE, A8(cpu));
}

/* $86:A4D3: sine term of angle A into $00. */
static void SineTerm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    BitImmediate8(cpu, 0x80u);
    Push8(memory, cpu, PackStatus(cpu));
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero) {
        And8(cpu, 0x3fu);
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        cpu->carry = 0;
        Adc8(cpu, 0x40u);
    }
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x007fu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, SineTableWord(memory, cpu->x));
    WramWrite16(wram, WORK_VALUE, cpu->accumulator);
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    IncrementA16(cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    if (cpu->zero)
        return;
    ExchangeAccumulatorBytes(cpu);
    WramWrite(wram, WORK_QUOTIENT_BYTE, A8(cpu));
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

/* $86:A5A9: restoring division of the 32-bit value at $00 by the word at $04;
 * the quotient replaces the value and the remainder is left in A. */
static void DivideLong(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    uint16_t remainder = 0;
    unsigned bit;

    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    for (bit = 0; bit < 32u; ++bit) {
        uint16_t low = WramRead16(wram, WORK_VALUE);
        uint16_t high;
        uint8_t carry = (uint8_t)(low >> 15);
        uint8_t overflow;

        low = (uint16_t)(low << 1);
        Write8(memory, WramAddress(wram, WORK_QUOTIENT_BYTE, 0), (uint8_t)(low >> 8));
        Write8(memory, WramAddress(wram, WORK_VALUE, 0), (uint8_t)low);
        high = WramRead16(wram, WORK_HIGH_WORD);
        {
            const uint16_t shifted = (uint16_t)((high << 1) | carry);

            carry = (uint8_t)(high >> 15);
            Write8(memory, WramAddress(wram, WORK_HIGH_BYTE, 0), (uint8_t)(shifted >> 8));
            Write8(memory, WramAddress(wram, WORK_HIGH_WORD, 0), (uint8_t)shifted);
        }
        overflow = (uint8_t)(remainder >> 15);
        remainder = (uint16_t)((remainder << 1) | carry);
        if (overflow || remainder >= WramRead16(wram, WORK_OPERAND)) {
            remainder = (uint16_t)(remainder - WramRead16(wram, WORK_OPERAND));
            (void)WramStep16(wram, WORK_VALUE, 1);
        }
    }
    cpu->accumulator = remainder;
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $86:A913: row step from the tilt table, for a non-zero mode. */
static void RowStepFromTiltTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    WramWrite16(wram, WORK_VALUE, 0);
    WramWrite16(wram, WORK_HIGH_WORD, 0);
    LoadA16(cpu, WramRead16(wram, PLANE_TILT));
    And16(cpu, 0x00ffu);
    Compare16(cpu, cpu->accumulator, 0x0040u);
    if (cpu->carry)
        return;
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, SineTableWord(memory, cpu->x));
    PushAccumulator16(memory, cpu);
    WramWrite16(wram, WORK_OPERAND, cpu->accumulator);
    LoadA16(cpu, WramRead16(wram, PLANE_START_ROW));
    WramWrite16(wram, WORK_VALUE, cpu->accumulator);
    SimulateJsrFrame(memory, cpu, 0xa932u);
    MultiplyWords(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
    PullAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    WramWrite(wram, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, 0xe0u);
    WramWrite(wram, SNES_WRMPYB, A8(cpu));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, PLANE_START_ROW));
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~WramRead16(wram, WORK_QUOTIENT_BYTE));
    WramWrite16(wram, WORK_HIGH_WORD, cpu->accumulator);
    WramWrite16(wram, WORK_VALUE, 0);
    LoadA16(cpu, WramRead16(wram, SNES_RDMPYH));
    And16(cpu, 0x00ffu);
    WramWrite16(wram, WORK_OPERAND, cpu->accumulator);
    SimulateJsrFrame(memory, cpu, 0xa954u);
    DivideLong(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
}

/* $86:A956: row step from the difference of two sine entries. */
static void RowStepFromSineDifference(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    WramWrite16(wram, WORK_HIGH_WORD, 0);
    WramWrite16(wram, WORK_VALUE, 0);
    LoadA16(cpu, 0x0000u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x40u);
    cpu->carry = 1;
    Sbc8(cpu, WramRead(wram, PLANE_TILT));
    if (cpu->zero || !cpu->carry)
        return;
    WramWrite16(wram, WORK_HIGH_WORD, cpu->x);
    AslA8(cpu);
    TransferAToX(cpu);
    LoadA8(cpu, SineTableByte(memory, cpu->x));
    WramWrite(wram, WORK_LIMIT, A8(cpu));
    LoadA8(cpu, WramRead(wram, PLANE_TILT));
    AslA8(cpu);
    TransferAToX(cpu);
    LoadA8(cpu, SineTableByte(memory, cpu->x));
    cpu->carry = 1;
    Sbc8(cpu, WramRead(wram, WORK_LIMIT));
    WramWrite(wram, WORK_OPERAND, A8(cpu));
    WramWrite(wram, WORK_OPERAND_HIGH, 0);
    SimulateJsrFrame(memory, cpu, 0xa985u);
    DivideLong(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
    LoadX16(cpu, WramRead16(wram, WORK_QUOTIENT_BYTE));
    WramWrite16(wram, WORK_OPERAND, cpu->x);
    LoadX16(cpu, 0x0000u);
    WramWrite16(wram, WORK_VALUE, cpu->x);
    LoadX16(cpu, 0x0001u);
    WramWrite16(wram, WORK_HIGH_WORD, cpu->x);
    SimulateJsrFrame(memory, cpu, 0xa996u);
    DivideLong(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, WramRead16(wram, PLANE_START_ROW));
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~WramRead16(wram, WORK_VALUE));
    WramWrite16(wram, WORK_QUOTIENT_BYTE, cpu->accumulator);
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
    BandExit done = {0, 0, 0, 0};

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86a894u);
    wram = WramViewOfCaller(memory, cpu);
    LoadA8(cpu, WramRead(wram, WRAM_WORLD_MAP_VIEW_ANGLE));
    SimulateJsrFrame(memory, cpu, 0xa899u);
    CosineTerm(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
    LoadX16(cpu, WramRead16(wram, WORK_VALUE));
    WramWrite16(wram, COSINE_TERM, cpu->x);
    LoadA8(cpu, WramRead(wram, WRAM_WORLD_MAP_VIEW_ANGLE));
    SimulateJsrFrame(memory, cpu, 0xa8a3u);
    SineTerm(memory, cpu, wram);
    SimulateRtsFrame(memory, cpu);
    LoadX16(cpu, WramRead16(wram, WORK_VALUE));
    WramWrite16(wram, SINE_TERM, cpu->x);
    LoadY16(cpu, TABLE_END);
    LoadA8(cpu, 0x00u);
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
    LoadA16(cpu, WramRead16(wram, PLANE_START_ROW));
    WramWrite16(wram, ROW_WHOLE, cpu->accumulator);
    TransferAToX(cpu);
    LoadA16(cpu, WramRead16(wram, PLANE_MODE));
    And16(cpu, 0x00ffu);
    if (!cpu->zero) {
        SimulateJsrFrame(memory, cpu, 0xa8d7u);
        RowStepFromTiltTable(memory, cpu, wram);
    } else {
        SimulateJsrFrame(memory, cpu, 0xa8dcu);
        RowStepFromSineDifference(memory, cpu, wram);
    }
    SimulateRtsFrame(memory, cpu);
    for (band = 0; band < 2u; ++band) {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, BAND_ROWS);
        PushAccumulator16(memory, cpu);
        WramWrite16(wram, ROWS_LEFT, cpu->accumulator);
        LoadX16(cpu, WramRead16(wram, BAND_QUADRANT));
        quadrant = (cpu->x >> 1) & 3u;
        SimulateJsrFrame(memory, cpu, band ? 0xa902u : 0xa8e9u);
        done = BuildBand(wram, cpu->y, quadrant);
        SimulateRtsFrame(memory, cpu);
        cpu->x = done.last_index;
        cpu->y = done.table_index;
        cpu->carry = done.carry;
        cpu->overflow = done.overflow;
        PullAccumulator16(memory, cpu);
        SetAccumulatorWidth(cpu, 1);
        cpu->y = (uint16_t)(cpu->y - 1u);
        SetNz16(cpu, cpu->y);
        Or8(cpu, BAND_HEADER);
        WramWriteAt(wram, MATRIX_AB_TABLE, cpu->y, A8(cpu));
        WramWriteAt(wram, MATRIX_CD_TABLE, cpu->y, A8(cpu));
    }
    WramWrite(wram, PLANE_REFRESH, 0);
    return ExecutionReturned(0x86a912u);
}
