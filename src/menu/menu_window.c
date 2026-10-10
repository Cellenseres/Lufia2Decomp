/* Menu window frames ($82:810E). */

#include "core/cpu_ops.h"
#include "lufia2/menu.h"

enum {
    TILEMAP = 0x7e2000u,
    POSITION = 0x54u,
    WIDTH = 0x56u,                      /* tiles */
    HEIGHT = 0x58u,
    PATTERNS = 0x827eu,                 /* bank $82 pointers */
};

typedef enum WindowEdge {
    EDGE_ROW,                           /* $82:81E6 */
    EDGE_CORNER,                        /* $82:820B, 2 rows */
    EDGE_COLUMN,                        /* $82:8230 */
    EDGE_CORNER3,                       /* $82:8259, 3 rows */
} WindowEdge;

static void TileStore(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
    uint16_t offset) {
    Write16Long(memory, LongIndexedAddress(TILEMAP + offset, cpu->x),
        cpu->accumulator);
}

static uint16_t Pattern(const Lufia2Memory *memory, const Lufia2CpuState *cpu) {
    return Read16Long(memory, (((uint32_t)cpu->data_bank << 16) +
        Read16Direct(memory, cpu, 0x5du) + cpu->y) & 0x00ffffffu);
}

/* Pattern Y of $82:827E drawn at tilemap offset A. */
static void WindowEdgeDraw(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    WindowEdge edge) {
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, PATTERNS, cpu->y));
    StoreXDirect16(memory, cpu, 0x5du);
    TransferAToX(cpu);
    LoadY16(cpu, 0x0000u);
    if (edge == EDGE_ROW || edge == EDGE_COLUMN) {
        LoadA16(cpu, Read16Direct(memory, cpu,
            edge == EDGE_ROW ? WIDTH : HEIGHT));
        Subtract16(cpu, 0x0004u);
        if (cpu->zero)
            return;
        StoreADirect16(memory, cpu, 0x5au);
        do {
            LoadA16(cpu, Pattern(memory, cpu));
            TileStore(memory, cpu, 0x0000u);
            if (edge == EDGE_ROW) {
                IncrementX16(cpu);
                IncrementX16(cpu);
            } else {
                TransferXToA(cpu);
                cpu->carry = 0;
                Add16Value(cpu, 0x0040u);
                TransferAToX(cpu);
            }
            LoadA16(cpu, (uint16_t)(cpu->y ^ 0x0002u));
            TransferAToY(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 0x5au), -1);
        } while (!cpu->zero);
        return;
    }
    do {
        LoadA16(cpu, Pattern(memory, cpu));
        TileStore(memory, cpu, 0x0000u);
        IncrementY16(cpu);
        IncrementY16(cpu);
        LoadA16(cpu, Pattern(memory, cpu));
        TileStore(memory, cpu, 0x0002u);
        IncrementY16(cpu);
        IncrementY16(cpu);
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        TransferAToX(cpu);
        Compare16(cpu, cpu->y, edge == EDGE_CORNER ? 0x0008u : 0x000cu);
    } while (!cpu->zero);
}

static void Edge(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    WindowEdge edge, uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    WindowEdgeDraw(memory, cpu, edge);
    SimulateRtsFrame(memory, cpu);
}

static uint16_t RowOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t rows_back) {
    LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, HEIGHT) - rows_back));
    ExchangeAccumulatorBytes(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    return cpu->accumulator;                                   /* rows * $40 */
}

