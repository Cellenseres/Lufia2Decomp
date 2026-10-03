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
    /* Each HDMA scanline holds two matrix words per table. */
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

/* Quarter-wave angles and reciprocal row depth tables. */
#define SINE_QUARTER_TABLE 0x97b226u
#define RECIPROCAL_TABLE 0xd3b7u

/* Cosine and sine signs select the quadrant builder. */
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

/* Angle terms retain their original status and stack residue. */

/* An angle folded into the first quarter of the wave. */
static uint8_t FoldIntoQuarter(uint8_t angle, bool decimal) {
    return (angle & 0x40u) != 0 ? Sum8Mode(
        (uint8_t)(0u - (angle & 0x3fu)), 0x40u, false, decimal).value : angle;
}

/* $86:A4FA: signed cosine term; retain decimal arithmetic flags. */
static void CosineTerm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint8_t angle) {
    const uint16_t steps = FoldIntoQuarter(angle, cpu->decimal) & 0x7fu;
    const Word16Result offset = Sum16Mode((uint16_t)(0u - steps),
        0x40u, false, cpu->decimal);
    uint16_t magnitude;
    uint8_t quarter;

    Push8(memory, cpu, angle);
    magnitude = SineTableWord(memory, (uint16_t)(offset.value << 1));
    WramWrite16(wram, WORK_VALUE, magnitude);
    quarter = Pull8(memory, cpu) & 0xc0u;
    cpu->carry = quarter == 0xc0u;
    cpu->overflow = offset.overflow;
    if (quarter != 0x00u && quarter != 0xc0u)
        WramWrite(wram, WORK_QUOTIENT_BYTE,
            (uint8_t)((uint16_t)(0u - magnitude) >> 8));
}

/* $86:A4D3: signed sine term; restore the pushed status. */
static void SineTerm(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram,
    uint8_t angle) {
    const uint16_t steps = FoldIntoQuarter(angle, cpu->decimal) & 0x7fu;
    uint16_t magnitude;

    cpu->zero = (angle & 0x80u) == 0;
    Push8(memory, cpu, PackStatus(cpu));
    magnitude = SineTableWord(memory, (uint16_t)(steps << 1));
    WramWrite16(wram, WORK_VALUE, magnitude);
    UnpackStatus(cpu, Pull8(memory, cpu));
    if (!cpu->zero)
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
    sum = Sum16Mode(sum, WramRead16(wram, SNES_RDMPYL), false, cpu->decimal).value;
    WramWrite16(wram, WORK_QUOTIENT_BYTE, sum);
    WramWrite(wram, SNES_WRMPYA, Pull8(memory, cpu));
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, WORK_OPERAND));
    {
        const uint16_t before = WramRead16(wram, WORK_QUOTIENT_BYTE);
        const Word16Result total = Sum16Mode(before,
            WramRead16(wram, SNES_RDMPYL), false, cpu->decimal);

        WramWrite16(wram, WORK_QUOTIENT_BYTE, total.value);
        high = (uint8_t)((WramRead(wram, WORK_HIGH_BYTE) << 1) | total.carry);
        WramWrite(wram, WORK_HIGH_BYTE, high);
    }
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, WORK_OPERAND_HIGH));
    {
        const uint16_t before = WramRead16(wram, WORK_HIGH_WORD);

        sum = Sum16Mode(before, WramRead16(wram, SNES_RDMPYL),
            false, cpu->decimal).value;
        WramWrite16(wram, WORK_HIGH_WORD, sum);
    }
    cpu->accumulator = sum;
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* The restoring divider is shared with object projection. */
static void DivideWork(Lufia2Wram wram, Lufia2CpuState *cpu, uint16_t return_address) {
    SimulateJsrFrame(wram.memory, cpu, return_address);
    (void)Lufia2WorldMapDivide32(wram.memory, cpu);
    SimulateRtsFrame(wram.memory, cpu);
}

