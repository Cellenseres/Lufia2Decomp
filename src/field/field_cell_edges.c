/* Walk along the edge of a region of the field cell map and mark it. */

#include <stddef.h>

#include "core/cpu_internal.h"
#include "core/cpu_ops.h"
#include "core/plain_ops.h"
#include "core/snes_registers.h"
#include "lufia2/field.h"

enum {
    MAP_WIDTH = 0x0005b9u,        /* cells per row, word */
    MAP_HEIGHT = 0x0005bbu,       /* rows, word */
    TRACE_MODE = 0x000692u,       /* 0: start inside the region */
    CELL_BASE = 0x7fd008u,        /* per-map table of cell pointers */
    MAP_SLOT = 0x0005aau,         /* selects the entry of that table */
    PARTY_COLUMN = 0x0006bau,
    PARTY_ROW = 0x0006e2u,
    TABLE_WIDTH = 0x7fd010u,      /* per-layer sizes, bytes */
    TABLE_HEIGHT = 0x7fd018u,
    PACKED_EDGES = 0x7fc000u,     /* edge bits as $80:ED0E restores them */

    CELL_BANK = 0x7f0000u,
    EDGE_BITS = 0x3000u,          /* the two edge bits of a cell word */
    EDGE_INSIDE = 0x1000u,
    EDGE_OPEN = 0x2000u,
    STEP_BUDGET = 0x0200u,

    CELL_POINTER_STACK_MIN = 0x1f04u,
    CELL_POINTER_STACK_MAX = 0x1ffcu,

    /* Entry S keeping both children in their stack bands. */
    TRACE_STACK_MIN = 0x1f09u,
    TRACE_STACK_MAX = 0x1ffcu,
    /* Preflight window: two rows above and below the table. */
    WINDOW_MARGIN_ROWS = 2u,
    WINDOW_CELLS = 0x4000u,
    WINDOW_END = 0xc000u,         /* packed edges and tables lie above */
    /* Conservative replay limit; longer paths stay in the original code. */
    IDLE_LIMIT = 11u,

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
    WALK_EXIT = 9,              /* nothing to trace: leave at once */
    WALK_REJECT = 10            /* preflight only: run the original */
} WalkDirection;

/* Edge bits around the table, two per cell. */
typedef struct {
    uint32_t low;               /* first byte address in bank $7F */
    uint32_t high;              /* one past the last */
    uint8_t bits[WINDOW_CELLS / 4u];
} EdgeWindow;

typedef struct {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    EdgeWindow *window;    /* set: replay on edge bits, no writes */
    uint16_t width;
    uint16_t height;
    uint16_t y;            /* byte offset of the current cell */
    uint16_t row;          /* $91 */
    uint16_t budget;       /* remaining steps */
    uint16_t exit_a;       /* accumulator for the early exits */
    unsigned idle;         /* transitions since the last step */
    bool rejected;
} Trace;

/* Cell pointers at $22-$5A; constant during the walk. */
static uint16_t TraceOffset(const Trace *trace, uint8_t field) {
    const uint16_t row = (uint16_t)(trace->width << 1);

    switch (field) {
    case DP_ROW_STEP: return row;
    case DP_NEXT_COLUMN: return (uint16_t)(row + 2u);
    case DP_NEXT_COLUMN_2: return (uint16_t)(row + 4u);
    case DP_TWO_ROWS: return (uint16_t)(row << 1);
    case DP_TWO_ROWS_NEXT: return (uint16_t)((row << 1) + 2u);
    default: return (uint16_t)((row << 1) + 4u);
    }
}

/* Binary: the preflight rejects decimal mode. */
static uint16_t StepRow(const Trace *trace, uint16_t cell, bool down) {
    const uint16_t stride = TraceOffset(trace, DP_ROW_STEP);

    return down ? (uint16_t)(cell + stride) : (uint16_t)(cell - stride);
}

static uint32_t WindowCell(Trace *t, uint32_t address) {
    const EdgeWindow *window = t->window;

    if (address < window->low || address + 2u > window->high ||
        ((address - window->low) & 1u) != 0) {
        t->rejected = true;
        return WINDOW_CELLS;
    }
    return (address - window->low) >> 1;
}

