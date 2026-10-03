/* Walk along the edge of a region of the field cell map and mark it. */

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/field.h"

enum {
    MAP_WIDTH = 0x0005b9u,        /* cells per row, word */
    MAP_HEIGHT = 0x0005bbu,       /* rows, word */
    TRACE_MODE = 0x000692u,       /* 0: start inside the region */
    CELL_BASE = 0x7fd008u,        /* per-map table of cell pointers */
    MAP_SLOT = 0x0005aau,         /* selects the entry of that table */

    CELL_BANK = 0x7f0000u,
    EDGE_BITS = 0x3000u,          /* the two edge bits of a cell word */
    EDGE_INSIDE = 0x1000u,
    EDGE_OPEN = 0x2000u,
    STEP_BUDGET = 0x0200u,

    DP_COLUMN = 0x8fu,            /* start cell, as two bytes */
    DP_ROW = 0x91u,
    DP_ROW_STEP = 0x28u,          /* one row of cells, in bytes */
    DP_TWO_ROWS = 0x26u,
    DP_NEXT_COLUMN = 0x5au,       /* one row down, one cell right */
    DP_NEXT_COLUMN_2 = 0x22u,
    DP_TWO_ROWS_NEXT = 0x56u,
    DP_TWO_ROWS_NEXT_2 = 0x24u,
    DP_CORNER = 0x5du,
    DP_DIRECTION = 0x94u,

    /* Scratch slots of the fold pass, which reuses the same direct page. */
    FOLD_SEEN = 0x54u,            /* closed cells seen in a row */
    FOLD_SKIP_COPY = 0x56u,       /* nonzero: leave the first table alone */
    FOLD_CELLS_LEFT = 0x58u,
    FOLD_ROW_CELLS = 0x5au,
    FOLD_BELOW = 0x5du,           /* offset of the cell one row down */
    FOLD_BELOW_NEXT = 0x60u,
    FOLD_ROWS_LEFT = 0x63u,
    FOLD_STYLE = 0x65u,           /* edge bits given to cells not closed */

    /* The edge bits as seen in the high byte of a cell word. */
    EDGE_HIGH = 0x30u,
    EDGE_OPEN_HIGH = 0x20u
};

typedef enum {
    WALK_DOWN = 0,
    WALK_LEFT = 2,
    WALK_UP = 4,
    WALK_RIGHT = 6,
    WALK_FINISH = 8,            /* the budget ran out: fold the marks */
    WALK_EXIT = 9               /* nothing to trace: leave at once */
} WalkDirection;

typedef enum {
    FROM_MODE,
    FROM_SCAN_DOWN,
    FROM_SCAN_UP,
    FROM_DIRECTION,
    FROM_UP,
    FROM_DOWN,
    FROM_LEFT,
    FROM_RIGHT,
    FROM_FINISH
} TraceEntry;

typedef struct {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    uint16_t y;            /* byte offset of the current cell */
    uint16_t budget;       /* remaining steps */
    uint16_t row;          /* offsets of the neighbours, from the DP */
    uint16_t two_rows;
    uint16_t next_column;
    uint16_t next_column_2;
    uint16_t two_rows_next;
    uint16_t two_rows_next_2;
    uint16_t exit_a;       /* accumulator for the early exits */
} Trace;

static uint16_t CellWord(const Trace *t, uint16_t offset) {
    return Read16Long(t->memory,
        (CELL_BANK + offset + t->y) & 0x00ffffffu);
}

static void SetCellWord(const Trace *t, uint16_t offset, uint16_t value) {
    Write16Long(t->memory, (CELL_BANK + offset + t->y) & 0x00ffffffu, value);
}

static uint8_t CellByte(const Trace *t, uint16_t offset) {
    return Read8(t->memory, (CELL_BANK + offset + t->y) & 0x00ffffffu);
}

static void SetCellByte(const Trace *t, uint16_t offset, uint8_t value) {
    Write8(t->memory, (CELL_BANK + offset + t->y) & 0x00ffffffu, value);
}

