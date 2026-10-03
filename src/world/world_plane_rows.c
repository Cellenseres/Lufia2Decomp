/* World map plane rows: the four quadrant variants that fill the scanline
 * tables at $1718/$1A9B from a rotating scale value. Each variant scales
 * the table entry at $D3B7 by the two factors in $58 and $5A, or copies it
 * when the factor has no fractional part, and counts the angle in $22/$24
 * down by the step in $00/$02. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "core/plain_ops.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "lufia2/world_map.h"

enum {
    FACTOR_A = 0x58u,
    FACTOR_B = 0x5au,
    ANGLE_LOW = 0x22u,
    ANGLE_HIGH = 0x24u,
    ANGLE_STEP_LOW = 0x00u,
    ANGLE_STEP_HIGH = 0x02u,
    ROWS_LEFT = 0x26u,
    SCALE = 0x4eu,
    SCALE_HIGH = 0x4fu,
    SCALE_TABLE = 0xd3b7u,
    TABLE_A = 0x1718u,
    TABLE_A_MIRROR = 0x1a9du,
    TABLE_B = 0x171au,
    TABLE_B_MIRROR = 0x1a9bu,
    ROW_BYTES = 4u
};

typedef struct {
    bool negate_a;            /* the first value is stored negated */
    bool mirror_first;        /* the second value goes to the mirror table first */
    uint32_t exit_scaled;
    uint32_t exit_plain;
} Quadrant;

/* A factor scaled by the row's reciprocal: ($58 or $5A) * $4E / 256 through
 * the multiply unit (reached through the data bank). The high byte of the
 * first product is read, then the second product is added to it. */
typedef struct {
    Word16Result sum;
    uint16_t high_read;       /* the 16-bit read of the high product byte */
} ScaledFactor;

static ScaledFactor ScaleFactor(Lufia2Wram wram, uint8_t factor) {
    ScaledFactor scaled;

    WramWrite(wram, SNES_WRMPYA, WramRead(wram, factor));
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, SCALE));
    scaled.high_read = WramRead16(wram, SNES_RDMPYH);
    WramWrite(wram, SNES_WRMPYB, WramRead(wram, SCALE_HIGH));
    scaled.sum = Sum16(
        (uint16_t)(scaled.high_read & 0x00ffu), WramRead16(wram, SNES_RDMPYL),
        false);
    return scaled;
}

/* The table entry itself, or zero for a zero factor. */
static uint16_t TableOrZero(Lufia2Wram wram, uint8_t factor, uint16_t index) {
    if (WramRead16(wram, factor) == 0)
        return 0;
    return WramRead16At(wram, SCALE_TABLE, index);
}

static uint16_t Negated(uint16_t value) {
    return (uint16_t)(~value + 1u);
}

static void StoreRow(
    Lufia2Wram wram, uint32_t table, uint16_t row, uint16_t value) {
    WramWrite16At(wram, table, row, value);
}

/* The angle counts down by the step; the carry of the low word runs into
 * the high word. Returns the high word's sum. */
static Word16Result NextAngle(Lufia2Wram wram) {
    const Word16Result low = Difference16(
        WramRead16(wram, ANGLE_LOW), WramRead16(wram, ANGLE_STEP_LOW));
    Word16Result high;

    WramWrite16(wram, ANGLE_LOW, low.value);
    high = Sum16(WramRead16(wram, ANGLE_HIGH),
        (uint16_t)~WramRead16(wram, ANGLE_STEP_HIGH), low.carry);
    WramWrite16(wram, ANGLE_HIGH, high.value);
    return high;
}

static Lufia2ExecutionResult Rows(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, const Quadrant *q, uint32_t entry) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const bool scaled = (WramRead16(wram, FACTOR_A) & 0x00ffu) != 0;
    uint16_t row = cpu->y;
    uint16_t index;
    uint16_t read_back = 0;
    Word16Result angle;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    do {
        uint16_t first;
        uint16_t second;
        ScaledFactor product;

        row = (uint16_t)(row - ROW_BYTES);
        index = (uint16_t)(WramRead16(wram, ANGLE_HIGH) << 1);
        if (scaled) {
            WramWrite16(wram, SCALE, WramRead16At(wram, SCALE_TABLE, index));
            product = ScaleFactor(wram, FACTOR_A);
            first = product.sum.value;
        } else {
            first = TableOrZero(wram, FACTOR_A, index);
        }
        if (q->negate_a)
            first = Negated(first);
        StoreRow(wram, TABLE_A, row, first);
        StoreRow(wram, TABLE_A_MIRROR, row, first);
        if (scaled) {
            product = ScaleFactor(wram, FACTOR_B);
            second = product.sum.value;
            read_back = product.high_read;
        } else {
            second = TableOrZero(wram, FACTOR_B, index);
        }
        if (q->mirror_first) {
            StoreRow(wram, TABLE_B_MIRROR, row, second);
            StoreRow(wram, TABLE_B, row, Negated(second));
        } else {
            StoreRow(wram, TABLE_B, row, second);
            StoreRow(wram, TABLE_B_MIRROR, row, Negated(second));
        }
        angle = NextAngle(wram);
    } while (WramStep16(wram, ROWS_LEFT, -1) != 0);
    cpu->y = row;
    cpu->x = scaled ? read_back : index;
    cpu->accumulator = angle.value;
    SetSumFlags(cpu, angle);
    SetNz16(cpu, 0);
    return ExecutionReturned(scaled ? q->exit_scaled : q->exit_plain);
}

static const Quadrant QUADRANT[4] = {
    {false, false, 0x86aa22u, 0x86aa5au},
    {true, false, 0x86aad1u, 0x86ab0du},
    {true, true, 0x86ab84u, 0x86abc0u},
    {false, true, 0x86ac33u, 0x86ac6bu}
};

static const uint32_t ENTRY[4] = {0x86a9b0u, 0x86aa5bu, 0x86ab0eu, 0x86abc1u};

Lufia2ExecutionResult Lufia2WorldPlaneRows0(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Rows(memory, cpu, &QUADRANT[0], ENTRY[0]);
}

Lufia2ExecutionResult Lufia2WorldPlaneRows1(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Rows(memory, cpu, &QUADRANT[1], ENTRY[1]);
}

Lufia2ExecutionResult Lufia2WorldPlaneRows2(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Rows(memory, cpu, &QUADRANT[2], ENTRY[2]);
}

Lufia2ExecutionResult Lufia2WorldPlaneRows3(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    return Rows(memory, cpu, &QUADRANT[3], ENTRY[3]);
}
