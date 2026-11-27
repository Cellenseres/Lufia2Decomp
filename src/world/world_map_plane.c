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
/* Runs the bound multiply ($86:A52F) and divide ($86:A5A9) children. */
static uint8_t PlaneChild(void *context, Lufia2CpuState *cpu,
    uint32_t target, uint32_t site, uint8_t frame) {
    const Lufia2Memory *memory = context;
    Lufia2ExecutionResult result;

    (void)site;
    if (frame != 2u || (target != 0x86a52fu && target != 0x86a5a9u)) {
        cpu->resume_pc = target;
        return 0u;
    }
    result = target == 0x86a52fu ? Lufia2WorldMapMultiply16(memory, cpu)
                                 : Lufia2WorldMapDivide32(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return 0u;
    SimulateRtsFrame(memory, cpu);
    return 1u;
}

Lufia2ExecutionResult Lufia2WorldMapPlane(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2Wram wram;
    unsigned quadrant;
    unsigned band;
    uint8_t angle;
    BandExit done = {0, 0, 0, 0};
    Lufia2ExecutionResult step;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->program_bank != 0x86u || cpu->data_bank != 0x86u ||
        cpu->direct_page != 0 || cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x86a894u);
    wram = WramViewOfCaller(memory, cpu);
    LoadA8(cpu, WramRead(wram, WRAM_WORLD_MAP_VIEW_ANGLE));
    SimulateJsrFrame(memory, cpu, 0xa899u);
    step = Lufia2WorldMapResolveAngleScale(memory, cpu);
    if (step.flow != LUFIA2_EXECUTION_RETURNED)
        return step;
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
        step = Lufia2WorldMapProjectTiltDistance(memory, cpu, PlaneChild, (void *)memory);
    } else {
        SimulateJsrFrame(memory, cpu, 0xa8dcu);
        step = Lufia2WorldMapProjectTiltReciprocal(memory, cpu, PlaneChild, (void *)memory);
    }
    if (step.flow == LUFIA2_EXECUTION_CHILD_UNWOUND)
        return ExecutionHandoff(cpu, cpu->resume_pc);
    if (step.flow != LUFIA2_EXECUTION_RETURNED)
        return step;
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
