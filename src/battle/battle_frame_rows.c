/* Battle window frame rows and small lookups of bank $81. */

#include "core/cpu_ops.h"
#include "lufia2/battle.h"

enum {
    FRAME_TILE = 0x09f4u,               /* first frame tile */
    FRAME_WIDTH = 0x09fau,
    FRAME_ROW = 0x09fcu,                /* tilemap row, $40 each */
    FRAME_COUNT = 0x0a3cu,
};

/* Stores A at absolute offset + Y. */
static void Store16Y(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t offset) {
    StoreAAbsolute16(memory, cpu, offset, cpu->y);
}

/* Stores A at absolute offset + X. */
static void Store16X(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t offset) {
    StoreAAbsolute16(memory, cpu, offset, cpu->x);
}

/* Word at an absolute address in the data bank. */
static uint16_t Abs16(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t address) {
    return Read16AbsoluteIndexed(memory, cpu, address, 0);
}

/* DEC abs, 16-bit; returns the new value. */
static uint16_t DecAbs16(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address) {
    const uint16_t value = (uint16_t)(Abs16(memory, cpu, address) - 1u);

    StoreWordAbsolute(memory, cpu, address, value);
    SetNz16(cpu, value);
    return value;
}

/* Y += 2. */
static void IncY2(Lufia2CpuState *cpu) {
    IncrementY16(cpu);
    IncrementY16(cpu);
}

/* X += 2. */
static void IncX2(Lufia2CpuState *cpu) {
    IncrementX16(cpu);
    IncrementX16(cpu);
}

/* Y = next tilemap row. */
static void NextRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA16(cpu, Abs16(memory, cpu, FRAME_ROW));
    cpu->carry = 0;
    Add16Value(cpu, 0x0040u);
    TransferAToY(cpu);
    StoreWordAbsolute(memory, cpu, FRAME_ROW, cpu->accumulator);
}

/* Y += 2 * width, twice through ADC. */
static void SkipWidth(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, Abs16(memory, cpu, FRAME_WIDTH));
    Add16Value(cpu, Abs16(memory, cpu, FRAME_WIDTH));
    TransferAToY(cpu);
}

/* $81:E479: top edge: corner, width - 1 fills, mirrored corner; M0X0. */
Lufia2ExecutionResult Lufia2BattleFrameTop(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, (uint16_t)(Abs16(memory, cpu, FRAME_WIDTH) - 2u));
    StoreWordAbsolute(memory, cpu, FRAME_COUNT, cpu->accumulator);
    LoadA16(cpu, Abs16(memory, cpu, FRAME_TILE));
    Store16Y(memory, cpu, 0x0000u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
    Store16Y(memory, cpu, 0x0002u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
    do {
        Store16Y(memory, cpu, 0x0004u);
        IncY2(cpu);
    } while (!(DecAbs16(memory, cpu, FRAME_COUNT) & 0x8000u));
    LoadA16(cpu, (uint16_t)((cpu->accumulator | 0x4000u) - 1u));
    Store16Y(memory, cpu, 0x0004u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    Store16Y(memory, cpu, 0x0006u);
    NextRow(memory, cpu);
    return ExecutionReturned(0x81e4acu);
}

/* $81:E4AD: side edges: tile, then mirrored at the right; M0X0. */
Lufia2ExecutionResult Lufia2BattleFrameSides(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, Abs16(memory, cpu, FRAME_TILE));
    Store16Y(memory, cpu, 0x0000u);
    SkipWidth(memory, cpu);
    LoadA16(cpu, (uint16_t)(Abs16(memory, cpu, FRAME_TILE) | 0x4000u));
    Store16Y(memory, cpu, 0x0004u);
    NextRow(memory, cpu);
    return ExecutionReturned(0x81e4d0u);
}

/* $81:E542: row from template $01:X (left, fill, right); M0X0. */
Lufia2ExecutionResult Lufia2BattleFrameRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, Abs16(memory, cpu, FRAME_WIDTH));
    StoreWordAbsolute(memory, cpu, FRAME_COUNT, cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x010000u, cpu->x)));
    Store16Y(memory, cpu, 0x0000u);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x010002u, cpu->x)));
    do {
        Store16Y(memory, cpu, 0x0002u);
        IncY2(cpu);
    } while (!(DecAbs16(memory, cpu, FRAME_COUNT) & 0x8000u));
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x010004u, cpu->x)));
    Store16Y(memory, cpu, 0x0002u);
    NextRow(memory, cpu);
    return ExecutionReturned(0x81e56fu);
}

