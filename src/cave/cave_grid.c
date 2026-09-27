/* Ancient Cave room grid helpers ($83:9B18-$83:9DE3).
 *
 * The floor is a 16x16 cell grid at $7F:EA00 (row = high nibble); each cell
 * holds a room id, bit 7 marks a visited cell and bit 6 a corridor. $7F:EB00
 * holds the matching block shapes. All routines run with DB = $7F.
 */

#include "cave/cave_internal.h"

#define GRID 0xea00u

/* $83:9B18: clear the visited and corridor bits of every cell. */
void Lufia2CaveClearVisited(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpLdx(cpu, 0x00ffu);                                     /* 9B18 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, GRID));
        OpAndValue(cpu, 0x3fu);
        OpSta(memory, cpu, OpAbsX(cpu, GRID));
        OpDex(cpu);
    } while (!cpu->negative);                                  /* 9B24 */
    SimulateRtsFrame(memory, cpu);
}

/* $83:9B27: $E6AF = number of non-empty cells. */
void Lufia2CaveCountCells(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpLdx(cpu, 0x0000u);                                     /* 9B27 */
    OpStz(memory, cpu, OpAbs(cpu, 0xe6afu));
    do {
        OpLda(memory, cpu, OpAbsX(cpu, GRID));             /* 9B2D */
        if (!cpu->zero)
            OpStepMem(memory, cpu, OpAbs(cpu, 0xe6afu), 1);
        OpInx(cpu);
        OpCpx(cpu, 0x0100u);
    } while (!cpu->carry);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9B3C: A += 3, B += 2 (the stair tile beside a cell origin). */
void Lufia2CaveStairOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpIncA(cpu);                                             /* 9B3C */
    OpIncA(cpu);
    OpIncA(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    ExchangeAccumulatorBytes(cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9B48: cell A to tile origin: B = 6 * column, A = 6 * (row - 1). */
void Lufia2CaveCellPosition(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpSta(memory, cpu, OpDp(cpu, 0x54u));                  /* 9B48 */
    OpAndValue(cpu, 0x0fu);
    AslA8(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x55u));
    AslA8(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x55u));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x54u));
    OpAndValue(cpu, 0xf0u);
    cpu->carry = 1;
    Sbc8(cpu, 0x10u);
    LsrA8(cpu);
    LsrA8(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x55u));
    LsrA8(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x55u));
    SimulateRtsFrame(memory, cpu);                             /* 9B61 */
}

/* Push cell (A) onto the $7F:0000,Y worklist. */
static void CavePushCell(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbsY(cpu, 0x0000u));
    OpIny(cpu);
}

/* Neighbour at `offset` joins the room when it holds the same id. */
static uint8_t CaveSameNeighbour(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t offset) {
    OpLda(memory, cpu, OpAbsX(cpu, offset));
    if (cpu->zero || cpu->negative)
        return 0;
    OpCmp(memory, cpu, OpDp(cpu, 0x54u));
    return cpu->zero;
}

/* $83:9B62: walk from the start cell $E6A9 through open neighbours; each
   vertical step into a different room queues the link at $E6F1. */
void Lufia2CaveLinkRooms(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpLda(memory, cpu, OpAbs(cpu, 0xe6a9u));               /* 9B62 */
    OpSta(memory, cpu, OpAbs(cpu, 0x0000u));
    OpLdy(cpu, 0x0001u);
    OpStz(memory, cpu, OpDp(cpu, 0x2du));
    OpStz(memory, cpu, OpDp(cpu, 0x2eu));
    for (;;) {
        OpDey(cpu);                                          /* 9B6F */
        if (cpu->negative)
            break;
        TransferDirectToA(cpu);                                /* 9B73 */
        OpLda(memory, cpu, OpAbsY(cpu, 0x0000u));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, GRID));
        if (cpu->negative)
            continue;
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        OpLda(memory, cpu, OpAbsX(cpu, 0xe9f0u));          /* 9B7F */
        if (!cpu->zero && !cpu->negative) {
            OpCmp(memory, cpu, OpDp(cpu, 0x54u));
            if (!cpu->zero) {
                OpTxa(cpu);
                cpu->carry = 1;
                OpSbcValue(cpu, 0x10u);
                Lufia2CaveQueueLink(memory, cpu, 0x9b8eu);
            }
            OpTxa(cpu);                                      /* 9B91 */
            cpu->carry = 1;
            OpSbcValue(cpu, 0x10u);
            CavePushCell(memory, cpu);
        }
        OpLda(memory, cpu, OpAbsX(cpu, 0xea10u));          /* 9B99 */
        if (!cpu->zero && !cpu->negative) {
            OpCmp(memory, cpu, OpDp(cpu, 0x54u));
            if (!cpu->zero) {
                OpTxa(cpu);
                Lufia2CaveQueueLink(memory, cpu, 0x9ba5u);
            }
            OpTxa(cpu);                                      /* 9BA8 */
            cpu->carry = 0;
            OpAdcValue(cpu, 0x10u);
            CavePushCell(memory, cpu);
        }
        if (CaveSameNeighbour(memory, cpu, 0xea01u)) {         /* 9BB0 */
            OpTxa(cpu);
            OpIncA(cpu);
            CavePushCell(memory, cpu);
        }
        if (CaveSameNeighbour(memory, cpu, 0xe9ffu)) {         /* 9BC1 */
            OpTxa(cpu);
            OpDecA(cpu);
            CavePushCell(memory, cpu);
        }
        OpLda(memory, cpu, OpAbsX(cpu, GRID));             /* 9BD2 */
        OpOraValue(cpu, 0x80u);
        OpSta(memory, cpu, OpAbsX(cpu, GRID));
    }
    SimulateRtsFrame(memory, cpu);                             /* 9B72 */
}

