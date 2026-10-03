/* World map plane rows: the four quadrant variants that fill the scanline
 * tables at $1718/$1A9B from a rotating scale value. Each variant scales
 * the table entry at $D3B7 by the two factors in $58 and $5A, or copies it
 * when the factor has no fractional part, and counts the angle in $22/$24
 * down by the step in $00/$02. */

#include "core/cpu_internal.h"
#include "core/snes_registers.h"
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
    TABLE_B_MIRROR = 0x1a9bu
};

typedef struct {
    int negate_a;             /* the first value is stored negated */
    int mirror_first;         /* the second value goes to the mirror table first */
    uint32_t exit_scaled;
    uint32_t exit_plain;
} Quadrant;

/* DEC dp, 16-bit, high byte first. */
static void DecrementDirect16(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t offset) {
    const uint16_t address = (uint16_t)(cpu->direct_page + offset);
    const uint16_t value = (uint16_t)(Read16Direct(memory, cpu, offset) - 1u);

    Write8(memory, (uint16_t)(address + 1u), (uint8_t)(value >> 8));
    Write8(memory, address, (uint8_t)value);
    SetNz16(cpu, value);
}

static void Negate16(Lufia2CpuState *cpu) {
    LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
}

static void StoreRow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address) {
    StoreAAbsolute16(memory, cpu, address, cpu->y);
}

/* The 4 x DEY that move to the previous row. */
static void PreviousRow(Lufia2CpuState *cpu) {
    int i;

    for (i = 0; i < 4; ++i)
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
}

/* ($58 or $5A) * $4E / 256 through the multiply unit (reached through the
 * data bank): the high byte of the low product plus the product of the high
 * byte. The first write of the
 * high byte follows the read of the product, as in the original. */
static void ScaledFactor(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t factor, int nop_first) {
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, factor));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYA, 0);
    LoadA8(cpu, DirectByte(memory, cpu, SCALE));
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    if (nop_first) {
        LoadA8(cpu, DirectByte(memory, cpu, SCALE_HIGH));
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYH, 0));
    } else {
        PreviousRow(cpu);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYH, 0));
        LoadA8(cpu, DirectByte(memory, cpu, SCALE_HIGH));
    }
    StoreAAbsolute8(memory, cpu, SNES_WRMPYB, 0);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    And16(cpu, 0x00ffu);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, SNES_RDMPYL, 0));
}

static void NextAngle(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16Direct(memory, cpu, ANGLE_LOW));
    Subtract16(cpu, Read16Direct(memory, cpu, ANGLE_STEP_LOW));
    StoreADirect16(memory, cpu, ANGLE_LOW);
    LoadA16(cpu, Read16Direct(memory, cpu, ANGLE_HIGH));
    Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, ANGLE_STEP_HIGH));
    StoreADirect16(memory, cpu, ANGLE_HIGH);
    DecrementDirect16(memory, cpu, ROWS_LEFT);
}

static void ScaleIndex(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16Direct(memory, cpu, ANGLE_HIGH));
    AslA16(cpu);
    TransferAToX(cpu);
}

/* The rows when a factor has a fractional part. */
static void ScaledRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    const Quadrant *q) {
    do {
        ScaleIndex(memory, cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SCALE_TABLE, cpu->x));
        StoreADirect16(memory, cpu, SCALE);
        ScaledFactor(memory, cpu, FACTOR_A, 0);
        if (q->negate_a)
            Negate16(cpu);
        StoreRow(memory, cpu, TABLE_A);
        StoreRow(memory, cpu, TABLE_A_MIRROR);
        ScaledFactor(memory, cpu, FACTOR_B, 1);
        if (q->mirror_first) {
            StoreRow(memory, cpu, TABLE_B_MIRROR);
            Negate16(cpu);
            StoreRow(memory, cpu, TABLE_B);
        } else {
            StoreRow(memory, cpu, TABLE_B);
            Negate16(cpu);
            StoreRow(memory, cpu, TABLE_B_MIRROR);
        }
        NextAngle(memory, cpu);
    } while (!cpu->zero);
}

/* The table entry itself, or zero for a zero factor. */
static void TableOrZero(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t factor) {
    LoadA16(cpu, Read16Direct(memory, cpu, factor));
    if (!cpu->zero)
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SCALE_TABLE, cpu->x));
}

static void PlainRows(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    const Quadrant *q) {
    do {
        PreviousRow(cpu);
        ScaleIndex(memory, cpu);
        TableOrZero(memory, cpu, FACTOR_A);
        if (q->negate_a)
            Negate16(cpu);
        StoreRow(memory, cpu, TABLE_A);
        StoreRow(memory, cpu, TABLE_A_MIRROR);
        TableOrZero(memory, cpu, FACTOR_B);
        if (q->mirror_first) {
            StoreRow(memory, cpu, TABLE_B_MIRROR);
            Negate16(cpu);
            StoreRow(memory, cpu, TABLE_B);
        } else {
            StoreRow(memory, cpu, TABLE_B);
            Negate16(cpu);
            StoreRow(memory, cpu, TABLE_B_MIRROR);
        }
        NextAngle(memory, cpu);
    } while (!cpu->zero);
}

static Lufia2ExecutionResult Rows(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, const Quadrant *q, uint32_t entry) {
    Lufia2ExecutionResult result;

    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, entry);
    LoadA16(cpu, Read16Direct(memory, cpu, FACTOR_A));
    And16(cpu, 0x00ffu);
    if (cpu->zero) {
        PlainRows(memory, cpu, q);
        result = ExecutionReturned(q->exit_plain);
    } else {
        ScaledRows(memory, cpu, q);
        result = ExecutionReturned(q->exit_scaled);
    }
    return result;
}

static const Quadrant QUADRANT[4] = {
    {0, 0, 0x86aa22u, 0x86aa5au},
    {1, 0, 0x86aad1u, 0x86ab0du},
    {1, 1, 0x86ab84u, 0x86abc0u},
    {0, 1, 0x86ac33u, 0x86ac6bu}
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