/* $82:810E: window at A, X = width << 8 | height; M0X0. */
Lufia2ExecutionResult Lufia2MenuDrawWindow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StoreADirect16(memory, cpu, POSITION);
    Write16Direct(memory, cpu, WIDTH, 0);
    Write16Direct(memory, cpu, HEIGHT, 0);
    TransferXToA(cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, HEIGHT);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, WIDTH);
    SetAccumulatorWidth(cpu, 0);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x82u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, POSITION));         /* interior */
    cpu->carry = 0;
    Add16Value(cpu, 0x0042u);
    StoreADirect16(memory, cpu, 0x63u);
    LoadA16(cpu, (uint16_t)(Read16Direct(memory, cpu, HEIGHT) - 2u));
    StoreADirect16(memory, cpu, 0x5au);
    do {
        LoadY16(cpu, (uint16_t)(Read16Direct(memory, cpu, WIDTH) - 2u));
        LoadA16(cpu, 0x080fu);
        LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
        do {
            TileStore(memory, cpu, 0x0000u);
            IncrementX16(cpu);
            IncrementX16(cpu);
            cpu->y = (uint16_t)(cpu->y - 1u);
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x63u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0040u);
        StoreADirect16(memory, cpu, 0x63u);
        OpStepMem(memory, cpu, OpDp(cpu, 0x5au), -1);
    } while (!cpu->zero);
    LoadY16(cpu, 0x0000u);                                     /* top */
    LoadA16(cpu, Read16Direct(memory, cpu, POSITION));
    cpu->carry = 0;
    Add16Value(cpu, 0x0004u);
    StoreADirect16(memory, cpu, 0x63u);
    Edge(memory, cpu, EDGE_ROW, 0x8160u);
    LoadY16(cpu, 0x0002u);                                     /* bottom */
    {
        const uint16_t rows = RowOffset(memory, cpu, 1u);

        LoadA16(cpu, rows);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x63u));
    }
    Edge(memory, cpu, EDGE_ROW, 0x816fu);
    LoadA16(cpu, Read16Direct(memory, cpu, HEIGHT));
    Compare16(cpu, cpu->accumulator, 0x0003u);
    if (cpu->zero) {
        LoadY16(cpu, 0x0010u);                                 /* 81CC */
        LoadA16(cpu, Read16Direct(memory, cpu, POSITION));
        Edge(memory, cpu, EDGE_CORNER3, 0x81d3u);
        LoadY16(cpu, 0x0012u);
        LoadA16(cpu, Read16Direct(memory, cpu, WIDTH));
        AslA16(cpu);
        Subtract16(cpu, 0x0004u);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, POSITION));
        Edge(memory, cpu, EDGE_CORNER3, 0x81e3u);
    } else {
        LoadY16(cpu, 0x0004u);
        LoadA16(cpu, Read16Direct(memory, cpu, POSITION));
        Edge(memory, cpu, EDGE_CORNER, 0x817eu);
        LoadY16(cpu, 0x0006u);
        LoadA16(cpu, Read16Direct(memory, cpu, WIDTH));
        AslA16(cpu);
        Subtract16(cpu, 0x0004u);
        StoreADirect16(memory, cpu, 0x5au);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, POSITION));
        Edge(memory, cpu, EDGE_CORNER, 0x8190u);
        LoadY16(cpu, 0x0008u);
        {
            const uint16_t rows = RowOffset(memory, cpu, 2u);

            LoadA16(cpu, rows);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, POSITION));
            StoreADirect16(memory, cpu, 0x63u);
        }
        Edge(memory, cpu, EDGE_CORNER, 0x81a2u);
        LoadY16(cpu, 0x000au);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x5au));
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x63u));
        Edge(memory, cpu, EDGE_CORNER, 0x81adu);
        LoadY16(cpu, 0x000cu);                                 /* sides */
        LoadA16(cpu, Read16Direct(memory, cpu, POSITION));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x63u);
        Edge(memory, cpu, EDGE_COLUMN, 0x81bbu);
        LoadY16(cpu, 0x000eu);
        LoadA16(cpu, Read16Direct(memory, cpu, WIDTH));
        AslA16(cpu);
        cpu->carry = 0;
        Add16Value(cpu, Read16Direct(memory, cpu, 0x63u));
        LoadA16(cpu, (uint16_t)(cpu->accumulator - 2u));
        Edge(memory, cpu, EDGE_COLUMN, 0x81c9u);
    }
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x8281e5u);
}

Lufia2ExecutionResult Lufia2MenuDrawWindowTopBottomEdge(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8281e6u);
    WindowEdgeDraw(memory, cpu, EDGE_ROW);
    return ExecutionReturned(0x82820au);
}

Lufia2ExecutionResult Lufia2MenuDrawWindowCorner(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x82820bu);
    WindowEdgeDraw(memory, cpu, EDGE_CORNER);
    return ExecutionReturned(0x82822fu);
}

Lufia2ExecutionResult Lufia2MenuDrawWindowSideEdge(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x828230u);
    WindowEdgeDraw(memory, cpu, EDGE_COLUMN);
    return ExecutionReturned(0x828258u);
}

Lufia2ExecutionResult Lufia2MenuDrawWindowThreeRowCorner(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->accumulator_is_8_bit || cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, 0x828259u);
    WindowEdgeDraw(memory, cpu, EDGE_CORNER3);
    return ExecutionReturned(0x82827du);
}