static uint16_t CellWord(Trace *t, uint16_t offset) {
    const uint32_t address = (uint32_t)offset + t->y;
    uint32_t cell;

    if (t->window == NULL)
        return Read16Long(t->memory, (CELL_BANK + address) & 0x00ffffffu);
    cell = WindowCell(t, address);
    if (cell == WINDOW_CELLS)
        return 0;
    return (uint16_t)(((t->window->bits[cell >> 2] >> ((cell & 3u) << 1)) & 3u) << 12);
}

static void SetCellWord(Trace *t, uint16_t offset, uint16_t value) {
    const uint32_t address = (uint32_t)offset + t->y;
    uint32_t cell;
    uint8_t *bits;

    if (t->window == NULL) {
        Write16Long(t->memory, (CELL_BANK + address) & 0x00ffffffu, value);
        return;
    }
    cell = WindowCell(t, address);
    if (cell == WINDOW_CELLS)
        return;
    bits = &t->window->bits[cell >> 2];
    *bits = (uint8_t)((*bits & ~(3u << ((cell & 3u) << 1))) |
        (((value >> 12) & 3u) << ((cell & 3u) << 1)));
}

static uint8_t CellByte(const Trace *t, uint16_t offset) {
    return Read8(t->memory, (CELL_BANK + offset + t->y) & 0x00ffffffu);
}

static void SetCellByte(const Trace *t, uint16_t offset, uint8_t value) {
    Write8(t->memory, (CELL_BANK + offset + t->y) & 0x00ffffffu, value);
}

static void MarkCell(Trace *t, uint16_t offset) {
    SetCellWord(t, offset, (uint16_t)(CellWord(t, offset) | EDGE_BITS));
}

static void ClearCellEdge(Trace *t, uint16_t offset) {
    SetCellWord(t, offset, (uint16_t)(CellWord(t, offset) & ~EDGE_BITS));
}

/* PHA/PHY; the replay keeps the value instead. */
static void SaveWord(const Trace *t, uint16_t value) {
    if (t->window != NULL)
        return;
    Push8(t->memory, t->cpu, (uint8_t)(value >> 8));
    Push8(t->memory, t->cpu, (uint8_t)value);
}

static uint16_t RestoreWord(const Trace *t, uint16_t saved) {
    uint8_t low;

    if (t->window != NULL)
        return saved;
    low = Pull8(t->memory, t->cpu);
    return (uint16_t)(low | ((uint16_t)Pull8(t->memory, t->cpu) << 8));
}

/* INC/DEC $91: the high byte is written first. */
static uint16_t AddToRow(Trace *t, int delta) {
    t->row = (uint16_t)(t->row + delta);
    if (t->window == NULL) {
        Write8(t->memory, DirectAddress(t->cpu, (uint8_t)(DP_ROW + 1u)),
            (uint8_t)(t->row >> 8));
        Write8(t->memory, DirectAddress(t->cpu, DP_ROW), (uint8_t)t->row);
    }
    return t->row;
}

static void SetCorner(const Trace *t) {
    if (t->window == NULL)
        Write16Direct(t->memory, t->cpu, DP_CORNER, t->y);
}

static WalkDirection Turn(const Trace *t, WalkDirection direction) {
    if (t->window == NULL)
        Write16Direct(t->memory, t->cpu, DP_DIRECTION, (uint16_t)direction);
    return direction;
}

/* A turn or detour; beyond the limit it loops forever. */
static bool Idle(Trace *t) {
    return ++t->idle <= IDLE_LIMIT || t->window == NULL;
}

/* The loop counter that ends the walk: some loops stop at zero, the
 * others when it turns negative. */
static int StepsLeftNonZero(Trace *t) {
    t->idle = 0;
    t->budget = (uint16_t)(t->budget - 1u);
    return t->budget != 0;
}

static int StepsLeftPositive(Trace *t) {
    t->idle = 0;
    t->budget = (uint16_t)(t->budget - 1u);
    return (t->budget & 0x8000u) == 0;
}

/* $80:FAC3: marks the cells around a bend, following the open cells down
 * the column. */