static void MarkCell(const Trace *t, uint16_t offset) {
    SetCellWord(t, offset, (uint16_t)(CellWord(t, offset) | EDGE_BITS));
}

static void ClearCellEdge(const Trace *t, uint16_t offset) {
    SetCellWord(t, offset, (uint16_t)(CellWord(t, offset) & ~EDGE_BITS));
}

static void PushWord(const Trace *t, uint16_t value) {
    Push8(t->memory, t->cpu, (uint8_t)(value >> 8));
    Push8(t->memory, t->cpu, (uint8_t)value);
}

static uint16_t PullWord(const Trace *t) {
    const uint8_t low = Pull8(t->memory, t->cpu);

    return (uint16_t)(low | ((uint16_t)Pull8(t->memory, t->cpu) << 8));
}

/* INC/DEC of a direct page word writes the high byte first. */
static uint16_t AddToWord(const Trace *t, uint8_t offset, int delta) {
    const uint16_t value =
        (uint16_t)(Read16Direct(t->memory, t->cpu, offset) + delta);

    Write8(t->memory, DirectAddress(t->cpu, (uint8_t)(offset + 1u)),
        (uint8_t)(value >> 8));
    Write8(t->memory, DirectAddress(t->cpu, offset), (uint8_t)value);
    return value;
}

static WalkDirection Turn(const Trace *t, WalkDirection direction) {
    Write16Direct(t->memory, t->cpu, DP_DIRECTION, (uint16_t)direction);
    return direction;
}

/* The loop counter that ends the walk: some loops stop at zero, the
 * others when it turns negative. */
static int StepsLeftNonZero(Trace *t) {
    t->budget = (uint16_t)(t->budget - 1u);
    return t->budget != 0;
}

static int StepsLeftPositive(Trace *t) {
    t->budget = (uint16_t)(t->budget - 1u);
    return (t->budget & 0x8000u) == 0;
}

/* $80:FAC3: marks the cells around a bend, following the open cells down
 * the column. */
static void MarkBend(Trace *t) {
    PushWord(t, t->y);
    MarkCell(t, t->next_column_2);
    MarkCell(t, t->row);
    do {
        t->y = (uint16_t)(t->y + t->row);
        MarkCell(t, t->next_column);
        MarkCell(t, t->next_column_2);
        MarkCell(t, t->row);
    } while ((CellWord(t, t->two_rows_next) & EDGE_BITS) == EDGE_OPEN);
    MarkCell(t, t->two_rows_next_2);
    MarkCell(t, t->two_rows_next);
    MarkCell(t, t->two_rows);
    t->y = PullWord(t);
}

/* Calls the bend marker as a subroutine from the return address given. */
static void CallMarkBend(Trace *t, uint16_t return_address) {
    SimulateJsrFrame(t->memory, t->cpu, return_address);
    MarkBend(t);
    SimulateRtsFrame(t->memory, t->cpu);
}

/* $80:FA15: edge runs upwards. */
static WalkDirection WalkUp(Trace *t) {
    for (;;) {
        uint16_t cell = CellWord(t, t->row);
        uint16_t beside;

        if ((cell & EDGE_BITS) == 0) {
            t->y = (uint16_t)(t->y - 2u);
            return Turn(t, WALK_LEFT);
        }
        SetCellWord(t, t->row, (uint16_t)(cell | EDGE_BITS));
        beside = (uint16_t)(CellWord(t, 2u) & EDGE_BITS);
        if (beside == EDGE_OPEN) {
            PushWord(t, t->y);
            do {
                t->y = (uint16_t)(t->y - t->row);
                ClearCellEdge(t, t->next_column);
            } while ((CellWord(t, 2u) & EDGE_BITS) != 0);
            MarkCell(t, 2u);
            t->y = PullWord(t);
            continue;
        }
        if (beside & EDGE_INSIDE) {
            MarkCell(t, 0u);
            return Turn(t, WALK_RIGHT);
        }
        t->y = (uint16_t)(t->y - t->row);
        if (!StepsLeftNonZero(t))
            return WALK_FINISH;
    }
}