/* $81:E570: template $01:X left and right tiles only; M0X0. */
Lufia2ExecutionResult Lufia2BattleFrameEnds(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x010000u, cpu->x)));
    Store16Y(memory, cpu, 0x0000u);
    SkipWidth(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x010002u, cpu->x)));
    Store16Y(memory, cpu, 0x0004u);
    NextRow(memory, cpu);
    return ExecutionReturned(0x81e592u);
}

/* $81:E5C1: 2x4 gauge block of tile A at X; M0X0. */
Lufia2ExecutionResult Lufia2BattleGaugeBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;
    int first = 1;

    cpu->carry = 0;
    Add16Value(cpu, 0x0005u);
    Store16X(memory, cpu, 0x0000u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
    IncX2(cpu);
    Store16X(memory, cpu, 0x0000u);
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)(0x0005u ^ 0xffffu));
    IncX2(cpu);
    LoadY16(cpu, 0x0004u);
    Write16Direct(memory, cpu, 0x11u, cpu->y);
    do {
        if (!first) {
            LoadA16(cpu, (uint16_t)(cpu->accumulator + 2u));   /* E5DB */
            Store16X(memory, cpu, 0x0000u);
            IncX2(cpu);
            LoadA16(cpu, (uint16_t)(cpu->accumulator - 2u));
        }
        first = 0;
        LoadY16(cpu, 0x0006u);                                 /* E5E4 */
        for (i = 0; i < 6u; ++i) {
            Store16X(memory, cpu, 0x0000u);
            IncX2(cpu);
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        }
        OpStepMem(memory, cpu, OpDp(cpu, 0x11u), -1);
    } while (!cpu->zero);
    Or16(cpu, 0x4000u);
    cpu->carry = 0;
    Add16Value(cpu, 0x0005u);
    Store16X(memory, cpu, 0x0000u);
    IncX2(cpu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    Store16X(memory, cpu, 0x0000u);
    return ExecutionReturned(0x81e603u);
}

/* $81:E604: gauge column of tile A at X, rows $10 apart; M0X0. */
Lufia2ExecutionResult Lufia2BattleGaugeColumn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    int first = 1;

    cpu->carry = 0;
    Add16Value(cpu, 0x0005u);
    Store16X(memory, cpu, 0x0000u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
    IncX2(cpu);
    Store16X(memory, cpu, 0x0000u);
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)(0x0004u ^ 0xffffu));
    TransferAToY(cpu);
    LoadA16(cpu, 0x0004u);
    StoreADirect16(memory, cpu, 0x11u);
    do {
        if (!first)
            Store16X(memory, cpu, 0x0000u);                    /* E61D */
        first = 0;
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x000eu);
        TransferAToX(cpu);
        LoadA16(cpu, cpu->y);
        OpStepMem(memory, cpu, OpDp(cpu, 0x11u), -1);
    } while (!cpu->zero);
    cpu->carry = 0;
    Add16Value(cpu, 0x0004u);
    Or16(cpu, 0x4000u);
    Store16X(memory, cpu, 0x0000u);
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    Store16X(memory, cpu, 0x0002u);
    return ExecutionReturned(0x81e639u);
}