/* $83:9BDD: flood room $2A from its last cell. A room of one cell takes a
   neighbour's id; otherwise each unreached cell of $2A takes its left, else
   upper neighbour's id, else D's low byte (bit 7 dropped everywhere). */
void Lufia2CaveMergeRoom(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpLdx(cpu, 0x00ffu);                                     /* 9BDD */
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, GRID));             /* 9BE0 */
        OpCmp(memory, cpu, OpDp(cpu, 0x2au));
        if (cpu->zero)
            break;
        OpDex(cpu);
        OpCpx(cpu, 0x0010u);
        if (!cpu->carry) {
            SimulateRtsFrame(memory, cpu);                     /* 9BED */
            return;
        }
    }
    TransferDirectToA(cpu);                                    /* 9BEE */
    OpTxa(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x0000u));
    OpSta(memory, cpu, OpDp(cpu, 0x56u));
    OpStz(memory, cpu, OpDp(cpu, 0x2du));
    OpLdy(cpu, 0x0001u);
    for (;;) {
        OpDey(cpu);                                          /* 9BFA */
        if (cpu->negative)
            break;
        TransferDirectToA(cpu);                                /* 9C00 */
        OpLda(memory, cpu, OpAbsY(cpu, 0x0000u));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, GRID));
        if (cpu->negative)
            continue;
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        if (CaveSameNeighbour(memory, cpu, 0xe9f0u)) {         /* 9C0C */
            OpTxa(cpu);
            cpu->carry = 1;
            OpSbcValue(cpu, 0x10u);
            CavePushCell(memory, cpu);
        }
        if (CaveSameNeighbour(memory, cpu, 0xea10u)) {         /* 9C1F */
            OpTxa(cpu);
            cpu->carry = 0;
            OpAdcValue(cpu, 0x10u);
            CavePushCell(memory, cpu);
        }
        if (CaveSameNeighbour(memory, cpu, 0xea01u)) {         /* 9C32 */
            OpTxa(cpu);
            OpIncA(cpu);
            CavePushCell(memory, cpu);
        }
        if (CaveSameNeighbour(memory, cpu, 0xe9ffu)) {         /* 9C43 */
            OpTxa(cpu);
            OpDecA(cpu);
            CavePushCell(memory, cpu);
        }
        OpStepMem(memory, cpu, OpDp(cpu, 0x2du), 1);       /* 9C54 */
        OpLda(memory, cpu, OpDp(cpu, 0x54u));
        OpOraValue(cpu, 0x80u);
        OpSta(memory, cpu, OpAbsX(cpu, GRID));
    }
    OpLda(memory, cpu, OpDp(cpu, 0x2du));                  /* 9C60 */
    OpCmpValue(cpu, 0x01u);
    if (cpu->zero) {
        /* X is the last cell taken from the worklist. */
        OpLda(memory, cpu, OpAbsX(cpu, 0xea01u));
        if (cpu->zero)
            OpLda(memory, cpu, OpAbsX(cpu, 0xe9ffu));
        if (cpu->zero)
            OpLda(memory, cpu, OpAbsX(cpu, 0xe9f0u));
        if (cpu->zero)
            OpLda(memory, cpu, OpAbsX(cpu, 0xea10u));
        OpSta(memory, cpu, OpAbsX(cpu, GRID));             /* 9C78 */
        SimulateRtsFrame(memory, cpu);
        return;
    }
    OpLdx(cpu, 0x00ffu);                                     /* 9C7D */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, GRID));
        if (cpu->negative) {
            OpAndValue(cpu, 0x7fu);                          /* 9C94 */
            OpSta(memory, cpu, OpAbsX(cpu, GRID));
        } else {
            OpCmp(memory, cpu, OpDp(cpu, 0x2au));
            if (cpu->zero) {
                OpLda(memory, cpu, OpAbsX(cpu, 0xe9ffu));
                if (cpu->zero) {
                    OpLda(memory, cpu, OpAbsX(cpu, 0xe9f0u));
                    if (cpu->zero)
                        TransferDirectToA(cpu);                /* 9C93 */
                }
                OpAndValue(cpu, 0x7fu);
                OpSta(memory, cpu, OpAbsX(cpu, GRID));
            }
        }
        OpDex(cpu);                                          /* 9C99 */
        OpCpx(cpu, 0x0010u);
    } while (cpu->carry);
    SimulateRtsFrame(memory, cpu);                             /* 9C9F */
}