/* $80:FA79: edge runs downwards. */
static WalkDirection WalkDown(Trace *t) {
    for (;;) {
        const uint16_t cell = CellWord(t, t->next_column_2);
        uint16_t below;

        if ((cell & EDGE_BITS) == 0) {
            t->y = (uint16_t)(t->y + 2u);
            return Turn(t, WALK_RIGHT);
        }
        SetCellWord(t, t->next_column_2, (uint16_t)(cell | EDGE_BITS));
        below = (uint16_t)(CellWord(t, t->two_rows_next) & EDGE_BITS);
        if (below == EDGE_OPEN) {
            CallMarkBend(t, 0xfabau);
            return Turn(t, WALK_UP);
        }
        if (below & EDGE_INSIDE) {
            MarkCell(t, t->two_rows_next_2);
            return Turn(t, WALK_LEFT);
        }
        t->y = (uint16_t)(t->y + t->row);
        if (!StepsLeftPositive(t))
            return WALK_FINISH;
    }
}

/* $80:FB0D: edge runs to the left. */
static WalkDirection WalkLeft(Trace *t) {
    for (;;) {
        const uint16_t cell = CellWord(t, t->two_rows_next);

        if ((cell & EDGE_BITS) == 0) {
            t->y = (uint16_t)(t->y + t->row);
            return Turn(t, WALK_DOWN);
        }
        SetCellWord(t, t->two_rows_next, (uint16_t)(cell | EDGE_BITS));
        if (CellWord(t, t->row) & EDGE_INSIDE) {
            MarkCell(t, t->two_rows);
            return Turn(t, WALK_UP);
        }
        t->y = (uint16_t)(t->y - 2u);
        if (!StepsLeftPositive(t))
            return WALK_FINISH;
    }
}

/* $80:FB44: edge runs to the right. */
static WalkDirection WalkRight(Trace *t) {
    for (;;) {
        const uint16_t cell = CellWord(t, 2u);
        uint16_t beside;

        if ((cell & EDGE_BITS) == 0) {
            t->y = (uint16_t)(t->y - t->row);
            return Turn(t, WALK_UP);
        }
        SetCellWord(t, 2u, (uint16_t)(cell | EDGE_BITS));
        beside = (uint16_t)(CellWord(t, t->next_column_2) & EDGE_BITS);
        if (beside == EDGE_OPEN) {
            PushWord(t, t->y);
            t->y = (uint16_t)(t->y + 2u);
            while ((CellWord(t, 2u) & EDGE_BITS) == EDGE_OPEN)
                t->y = (uint16_t)(t->y - t->row);
            MarkCell(t, 2u);
            MarkCell(t, 0u);
            MarkCell(t, 4u);
            while ((CellWord(t, t->next_column) & EDGE_BITS) != 0) {
                ClearCellEdge(t, t->next_column);
                t->y = (uint16_t)(t->y + t->row);
            }
            t->y = PullWord(t);
            continue;
        }
        if (beside & EDGE_INSIDE) {
            MarkCell(t, 4u);
            return Turn(t, WALK_DOWN);
        }
        t->y = (uint16_t)(t->y + 2u);
        if (!StepsLeftPositive(t))
            return WALK_FINISH;
    }
}

/* $80:F933: rewrites the edge bits of the second cell table ($7F:D00A) and,
 * unless $7F:D020 is set, of the table at $7F:D008 as well. Returns the
 * accumulator at the exit and the final table pointer and cell offset. */