/* Sets the data bank to $7E through the stack. */
static void SetBank7E(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* X += count. */
static void IncX(Lufia2CpuState *cpu, unsigned count) {
    while (count--)
        IncrementX16(cpu);
}

/* $81:E503: window $09F2 x $09F3 at $7E:X from template $01:$09F4. */
Lufia2ExecutionResult Lufia2BattleWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    SetBank7E(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x09f2u, 0);
    DecrementA8(cpu);
    StoreAAbsolute8(memory, cpu, 0x09fau, 0);
    StoreZeroAbsolute8(memory, cpu, 0x09fbu, 0);
    LoadAAbsolute8(memory, cpu, 0x09f3u, 0);
    StoreAAbsolute8(memory, cpu, 0x09feu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x09ffu, 0);
    StoreWordAbsolute(memory, cpu, FRAME_ROW, cpu->x);
    LoadY16(cpu, cpu->x);
    LoadX16(cpu, Abs16(memory, cpu, 0x09f4u));
    SetAccumulatorWidth(cpu, 0);
    SimulateJsrFrame(memory, cpu, 0xe526u);
    (void)Lufia2BattleFrameRow(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    IncX(cpu, 6u);
    while (!(DecAbs16(memory, cpu, 0x09feu) & 0x8000u)) {
        SimulateJsrFrame(memory, cpu, 0xe534u);
        (void)Lufia2BattleFrameEnds(memory, cpu);
        SimulateRtsFrame(memory, cpu);
    }
    IncX(cpu, 4u);
    SimulateJsrFrame(memory, cpu, 0xe53du);
    (void)Lufia2BattleFrameRow(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81e541u);
}

/* $81:E593: gauge panel at $7E:2D80-$2EFF; P kept. */
Lufia2ExecutionResult Lufia2BattleGaugePanel(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetBank7E(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, 0x2d80u);
    LoadA16(cpu, 0x2155u);
    SimulateJsrFrame(memory, cpu, 0xe5a3u);
    (void)Lufia2BattleGaugeBlock(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadX16(cpu, 0x2dc0u);
    do {
        LoadA16(cpu, 0x2157u);
        SimulateJsrFrame(memory, cpu, 0xe5acu);
        (void)Lufia2BattleGaugeColumn(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        IncX(cpu, 6u);
        Compare16(cpu, cpu->x, 0x2ec0u);
    } while (!cpu->zero);
    LoadA16(cpu, 0xa155u);
    SimulateJsrFrame(memory, cpu, 0xe5bdu);
    (void)Lufia2BattleGaugeBlock(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81e5c0u);
}

/* Fill inside X + $42 with tile, then window template at X. */
static Lufia2ExecutionResult FilledWindow(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t tile, uint16_t tmpl, uint16_t entry) {
    PushIndex(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0042u);
    TransferAToX(cpu);
    LoadA16(cpu, tile);
    StoreWordAbsolute(memory, cpu, 0x09f6u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    SimulateJsrFrame(memory, cpu, (uint16_t)(entry + 0x13u));
    (void)Lufia2BattleFillRect(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadX16(cpu, tmpl);
    StoreWordAbsolute(memory, cpu, 0x09f4u, cpu->x);
    cpu->x = PullIndexValue(memory, cpu);
    SimulateJsrFrame(memory, cpu, (uint16_t)(entry + 0x1du));
    (void)Lufia2BattleWindow(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x810000u | (uint16_t)(entry + 0x1eu));
}

/* $81:E3AE: window $87E3 at X, inside filled with $2154. */
Lufia2ExecutionResult Lufia2BattleWindowE3AE(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return FilledWindow(memory, cpu, 0x2154u, 0x87e3u, 0xe3aeu);
}

/* $81:E3CD: window $87F9 at X, inside filled with $2167. */
Lufia2ExecutionResult Lufia2BattleWindowE3CD(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return FilledWindow(memory, cpu, 0x2167u, 0x87f9u, 0xe3cdu);
}

/* Step a word by delta; N/Z from the last step. */
static void IncAbs16(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t address, int delta) {
    const int step = delta < 0 ? -1 : 1;

    while (delta) {
        const uint16_t value = (uint16_t)(Abs16(memory, cpu, address) + step);

        StoreWordAbsolute(memory, cpu, address, value);
        SetNz16(cpu, value);
        delta -= step;
    }
}

/* Calls one frame part as a JSR returning to ret. */
static void FrameCall(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2ExecutionResult (*part)(const Lufia2Memory *, Lufia2CpuState *),
    uint16_t ret) {
    SimulateJsrFrame(memory, cpu, ret);
    (void)part(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $81:E405: tile-set frame $10F1 at X + $8C0, $09F2 x $09F3. */
Lufia2ExecutionResult Lufia2BattleTileFrame(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadY16(cpu, 0x10f1u);
    PushDataBank(memory, cpu);
    SetBank7E(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x08c0u);
    TransferAToX(cpu);
    LoadA16(cpu, 0x10f0u);
    StoreWordAbsolute(memory, cpu, 0x09f6u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    StoreWordAbsolute(memory, cpu, FRAME_TILE, cpu->y);
    LoadAAbsolute8(memory, cpu, 0x09f2u, 0);
    DecrementA8(cpu);
    StoreAAbsolute8(memory, cpu, 0x09fau, 0);
    StoreZeroAbsolute8(memory, cpu, 0x09fbu, 0);
    LoadAAbsolute8(memory, cpu, 0x09f3u, 0);
    DecrementA8(cpu);
    DecrementA8(cpu);
    StoreAAbsolute8(memory, cpu, 0x09feu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x09ffu, 0);
    StoreWordAbsolute(memory, cpu, FRAME_ROW, cpu->x);
    LoadY16(cpu, cpu->x);
    SetAccumulatorWidth(cpu, 0);
    FrameCall(memory, cpu, Lufia2BattleFrameTop, 0xe442u);
    IncAbs16(memory, cpu, FRAME_TILE, 3);
    FrameCall(memory, cpu, Lufia2BattleFrameSides, 0xe44eu);
    IncAbs16(memory, cpu, FRAME_TILE, 1);
    while (!(DecAbs16(memory, cpu, 0x09feu) & 0x8000u))
        FrameCall(memory, cpu, Lufia2BattleFrameSides, 0xe456u);
    LoadA16(cpu, (uint16_t)((Abs16(memory, cpu, FRAME_TILE) | 0x8000u) - 1u));
    StoreWordAbsolute(memory, cpu, FRAME_TILE, cpu->accumulator);
    FrameCall(memory, cpu, Lufia2BattleFrameSides, 0xe468u);
    IncAbs16(memory, cpu, FRAME_TILE, -3);
    FrameCall(memory, cpu, Lufia2BattleFrameTop, 0xe474u);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81e478u);
}

/* $81:E3EC: inside X + $902 filled with $10F0, then $81:E405. */
Lufia2ExecutionResult Lufia2BattleTileWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushIndex(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    TransferXToA(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0902u);
    TransferAToX(cpu);
    LoadA16(cpu, 0x10f0u);
    StoreWordAbsolute(memory, cpu, 0x09f6u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    FrameCall(memory, cpu, Lufia2BattleFillRect, 0xe3ffu);
    cpu->x = PullIndexValue(memory, cpu);
    FrameCall(memory, cpu, Lufia2BattleTileFrame, 0xe403u);
    return ExecutionReturned(0x81e404u);
}

/* $81:E7D2: fill $09F2 x $09F3 words of $09F6 at $7E:X. */
Lufia2ExecutionResult Lufia2BattleFillRect(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t rows;

    PushDataBank(memory, cpu);
    SetBank7E(memory, cpu);
    StoreWordAbsolute(memory, cpu, 0x09f4u, cpu->x);
    LoadAAbsolute8(memory, cpu, 0x09f3u, 0);
    StoreAAbsolute8(memory, cpu, 0x09f9u, 0);
    do {
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x09f2u, 0) & 0x00ffu));
        TransferAToY(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09f6u, 0));
        do {
            StoreAAbsolute16(memory, cpu, 0x0000u, cpu->x);
            IncrementX16(cpu);
            IncrementX16(cpu);
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->zero);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09f4u, 0));
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        TransferAToX(cpu);
        StoreWordAbsolute(memory, cpu, 0x09f4u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        rows = (uint8_t)(AbsoluteByte(memory, cpu, 0x09f9u, 0) - 1u);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x09f9u, 0), rows);
        SetNz8(cpu, rows);
    } while (rows);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81e807u);
}