static void MarkBend(Trace *t) {
    const uint16_t saved = t->y;

    SaveWord(t, saved);
    MarkCell(t, TraceOffset(t, DP_NEXT_COLUMN_2));
    MarkCell(t, TraceOffset(t, DP_ROW_STEP));
    do {
        t->y = StepRow(t, t->y, true);
        MarkCell(t, TraceOffset(t, DP_NEXT_COLUMN));
        MarkCell(t, TraceOffset(t, DP_NEXT_COLUMN_2));
        MarkCell(t, TraceOffset(t, DP_ROW_STEP));
    } while ((CellWord(t, TraceOffset(t, DP_TWO_ROWS_NEXT)) & EDGE_BITS) == EDGE_OPEN);
    MarkCell(t, TraceOffset(t, DP_TWO_ROWS_NEXT_2));
    MarkCell(t, TraceOffset(t, DP_TWO_ROWS_NEXT));
    MarkCell(t, TraceOffset(t, DP_TWO_ROWS));
    t->y = RestoreWord(t, saved);
}

/* Calls the bend marker as a subroutine from the return address given. */
static void CallMarkBend(Trace *t, uint16_t return_address) {
    if (t->window == NULL)
        SimulateJsrFrame(t->memory, t->cpu, return_address);
    MarkBend(t);
    if (t->window == NULL)
        SimulateRtsFrame(t->memory, t->cpu);
}

/* $80:FA15: edge runs upwards. */
static WalkDirection WalkUp(Trace *t) {
    for (;;) {
        uint16_t cell = CellWord(t, TraceOffset(t, DP_ROW_STEP));
        uint16_t beside;

        if ((cell & EDGE_BITS) == 0) {
            t->y = (uint16_t)(t->y - 2u);
            return Turn(t, WALK_LEFT);
        }
        SetCellWord(t, TraceOffset(t, DP_ROW_STEP), (uint16_t)(cell | EDGE_BITS));
        beside = (uint16_t)(CellWord(t, 2u) & EDGE_BITS);
        if (beside == EDGE_OPEN) {
            const uint16_t saved = t->y;

            if (!Idle(t))
                return WALK_REJECT;
            SaveWord(t, saved);
            do {
                t->y = StepRow(t, t->y, false);
                ClearCellEdge(t, TraceOffset(t, DP_NEXT_COLUMN));
            } while ((CellWord(t, 2u) & EDGE_BITS) != 0);
            MarkCell(t, 2u);
            t->y = RestoreWord(t, saved);
            continue;
        }
        if (beside & EDGE_INSIDE) {
            MarkCell(t, 0u);
            return Turn(t, WALK_RIGHT);
        }
        t->y = StepRow(t, t->y, false);
        if (!StepsLeftNonZero(t))
            return WALK_FINISH;
    }
}

/* $80:FA79: edge runs downwards. */
static WalkDirection WalkDown(Trace *t) {
    for (;;) {
        const uint16_t cell = CellWord(t, TraceOffset(t, DP_NEXT_COLUMN_2));
        uint16_t below;

        if ((cell & EDGE_BITS) == 0) {
            t->y = (uint16_t)(t->y + 2u);
            return Turn(t, WALK_RIGHT);
        }
        SetCellWord(t, TraceOffset(t, DP_NEXT_COLUMN_2), (uint16_t)(cell | EDGE_BITS));
        below = (uint16_t)(CellWord(t, TraceOffset(t, DP_TWO_ROWS_NEXT)) & EDGE_BITS);
        if (below == EDGE_OPEN) {
            CallMarkBend(t, 0xfabau);
            return Turn(t, WALK_UP);
        }
        if (below & EDGE_INSIDE) {
            MarkCell(t, TraceOffset(t, DP_TWO_ROWS_NEXT_2));
            return Turn(t, WALK_LEFT);
        }
        t->y = StepRow(t, t->y, true);
        if (!StepsLeftPositive(t))
            return WALK_FINISH;
    }
}