static uint8_t FoldMarks(const Trace *t, uint16_t *table, uint16_t *cell) {
    const Lufia2Memory *memory = t->memory;
    const Lufia2CpuState *cpu = t->cpu;
    const uint16_t width = Read16Long(memory, MAP_WIDTH);
    Trace row = *t;
    uint16_t x;
    uint8_t a;

    Write16Direct(memory, cpu, FOLD_ROW_CELLS, width);
    Write16Direct(memory, cpu, FOLD_CELLS_LEFT, width);
    Write16Direct(memory, cpu, FOLD_BELOW, (uint16_t)((width << 1) + 1u));
    Write16Direct(memory, cpu, FOLD_BELOW_NEXT, (uint16_t)((width << 1) + 3u));
    Write16Direct(memory, cpu, FOLD_ROWS_LEFT,
        (uint16_t)(Read16Long(memory, MAP_HEIGHT) - 1u));
    Write16Direct(memory, cpu, FOLD_SKIP_COPY,
        (uint16_t)(Read16Long(memory, CELL_BANK | 0xd020u) & 0x00ffu));
    x = Read16Long(memory, CELL_BANK | 0xd008u);
    row.y = Read16Long(memory, CELL_BANK | 0xd00au);

    /* First pass over one row: a cell that has both edge bits becomes open
     * (only bit 13), every other cell gets both bits. */
    a = 0;
    {
        uint8_t count = (uint8_t)width;

        do {
            const uint8_t cell_bits = CellByte(&row, 1u);

            if ((cell_bits & EDGE_HIGH) == EDGE_HIGH) {
                a = (uint8_t)((cell_bits & ~EDGE_HIGH) | EDGE_OPEN_HIGH);
                SetCellByte(&row, 1u, a);
                a = DirectByte(memory, cpu, FOLD_SKIP_COPY);
                if (a == 0) {
                    a = (uint8_t)((Read8(memory, CELL_BANK + 1u + x) & ~EDGE_HIGH) |
                        EDGE_OPEN_HIGH);
                    Write8(memory, CELL_BANK + 1u + x, a);
                }
            } else {
                a = (uint8_t)(cell_bits | EDGE_HIGH);
                SetCellByte(&row, 1u, a);
                a = DirectByte(memory, cpu, FOLD_SKIP_COPY);
                if (a == 0) {
                    a = (uint8_t)(Read8(memory, CELL_BANK + 1u + x) | EDGE_HIGH);
                    Write8(memory, CELL_BANK + 1u + x, a);
                }
            }
            x = (uint16_t)(x + 2u);
            row.y = (uint16_t)(row.y + 2u);
            --count;
            Write8(memory, DirectAddress(cpu, FOLD_CELLS_LEFT), count);
        } while (count != 0);
    }
    row.y = Read16Long(memory, CELL_BANK | 0xd00au);

    /* Second pass, row after row: each cell is rewritten from the cell
     * below it; $65 holds the style used where that cell is not closed. */
    for (;;) {
        uint8_t count;

        a = DirectByte(memory, cpu, FOLD_ROW_CELLS);
        count = a;
        Write8(memory, DirectAddress(cpu, FOLD_CELLS_LEFT), count);
        a = EDGE_HIGH;
        Write8(memory, DirectAddress(cpu, FOLD_STYLE), a);
        do {
            const uint16_t below = Read16Direct(memory, cpu, FOLD_BELOW);
            const uint16_t below_next = Read16Direct(memory, cpu, FOLD_BELOW_NEXT);

            a = (uint8_t)(CellByte(&row, below) & EDGE_HIGH);
            if (a == EDGE_HIGH) {
                int toggle = 1;

                a = (uint8_t)(CellByte(&row, 1u) & EDGE_HIGH);
                if (a != EDGE_OPEN_HIGH) {
                    toggle = 0;
                } else {
                    a = (uint8_t)(CellByte(&row, below_next) & EDGE_HIGH);
                    if (a == EDGE_HIGH) {
                        a = (uint8_t)(CellByte(&row, 3u) & EDGE_HIGH);
                        if (a == EDGE_OPEN_HIGH) {
                            const uint8_t seen =
                                (uint8_t)(DirectByte(memory, cpu, FOLD_SEEN) + 1u);

                            Write8(memory, DirectAddress(cpu, FOLD_SEEN), seen);
                            a = (uint8_t)(seen - 1u);
                            toggle = a == 0;
                        }
                    }
                }
                if (toggle) {
                    a = (uint8_t)(DirectByte(memory, cpu, FOLD_STYLE) ^ EDGE_HIGH);
                    Write8(memory, DirectAddress(cpu, FOLD_STYLE), a);
                }
                a = (uint8_t)((CellByte(&row, below) & ~EDGE_HIGH) | EDGE_OPEN_HIGH);
                SetCellByte(&row, below, a);
                a = DirectByte(memory, cpu, FOLD_SKIP_COPY);
                if (a == 0) {
                    a = (uint8_t)((Read8(memory, CELL_BANK + 1u + x) & ~EDGE_HIGH) |
                        EDGE_OPEN_HIGH);
                    Write8(memory, CELL_BANK + 1u + x, a);
                }
            } else {
                Write8(memory, DirectAddress(cpu, FOLD_SEEN), 0);
                a = (uint8_t)((CellByte(&row, below) & ~EDGE_HIGH) |
                    DirectByte(memory, cpu, FOLD_STYLE));
                SetCellByte(&row, below, a);
                a = DirectByte(memory, cpu, FOLD_SKIP_COPY);
                if (a == 0) {
                    a = (uint8_t)((Read8(memory, CELL_BANK + 1u + x) & ~EDGE_HIGH) |
                        DirectByte(memory, cpu, FOLD_STYLE));
                    Write8(memory, CELL_BANK + 1u + x, a);
                }
            }
            x = (uint16_t)(x + 2u);
            row.y = (uint16_t)(row.y + 2u);
            --count;
            Write8(memory, DirectAddress(cpu, FOLD_CELLS_LEFT), count);
        } while (count != 0);
        {
            const uint8_t rows_left =
                (uint8_t)(DirectByte(memory, cpu, FOLD_ROWS_LEFT) - 1u);

            Write8(memory, DirectAddress(cpu, FOLD_ROWS_LEFT), rows_left);
            if (rows_left == 0)
                break;
        }
    }
    *table = x;
    *cell = row.y;
    return a;
}