/* $83:9CA0: fill the empty cells of rectangle $26,$27 size $24x$25
   with room id $54. */
void Lufia2CaveFillRoom(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpLda(memory, cpu, OpDp(cpu, 0x26u));                  /* 9CA0 */
    OpSta(memory, cpu, OpDp(cpu, 0x8fu));
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    OpSta(memory, cpu, OpDp(cpu, 0x91u));
    OpLda(memory, cpu, OpDp(cpu, 0x25u));
    OpSta(memory, cpu, OpDp(cpu, 0x59u));
    do {
        Lufia2CaveCellIndex(memory, cpu, 0x9cacu);             /* 9CAC */
        OpLda(memory, cpu, OpDp(cpu, 0x24u));
        OpSta(memory, cpu, OpDp(cpu, 0x58u));
        do {
            OpLda(memory, cpu, OpLongX(cpu, 0x7fea00u));   /* 9CB3 */
            if (cpu->zero) {
                OpLda(memory, cpu, OpDp(cpu, 0x54u));
                OpSta(memory, cpu, OpLongX(cpu, 0x7fea00u));
            }
            OpInx(cpu);                                      /* 9CBF */
            OpStepMem(memory, cpu, OpDp(cpu, 0x58u), -1);
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpDp(cpu, 0x91u), 1);       /* 9CC4 */
        OpStepMem(memory, cpu, OpDp(cpu, 0x59u), -1);
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);                             /* 9CCA */
}

/* $83:9CCB: X = $91 << 4 | $8F (row, column). */
void Lufia2CaveCellIndex(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    TransferDirectToA(cpu);                                    /* 9CCB */
    OpLda(memory, cpu, OpDp(cpu, 0x91u));
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    OpOra(memory, cpu, OpDp(cpu, 0x8fu));
    OpTax(cpu);
    SimulateRtsFrame(memory, cpu);                             /* 9CD5 */
}

/* Offset = ($91 << shift) * 3 + 2 * $8F, 16-bit; A carries the result. */
static void CaveTileOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned shift) {
    for (unsigned i = 0; i < shift; ++i)
        AslA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    AslA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x54u));
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpLda(memory, cpu, OpDp(cpu, 0x8fu));
    OpAndValue(cpu, 0x00ffu);
    AslA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x54u));
}

/* $83:9CD6: X = $91 * 96 + $8F * 2 (row of $7E:4000 blocks). */
void Lufia2CaveBlockOffsetX(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    TransferDirectToA(cpu);                                    /* 9CD6 */
    OpLda(memory, cpu, OpDp(cpu, 0x91u));
    OpRep(cpu, 0x20u);
    CaveTileOffset(memory, cpu, 5);
    OpTax(cpu);
    OpSep(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);                             /* 9CF2 */
}

/* $83:9CF3: Y = $91 * 192 + $8F * 2 (tile map row of $7F:000A). */
void Lufia2CaveTileOffsetY(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    TransferDirectToA(cpu);                                    /* 9CF3 */
    OpLda(memory, cpu, OpDp(cpu, 0x91u));
    OpRep(cpu, 0x20u);
    CaveTileOffset(memory, cpu, 6);
    OpTay(cpu);
    OpSep(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);                             /* 9D10 */
}

/* $83:9D68: as $83:9CF3, reading $91 as a word masked to its low byte. */
void Lufia2CaveTileOffsetY2(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpRep(cpu, 0x20u);                                       /* 9D68 */
    OpLda(memory, cpu, OpDp(cpu, 0x91u));
    OpAndValue(cpu, 0x00ffu);
    CaveTileOffset(memory, cpu, 6);
    OpTay(cpu);
    OpSep(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);                             /* 9D87 */
}