/* $80:FB0D: edge runs to the left. */
static WalkDirection WalkLeft(Trace *t) {
    for (;;) {
        const uint16_t cell = CellWord(t, TraceOffset(t, DP_TWO_ROWS_NEXT));

        if ((cell & EDGE_BITS) == 0) {
            t->y = StepRow(t, t->y, true);
            return Turn(t, WALK_DOWN);
        }
        SetCellWord(t, TraceOffset(t, DP_TWO_ROWS_NEXT), (uint16_t)(cell | EDGE_BITS));
        if (CellWord(t, TraceOffset(t, DP_ROW_STEP)) & EDGE_INSIDE) {
            MarkCell(t, TraceOffset(t, DP_TWO_ROWS));
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
            t->y = StepRow(t, t->y, false);
            return Turn(t, WALK_UP);
        }
        SetCellWord(t, 2u, (uint16_t)(cell | EDGE_BITS));
        beside = (uint16_t)(CellWord(t, TraceOffset(t, DP_NEXT_COLUMN_2)) & EDGE_BITS);
        if (beside == EDGE_OPEN) {
            const uint16_t saved = t->y;

            if (!Idle(t))
                return WALK_REJECT;
            SaveWord(t, saved);
            t->y = (uint16_t)(t->y + 2u);
            while ((CellWord(t, 2u) & EDGE_BITS) == EDGE_OPEN)
                t->y = StepRow(t, t->y, false);
            MarkCell(t, 2u);
            MarkCell(t, 0u);
            MarkCell(t, 4u);
            for (;;) {
                const uint16_t marked = CellWord(t, TraceOffset(t, DP_NEXT_COLUMN));

                if ((marked & EDGE_BITS) == 0)
                    break;
                SetCellWord(t, TraceOffset(t, DP_NEXT_COLUMN), (uint16_t)(marked & ~EDGE_BITS));
                t->y = StepRow(t, t->y, true);
            }
            t->y = RestoreWord(t, saved);
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
            count = (uint8_t)(DirectByte(memory, cpu, FOLD_CELLS_LEFT) - 1u);
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
            count = (uint8_t)(DirectByte(memory, cpu, FOLD_CELLS_LEFT) - 1u);
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
    uint16_t edge;

    t->y = StepRow(t, t->y, true);
    (void)AddToRow(t, 1);
    for (;;) {
        edge = (uint16_t)(CellWord(t, 0u) & EDGE_BITS);
        if (t->rejected)
            return WALK_REJECT;
        if (edge != 0)
            break;
        t->y = StepRow(t, t->y, true);
        /* LDA/INC/STA: the low byte is written first. */
        t->row = (uint16_t)(t->row + 1u);
        if (t->window == NULL)
            Write16Direct(t->memory, t->cpu, DP_ROW, t->row);
        if (t->row == t->height) {
            t->exit_a = t->row;
            return WALK_EXIT;
        }
    }
    SaveWord(t, edge);
    t->y = (uint16_t)(StepRow(t, StepRow(t, t->y, false), false) - 2u);
    SetCorner(t);
    edge = RestoreWord(t, edge);
    if (edge == EDGE_OPEN)
        CallMarkBend(t, 0xf8b4u);
    t->budget = STEP_BUDGET;
    return Turn(t, WALK_LEFT);
}

/* $80:F8BF: the walk starts on the edge: go up until the open edge, and
 * clear the marks passed on the way. */
static WalkDirection StartOnEdge(Trace *t) {
    uint16_t edge;

    (void)AddToRow(t, 1);
    while ((CellWord(t, 0u) & EDGE_BITS) != 0) {
        t->y = StepRow(t, t->y, false);
        (void)AddToRow(t, -1);
    }
    for (;;) {
        edge = (uint16_t)(CellWord(t, 0u) & EDGE_BITS);
        if (t->rejected)
            return WALK_REJECT;
        if (edge == EDGE_OPEN) {
            uint16_t cell;

            /* SBC without SEC: the carry stays set inside the window. */
            for (;;) {
                cell = CellWord(t, 0u);
                if ((cell & EDGE_BITS) == 0)
                    break;
                SetCellWord(t, 0u, (uint16_t)(cell & ~EDGE_BITS));
                t->y = StepRow(t, t->y, false);
            }
            SetCellWord(t, 0u, (uint16_t)(cell | EDGE_BITS));
            break;
        }
        if (edge & EDGE_INSIDE)
            break;
        t->y = StepRow(t, t->y, false);
        if (AddToRow(t, -1) == 0) {
            t->exit_a = t->y;
            return WALK_EXIT;
        }
    }
    t->y = (uint16_t)(t->y - 2u);
    SetCorner(t);
    t->budget = STEP_BUDGET;
    return Turn(t, WALK_RIGHT);
}

/* From the mode at $0692 to the end of the walk. */
static WalkDirection Walk(Trace *t) {
    WalkDirection direction = Read8(t->memory, TRACE_MODE) == 0 ?
        StartInside(t) : StartOnEdge(t);

    if (t->rejected)
        return WALK_REJECT;
    while (direction < WALK_FINISH) {
        switch (direction) {
        case WALK_DOWN: direction = WalkDown(t); break;
        case WALK_LEFT: direction = WalkLeft(t); break;
        case WALK_UP: direction = WalkUp(t); break;
        default: direction = WalkRight(t); break;
        }
        if (t->rejected)
            return WALK_REJECT;
        if (direction < WALK_FINISH && !Idle(t))
            return WALK_REJECT;
    }
    return direction;
}

static bool LowWramDataBank(uint8_t bank) {
    return bank < 0x40u || (bank >= 0x80u && bank < 0xc0u) || bank == 0x7eu;
}

/* Read-only preflight: entry domain, then the walk on edge bits. */
static bool TraceReady(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    EdgeWindow window;
    Trace t;
    uint8_t layout;
    uint8_t column;
    uint8_t row;
    uint16_t width;
    uint16_t height;
    uint16_t table;
    uint32_t cells;
    uint32_t packed_end;
    uint32_t cell;
    WalkDirection direction;

    if (cpu->program_bank != 0x80u || cpu->direct_page != 0u || cpu->decimal ||
        cpu->stack < TRACE_STACK_MIN || cpu->stack > TRACE_STACK_MAX ||
        !LowWramDataBank(cpu->data_bank))
        return false;
    layout = Read8(memory, MAP_SLOT);
    if ((layout & 0xf9u) != 0 || Read8(memory, MAP_SLOT + 1u) != 0)
        return false;
    width = Read16Long(memory, MAP_WIDTH);
    height = Read16Long(memory, MAP_HEIGHT);
    if (width < 3u || width > 0xffu || height < 2u || height > 0xffu ||
        Read8(memory, TABLE_WIDTH + layout) != width ||
        Read8(memory, TABLE_HEIGHT + layout) != height)
        return false;
    column = Read8(memory, PARTY_COLUMN);
    row = Read8(memory, PARTY_ROW);
    if (column >= width || row >= height)
        return false;

    /* Walk below the packed edges, fold inside bank $7F. */
    cells = (uint32_t)width * height;
    table = Read16Long(memory, CELL_BASE + layout);
    /* The first table starts near $7F:0000; clip the margin there. */
    window.low = table > WINDOW_MARGIN_ROWS * 2u * width ?
        (uint32_t)table - WINDOW_MARGIN_ROWS * 2u * width : 0u;
    window.high = (uint32_t)table + 2u * width * (height + WINDOW_MARGIN_ROWS);
    if ((table & 1u) != 0 || window.high > WINDOW_END ||
        ((window.high - window.low) >> 1) > WINDOW_CELLS ||
        Read16Long(memory, CELL_BANK | 0xd008u) + 2u * cells > WINDOW_END ||
        Read16Long(memory, CELL_BANK | 0xd00au) + 2u * cells + 2u > WINDOW_END)
        return false;

    /* Edge bits as they stand after $80:ED0E. */
    packed_end = table + 8u * ((cells + 3u) >> 2);
    for (cell = 0; cell < (window.high - window.low) >> 1; ++cell) {
        const uint32_t address = window.low + 2u * cell;
        uint8_t edge;

        if (address >= table && address < packed_end) {
            const uint32_t index = (address - table) >> 1;

            edge = (uint8_t)((Read8(memory, PACKED_EDGES + (index >> 2)) >>
                ((index & 3u) << 1)) & 3u);
        } else {
            edge = (uint8_t)((Read8(memory, CELL_BANK + address + 1u) >> 4) & 3u);
        }
        if ((cell & 3u) == 0)
            window.bits[cell >> 2] = 0;
        window.bits[cell >> 2] = (uint8_t)(window.bits[cell >> 2] |
            (edge << ((cell & 3u) << 1)));
    }

    t.memory = memory;
    t.cpu = cpu;
    t.window = &window;
    t.width = width;
    t.height = height;
    /* $83:F9D0 with the 8-bit hardware product. */
    t.y = (uint16_t)(table + (uint16_t)(((uint16_t)(row * width) + column) << 1));
    t.row = row;
    t.budget = 0;
    t.exit_a = 0;
    t.idle = 0;
    t.rejected = false;
    direction = Walk(&t);
    return direction == WALK_FINISH || direction == WALK_EXIT;
}

/* The walk, then the PLB/PLP/RTL tail of $80:F821. */
static Lufia2ExecutionResult RunTrace(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Trace t;
    WalkDirection direction;
    uint16_t exit_a;
    uint16_t exit_x;
    uint16_t exit_y;

    t.memory = memory;
    t.cpu = cpu;
    t.window = NULL;
    t.width = Read16Long(memory, MAP_WIDTH);
    t.height = Read16Long(memory, MAP_HEIGHT);
    t.y = cpu->y;
    t.row = Read16Direct(memory, cpu, DP_ROW);
    t.budget = cpu->x;
    t.exit_a = 0;
    t.idle = 0;
    t.rejected = false;

    SetAccumulatorWidth(cpu, 1);
    direction = Walk(&t);
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

    if (!TraceReady(memory, cpu))
        return ExecutionHandoff(cpu, 0x80f821u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, 0x27u);
    for (slot = 0x28u; slot-- > 0;) {
        const uint32_t address = AbsoluteIndexedAddress(cpu, 0x0736u, cpu->x);

        LoadA8(cpu, (uint8_t)(Read8(memory, address) & 0xbfu));
        Write8(memory, address, A8(cpu));
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    }
    SimulateJslFrame(memory, cpu, 0x80u, 0xf838u);
    result = Lufia2FieldUnpackAttributes(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, AbsoluteByte(memory, cpu, 0x06bau, 0));
    Write8(memory, DirectAddress(cpu, DP_COLUMN), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x90u), 0);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, AbsoluteByte(memory, cpu, 0x06e2u, 0));
    Write8(memory, DirectAddress(cpu, DP_ROW), A8(cpu));
    Write8(memory, DirectAddress(cpu, 0x92u), 0);
    SimulateJslFrame(memory, cpu, 0x80u, 0xf84bu);
    cpu->program_bank = 0x83u;
    result = Lufia2FieldCellPointer(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtlFrame(memory, cpu);
    cpu->program_bank = 0x80u;
    cpu->y = cpu->x;
    width = Read16AbsoluteIndexed(memory, cpu, 0x05b9u, 0);
    Write16Direct(memory, cpu, DP_ROW_STEP, (uint16_t)(width << 1));
    Write16Direct(memory, cpu, DP_TWO_ROWS, (uint16_t)(width << 2));
    Write16Direct(memory, cpu, DP_TWO_ROWS_NEXT, (uint16_t)((width << 2) + 2u));
    Write16Direct(memory, cpu, DP_TWO_ROWS_NEXT_2, (uint16_t)((width << 2) + 4u));
    Write16Direct(memory, cpu, DP_NEXT_COLUMN, (uint16_t)((width << 1) + 2u));
    Write16Direct(memory, cpu, DP_NEXT_COLUMN_2, (uint16_t)((width << 1) + 4u));
    OpSetDataBank(memory, cpu, 0x7fu);
    return RunTrace(memory, cpu);
}

/* $83:F9D0: pointer into the cell table for the cell whose column is at
 * $8F and row at $91: twice (column + row * width) plus the table entry of
 * the current map. M8/X16, JSL; result in X. */
Lufia2ExecutionResult Lufia2FieldCellPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint8_t column;
    uint8_t row;
    uint16_t relative;
    uint16_t map_slot;
    Word16Result address;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->stack < CELL_POINTER_STACK_MIN || cpu->stack > CELL_POINTER_STACK_MAX)
        return ExecutionHandoff(cpu, 0x83f9d0u);
    SimulateJsrFrame(memory, cpu, 0xf9d2u);
    column = DirectByte(memory, cpu, DP_COLUMN);
    row = DirectByte(memory, cpu, DP_ROW);
    SimulateJsrFrame(memory, cpu, 0xf9dbu);
    Write8(memory, SNES_WRMPYA, row);
    Write8(memory, SNES_WRMPYB, Read8(memory, MAP_WIDTH));
    address = Sum16Mode(column, Read16Long(memory, SNES_RDMPYL), false,
        cpu->decimal);
    relative = (uint16_t)(address.value << 1);
    SimulateRtsFrame(memory, cpu);

    /* The original saves the relative offset while selecting the map. */
    PushStackWord(memory, cpu, relative);
    map_slot = Read16Long(memory, MAP_SLOT);
    relative = PullStackWord(memory, cpu);
    address = Sum16Mode(relative, Read16Long(memory, CELL_BASE + map_slot),
        false, cpu->decimal);
    cpu->x = address.value;
    LeaveSum(cpu, address);
    SimulateRtsFrame(memory, cpu);
    return ExecutionReturned(0x83f9d3u);
}