/* $80:F87B: the walk starts inside the region: go down until an edge cell
 * shows. */
static WalkDirection StartInside(Trace *t) {
    const Lufia2Memory *memory = t->memory;
    Lufia2CpuState *cpu = t->cpu;
    uint16_t edge;

    t->y = (uint16_t)(t->y + t->row);
    (void)AddToWord(t, DP_ROW, 1);
    for (;;) {
        uint16_t row_index;

        edge = (uint16_t)(CellWord(t, 0u) & EDGE_BITS);
        if (edge != 0)
            break;
        t->y = (uint16_t)(t->y + t->row);
        row_index = (uint16_t)(Read16Direct(memory, cpu, DP_ROW) + 1u);
        Write16Direct(memory, cpu, DP_ROW, row_index);
        if (row_index == Read16Long(memory, MAP_HEIGHT)) {
            t->exit_a = row_index;
            return WALK_EXIT;
        }
    }
    PushWord(t, edge);
    t->y = (uint16_t)(t->y - t->row - t->row - 2u);
    Write16Direct(memory, cpu, DP_CORNER, t->y);
    edge = PullWord(t);
    if (edge == EDGE_OPEN)
        CallMarkBend(t, 0xf8b4u);
    t->budget = STEP_BUDGET;
    return Turn(t, WALK_LEFT);
}

/* $80:F8BF: the walk starts on the edge: go up until the open edge, and
 * clear the marks passed on the way. */