/* $86:A913: derive the row step from the tilt table. */
static void RowStepFromTiltTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, Lufia2Wram wram) {
    uint16_t tilt;
    uint16_t sine;
    uint16_t start_row;
    Word16Result remaining;
    uint16_t divisor;

    WramWrite16(wram, WORK_VALUE, 0);
    WramWrite16(wram, WORK_HIGH_WORD, 0);
    tilt = WramRead16(wram, PLANE_TILT) & 0x00ffu;
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
    remaining.value = WramRead16(wram, PLANE_START_ROW);
    remaining = Difference16Mode(remaining.value,
        WramRead16(wram, WORK_QUOTIENT_BYTE), cpu->decimal);
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
    const uint16_t start_row = cpu->x;
    uint8_t complement_sine;
    Byte8Result difference;
    uint16_t quotient;
    Word16Result remaining;

    WramWrite16(wram, WORK_HIGH_WORD, 0);
    WramWrite16(wram, WORK_VALUE, 0);
    SetAccumulatorWidth(cpu, 1);
    difference = Difference8Mode(0x40u, WramRead(wram, PLANE_TILT), cpu->decimal);
    if (!difference.carry || difference.value == 0)
        return;
    WramWrite16(wram, WORK_HIGH_WORD, start_row);
    complement_sine = SineTableByte(memory, (uint8_t)(difference.value << 1));
    WramWrite(wram, WORK_LIMIT, complement_sine);
    difference.value = SineTableByte(memory,
        (uint8_t)(WramRead(wram, PLANE_TILT) << 1));
    difference = Difference8Mode(difference.value,
        WramRead(wram, WORK_LIMIT), cpu->decimal);
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
    remaining.value = WramRead16(wram, PLANE_START_ROW);
    remaining = Difference16Mode(remaining.value,
        WramRead16(wram, WORK_VALUE), cpu->decimal);
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

/* The same quadrant builders are exposed as independent ROM entries. */
static BandExit BuildBand(Lufia2Wram wram, Lufia2CpuState *cpu,
    uint16_t y, unsigned quadrant) {
    static Lufia2ExecutionResult (*const builders[4])(
        const Lufia2Memory *, Lufia2CpuState *) = {
        Lufia2WorldPlaneRows0, Lufia2WorldPlaneRows1,
        Lufia2WorldPlaneRows3, Lufia2WorldPlaneRows2
    };
    BandExit done;

    cpu->y = y;
    (void)builders[quadrant](wram.memory, cpu);
    done.table_index = cpu->y;
    done.last_index = cpu->x;
    done.carry = cpu->carry;
    done.overflow = cpu->overflow;
    return done;
}

/* ROL of a byte in memory through the carry. */
static void RotateByteIn(Lufia2Wram wram, uint32_t location, unsigned bit) {
    WramWrite(wram, location, (uint8_t)((WramRead(wram, location) << 1) | bit));
}

/* $86:A894: build two bands of 112 Mode 7 perspective scanlines. */
Lufia2ExecutionResult Lufia2WorldMapPlane(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    unsigned quadrant;
    unsigned band;
    uint8_t angle;
    BandExit done = {0, 0, 0, 0};

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        !DirectWorkWordAvailable(cpu, ROWS_LEFT))
        return ExecutionHandoff(cpu, 0x86a894u);
    wram = WramViewOfCaller(memory, cpu);
    angle = WramRead(wram, WRAM_WORLD_MAP_VIEW_ANGLE);
    SimulateJsrFrame(memory, cpu, 0xa899u);
    CosineTerm(memory, cpu, wram, angle);
    SimulateRtsFrame(memory, cpu);
    WramWrite16(wram, COSINE_TERM, WramRead16(wram, WORK_VALUE));
    angle = WramRead(wram, WRAM_WORLD_MAP_VIEW_ANGLE);
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
        done = BuildBand(wram, cpu, cpu->y, quadrant);
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