/* $83:9D11: A = X = a random non-empty cell (scanned from $F0 down, one
   wrap); carry clear when none was found. */
void Lufia2CavePickCell(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpLda(memory, cpu, OpAbs(cpu, 0xe6afu));               /* 9D11 */
    Lufia2CaveRandomBelow(memory, cpu, 0x9d14u);
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    Lufia2CaveFindCell(memory, cpu, 0x9d1au);
    if (!cpu->carry)
        Lufia2CaveFindCell(memory, cpu, 0x9d1fu);
    SimulateRtsFrame(memory, cpu);                             /* 9D22 */
}

/* $83:9D23: the $54-th unvisited non-empty cell from $F0 down. */
void Lufia2CaveFindCell(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpLdx(cpu, 0x00f0u);                                     /* 9D23 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, GRID));
        if (!cpu->zero && !cpu->negative) {
            OpStepMem(memory, cpu, OpDp(cpu, 0x54u), -1);
            if (cpu->zero) {
                OpTxa(cpu);                                  /* 9D31 */
                cpu->carry = 1;
                SimulateRtsFrame(memory, cpu);
                return;
            }
        }
        OpDex(cpu);                                          /* 9D34 */
    } while (!cpu->negative);
    cpu->carry = 0;                                            /* 9D37 */
    SimulateRtsFrame(memory, cpu);
}

/* $83:9D39: append A to the $E6F1 link list, count $2D; Y kept. */
void Lufia2CaveQueueLink(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpWriteX(memory, cpu, OpDp(cpu, 0x2au), cpu->y);       /* 9D39 */
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x2du)));
    OpSta(memory, cpu, OpAbsY(cpu, 0xe6f1u));
    OpIny(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x2du), cpu->y);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x2au)));
    SimulateRtsFrame(memory, cpu);                             /* 9D45 */
}

/* $83:9D46 (M0): upper-layer tile $3F0E,Y keeps its high 6 bits, takes A. */
void Lufia2CaveSetUpperTile(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpAndValue(cpu, 0x03ffu);                                /* 9D46 */
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpLda(memory, cpu, OpAbsY(cpu, 0x3f0eu));
    OpAndValue(cpu, 0xfc00u);
    OpOra(memory, cpu, OpDp(cpu, 0x54u));
    OpSta(memory, cpu, OpAbsY(cpu, 0x3f0eu));
    SimulateRtsFrame(memory, cpu);                             /* 9D56 */
}

/* $83:9D88: draw block $22 of the $93:D69B table at tile offset Y; zero
   source tiles leave both layers unchanged. */
void Lufia2CaveDrawBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->y);       /* 9D88 */
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    OpRep(cpu, 0x20u);
    AslA16(cpu);
    AslA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x93d69bu));
    OpSta(memory, cpu, OpDp(cpu, 0x5du));
    OpSep(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x93d69du));           /* 9D9A */
    OpSta(memory, cpu, OpDp(cpu, 0x5au));
    OpStz(memory, cpu, OpDp(cpu, 0x5bu));
    OpLda(memory, cpu, OpLongX(cpu, 0x93d69eu));
    OpSta(memory, cpu, OpDp(cpu, 0x56u));
    OpStz(memory, cpu, OpDp(cpu, 0x57u));
    OpRep(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x5du)));  /* 9DAC */
    do {
        OpLda(memory, cpu, OpDp(cpu, 0x5au));              /* 9DAE */
        OpSta(memory, cpu, OpDp(cpu, 0x58u));
        do {
            OpLda(memory, cpu, OpLongX(cpu, 0x7e400au));   /* 9DB2 */
            if (!cpu->zero)
                OpSta(memory, cpu, OpAbsY(cpu, 0x000au));
            OpLda(memory, cpu, OpLongX(cpu, 0x7e4fceu));   /* 9DBB */
            if (!cpu->zero)
                OpSta(memory, cpu, OpAbsY(cpu, 0x3f0eu));
            OpInx(cpu);                                      /* 9DC4 */
            OpInx(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 0x58u), -1);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpDp(cpu, 0x5du));              /* 9DCC */
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0060u);
        OpSta(memory, cpu, OpDp(cpu, 0x5du));
        OpTax(cpu);
        OpLda(memory, cpu, OpDp(cpu, 0x60u));
        OpAdcValue(cpu, 0x00c0u);                            /* no CLC */
        OpSta(memory, cpu, OpDp(cpu, 0x60u));
        OpTay(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x56u), -1);
    } while (!cpu->zero);
    OpSep(cpu, 0x20u);                                       /* 9DE1 */
    SimulateRtsFrame(memory, cpu);
}