static WalkDirection StartOnEdge(Trace *t) {
    uint16_t edge;

    (void)AddToWord(t, DP_ROW, 1);
    while ((CellWord(t, 0u) & EDGE_BITS) != 0) {
        t->y = (uint16_t)(t->y - t->row);
        (void)AddToWord(t, DP_ROW, -1);
    }
    for (;;) {
        edge = (uint16_t)(CellWord(t, 0u) & EDGE_BITS);
        if (edge == EDGE_OPEN) {
            int carry = 1;
            uint16_t cell;

            /* The subtraction below continues without setting the carry,
             * as the original does. */
            for (;;) {
                uint32_t step;

                cell = CellWord(t, 0u);
                if ((cell & EDGE_BITS) == 0)
                    break;
                SetCellWord(t, 0u, (uint16_t)(cell & ~EDGE_BITS));
                step = (uint32_t)t->row + (carry ? 0u : 1u);
                carry = t->y >= step;
                t->y = (uint16_t)(t->y - step);
            }
            SetCellWord(t, 0u, (uint16_t)(cell | EDGE_BITS));
            break;
        }
        if (edge & EDGE_INSIDE)
            break;
        t->y = (uint16_t)(t->y - t->row);
        if (AddToWord(t, DP_ROW, -1) == 0) {
            t->exit_a = t->y;
            return WALK_EXIT;
        }
    }
    t->y = (uint16_t)(t->y - 2u);
    Write16Direct(t->memory, t->cpu, DP_CORNER, t->y);
    t->budget = STEP_BUDGET;
    return Turn(t, WALK_RIGHT);
}

/* Runs the walk from one of its entry points and leaves through the
 * PLB/PLP/RTL tail of $80:F821. */
static Lufia2ExecutionResult RunTrace(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    TraceEntry entry) {
    Trace t;
    WalkDirection direction = WALK_FINISH;
    uint16_t exit_a;
    uint16_t exit_x;
    uint16_t exit_y;

    t.memory = memory;
    t.cpu = cpu;
    t.y = cpu->y;
    t.budget = cpu->x;
    t.row = Read16Direct(memory, cpu, DP_ROW_STEP);
    t.two_rows = Read16Direct(memory, cpu, DP_TWO_ROWS);
    t.next_column = Read16Direct(memory, cpu, DP_NEXT_COLUMN);
    t.next_column_2 = Read16Direct(memory, cpu, DP_NEXT_COLUMN_2);
    t.two_rows_next = Read16Direct(memory, cpu, DP_TWO_ROWS_NEXT);
    t.two_rows_next_2 = Read16Direct(memory, cpu, DP_TWO_ROWS_NEXT_2);
    t.exit_a = 0;

    switch (entry) {
    case FROM_MODE:
        SetAccumulatorWidth(cpu, 1);
        direction = Read8(memory, TRACE_MODE) == 0 ?
            StartInside(&t) : StartOnEdge(&t);
        break;
    case FROM_SCAN_DOWN:
        direction = StartInside(&t);
        break;
    case FROM_SCAN_UP:
        direction = StartOnEdge(&t);
        break;
    case FROM_DIRECTION:
        switch (Read16Direct(memory, cpu, DP_DIRECTION)) {
        case WALK_RIGHT: direction = WALK_RIGHT; break;
        case WALK_DOWN: direction = WALK_DOWN; break;
        case WALK_LEFT: direction = WALK_LEFT; break;
        default: direction = WALK_UP; break;
        }
        break;
    case FROM_UP: direction = WALK_UP; break;
    case FROM_DOWN: direction = WALK_DOWN; break;
    case FROM_LEFT: direction = WALK_LEFT; break;
    case FROM_RIGHT: direction = WALK_RIGHT; break;
    case FROM_FINISH: direction = WALK_FINISH; break;
    }
    while (direction != WALK_FINISH && direction != WALK_EXIT) {
        switch (direction) {
        case WALK_DOWN: direction = WalkDown(&t); break;
        case WALK_LEFT: direction = WalkLeft(&t); break;
        case WALK_UP: direction = WalkUp(&t); break;
        default: direction = WalkRight(&t); break;
        }
    }
    if (direction == WALK_EXIT) {
        exit_a = t.exit_a;
        exit_x = t.budget;
        exit_y = t.y;
    } else {
        exit_a = FoldMarks(&t, &exit_x, &exit_y);
    }
    cpu->accumulator = exit_a;
    cpu->x = exit_x;
    cpu->y = exit_y;
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80fa14u);
}

/* $80:F821: from the party's cell, trace the edge of the walkable region
 * in the cell map (cell words in bank $7F, edge bits 12 and 13) and fold
 * the result into the attribute bytes. Any width, JSL; the cell is picked
 * with $83:F9D0. */
Lufia2ExecutionResult Lufia2FieldTraceCellEdges(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;
    uint16_t width;
    unsigned slot;

    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    for (slot = 0x28u; slot-- > 0;) {
        const uint32_t address = AbsoluteIndexedAddress(cpu, 0x0736u, (uint16_t)slot);

        Write8(memory, address, (uint8_t)(Read8(memory, address) & 0xbfu));
    }
    SimulateJslFrame(memory, cpu, 0x80u, 0xf838u);
    result = Lufia2FieldUnpackAttributes(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtlFrame(memory, cpu);
    Write8(memory, DirectAddress(cpu, DP_COLUMN),
        AbsoluteByte(memory, cpu, 0x06bau, 0));
    Write8(memory, DirectAddress(cpu, 0x90u), 0);
    Write8(memory, DirectAddress(cpu, DP_ROW),
        AbsoluteByte(memory, cpu, 0x06e2u, 0));
    Write8(memory, DirectAddress(cpu, 0x92u), 0);
    SimulateJslFrame(memory, cpu, 0x80u, 0xf84bu);
    result = Lufia2FieldCellPointer(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtlFrame(memory, cpu);
    cpu->y = cpu->x;
    width = Read16AbsoluteIndexed(memory, cpu, 0x05b9u, 0);
    Write16Direct(memory, cpu, DP_ROW_STEP, (uint16_t)(width << 1));
    Write16Direct(memory, cpu, DP_TWO_ROWS, (uint16_t)(width << 2));
    Write16Direct(memory, cpu, DP_TWO_ROWS_NEXT, (uint16_t)((width << 2) + 2u));
    Write16Direct(memory, cpu, DP_TWO_ROWS_NEXT_2, (uint16_t)((width << 2) + 4u));
    Write16Direct(memory, cpu, DP_NEXT_COLUMN, (uint16_t)((width << 1) + 2u));
    Write16Direct(memory, cpu, DP_NEXT_COLUMN_2, (uint16_t)((width << 1) + 4u));
    OpSetDataBank(memory, cpu, 0x7fu);
    return RunTrace(memory, cpu, FROM_MODE);
}

/* $83:F9D0: pointer into the cell table for the cell whose column is at
 * $8F and row at $91: twice (column + row * width) plus the table entry of
 * the current map. M8/X16, JSL; result in X. */
Lufia2ExecutionResult Lufia2FieldCellPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x83f9d0u);
    SimulateJsrFrame(memory, cpu, 0xf9d2u);
    LoadA8(cpu, DirectByte(memory, cpu, DP_COLUMN));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, DP_ROW));
    SimulateJsrFrame(memory, cpu, 0xf9dbu);
    Write8(memory, SNES_WRMPYA, A8(cpu));
    LoadA8(cpu, Read8(memory, MAP_WIDTH));
    Write8(memory, SNES_WRMPYB, A8(cpu));
    LoadA8(cpu, 0);
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    OpAdcValue(cpu, Read16Long(memory, SNES_RDMPYL));
    AslA16(cpu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    PushIndex(memory, cpu);
    LoadA16(cpu, Read16Long(memory, MAP_SLOT));
    TransferAToX(cpu);
    PullAccumulator16(memory, cpu);
    cpu->carry = 0;
    OpAdcValue(cpu, Read16Long(memory, CELL_BASE + cpu->x));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x83f9d3u);
}
