/* Ancient Cave room grid; bound leaf routines stay op-level. */

#include "cave/cave_internal.h"
#include "system/dp_scratch.h"

/* $83:9B18: Clear visited and corridor bits. */
static uint32_t Lufia2CaveClearVisitedBody(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x00ffu);                                     /* 9B18 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        OpAndValue(cpu, 0x3fu);
        OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        OpDex(cpu);
    } while (!cpu->negative);                                  /* 9B24 */
    return 0x839b26u;
}

void Lufia2CaveClearVisited(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveClearVisitedBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9B27: Count nonempty grid cells. */
static uint32_t Lufia2CaveCountCellsBody(const Lufia2Memory *memory,
                                         Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x0000u);                                     /* 9B27 */
    OpStz(memory, cpu, OpAbs(cpu, 0xe6afu));
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));             /* 9B2D */
        if (!cpu->zero)
            OpStepMem(memory, cpu, OpAbs(cpu, 0xe6afu), 1);
        OpInx(cpu);
        OpCpx(cpu, 0x0100u);
    } while (!cpu->carry);
    return 0x839b3bu;
}

void Lufia2CaveCountCells(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveCountCellsBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9B3C: Offset stairs within their cell. */
static uint32_t Lufia2CaveStairOffsetBody(Lufia2CpuState *cpu) {
    OpIncA(cpu);                                             /* 9B3C */
    OpIncA(cpu);
    OpIncA(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    ExchangeAccumulatorBytes(cpu);
    return 0x839b43u;
}

void Lufia2CaveStairOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveStairOffsetBody(cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9B48: Resolve cell tile coordinates. */
static uint32_t Lufia2CaveCellPositionBody(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A)); /* 9B48 */
    OpAndValue(cpu, 0x0fu);
    AslA8(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_B));
    AslA8(cpu);
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_B));
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpAndValue(cpu, 0xf0u);
    cpu->carry = 1;
    Sbc8(cpu, 0x10u);
    LsrA8(cpu);
    LsrA8(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_B));
    LsrA8(cpu);
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_B));
    return 0x839b61u; /* 9B61 */
}

void Lufia2CaveCellPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveCellPositionBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
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
    OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    return cpu->zero;
}

/* $83:9B62: Traverse room cells and queue links. */
static uint32_t Lufia2CaveLinkRoomsBody(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, CAVE_START_COLUMN));               /* 9B62 */
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
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        if (cpu->negative)
            continue;
        OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        OpLda(memory, cpu, OpAbsX(cpu, 0xe9f0u));          /* 9B7F */
        if (!cpu->zero && !cpu->negative) {
            OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
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
            OpCmp(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
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
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));             /* 9BD2 */
        OpOraValue(cpu, 0x80u);
        OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
    }
    return 0x839b72u; /* 9B72 */
}

void Lufia2CaveLinkRooms(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveLinkRoomsBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9BDD: Merge the room containing its last cell. */
static uint32_t Lufia2CaveMergeRoomBody(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x00ffu);                                     /* 9BDD */
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));             /* 9BE0 */
        OpCmp(memory, cpu, OpDp(cpu, 0x2au));
        if (cpu->zero)
            break;
        OpDex(cpu);
        OpCpx(cpu, 0x0010u);
        if (!cpu->carry) {
            return 0x839bedu;
        }
    }
    TransferDirectToA(cpu);                                    /* 9BEE */
    OpTxa(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0x0000u));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    OpStz(memory, cpu, OpDp(cpu, 0x2du));
    OpLdy(cpu, 0x0001u);
    for (;;) {
        OpDey(cpu);                                          /* 9BFA */
        if (cpu->negative)
            break;
        TransferDirectToA(cpu);                                /* 9C00 */
        OpLda(memory, cpu, OpAbsY(cpu, 0x0000u));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        if (cpu->negative)
            continue;
        OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
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
        OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        OpOraValue(cpu, 0x80u);
        OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
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
        OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));             /* 9C78 */
        return 0x839c9fu;
    }
    OpLdx(cpu, 0x00ffu);                                     /* 9C7D */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        if (cpu->negative) {
            OpAndValue(cpu, 0x7fu);                          /* 9C94 */
            OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
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
                OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
            }
        }
        OpDex(cpu);                                          /* 9C99 */
        OpCpx(cpu, 0x0010u);
    } while (cpu->carry);
    return 0x839c9fu; /* 9C9F */
}

void Lufia2CaveMergeRoom(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveMergeRoomBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9CA0: Fill empty cells in a room rectangle. */
static uint32_t Lufia2CaveFillRoomBody(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, 0x26u));                  /* 9CA0 */
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    OpLda(memory, cpu, OpDp(cpu, 0x27u));
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    OpLda(memory, cpu, OpDp(cpu, 0x25u));
    OpSta(memory, cpu, OpDp(cpu, 0x59u));
    do {
        Lufia2CaveCellIndex(memory, cpu, 0x9cacu);             /* 9CAC */
        OpLda(memory, cpu, OpDp(cpu, 0x24u));
        OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_E));
        do {
            OpLda(memory, cpu, OpLongX(cpu, CAVE_ROOM_GRID_LONG));   /* 9CB3 */
            if (cpu->zero) {
                OpLda(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
                OpSta(memory, cpu, OpLongX(cpu, CAVE_ROOM_GRID_LONG));
            }
            OpInx(cpu);                                      /* 9CBF */
            OpStepMem(memory, cpu, OpDp(cpu, DP_SCRATCH_E), -1);
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW), 1);       /* 9CC4 */
        OpStepMem(memory, cpu, OpDp(cpu, 0x59u), -1);
    } while (!cpu->zero);
    return 0x839ccau; /* 9CCA */
}

void Lufia2CaveFillRoom(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveFillRoomBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9CCB: Resolve the packed cell index. */
static uint32_t Lufia2CaveCellIndexBody(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);                                    /* 9CCB */
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    AslA8(cpu);
    OpOra(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    OpTax(cpu);
    return 0x839cd5u; /* 9CD5 */
}

void Lufia2CaveCellIndex(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveCellIndexBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* Offset = ($91 << shift) * 3 + 2 * $8F, 16-bit; A carries the result. */
static void CaveTileOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    unsigned shift) {
    for (unsigned i = 0; i < shift; ++i)
        AslA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    AslA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    OpAndValue(cpu, 0x00ffu);
    AslA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
}

/* $83:9CD6: Resolve the block row offset. */
static uint32_t Lufia2CaveBlockOffsetXBody(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);                                    /* 9CD6 */
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    OpRepWidths(cpu, 0x20u);
    CaveTileOffset(memory, cpu, 5);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    return 0x839cf2u; /* 9CF2 */
}

void Lufia2CaveBlockOffsetX(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveBlockOffsetXBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9CF3: Resolve the tile map row offset. */
static uint32_t Lufia2CaveTileOffsetYBody(const Lufia2Memory *memory,
                                          Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);                                    /* 9CF3 */
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    OpRepWidths(cpu, 0x20u);
    CaveTileOffset(memory, cpu, 6);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    return 0x839d10u; /* 9D10 */
}

void Lufia2CaveTileOffsetY(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                           uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveTileOffsetYBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9D68: Resolve the word-loaded tile map offset. */
static uint32_t Lufia2CaveTileOffsetY2Body(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);                                       /* 9D68 */
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    OpAndValue(cpu, 0x00ffu);
    CaveTileOffset(memory, cpu, 6);
    OpTay(cpu);
    OpSepWidths(cpu, 0x20u);
    return 0x839d87u; /* 9D87 */
}

void Lufia2CaveTileOffsetY2(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveTileOffsetY2Body(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9D11: Pick an unvisited nonempty cell. */
static uint32_t Lufia2CavePickCellBody(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, 0xe6afu));               /* 9D11 */
    Lufia2CaveRandomBelow(memory, cpu, 0x9d14u);
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    Lufia2CaveFindCell(memory, cpu, 0x9d1au);
    if (!cpu->carry)
        Lufia2CaveFindCell(memory, cpu, 0x9d1fu);
    return 0x839d22u; /* 9D22 */
}

void Lufia2CavePickCell(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CavePickCellBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9D23: Find the requested unvisited cell. */
static uint32_t Lufia2CaveFindCellBody(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x00f0u);                                     /* 9D23 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        if (!cpu->zero && !cpu->negative) {
            OpStepMem(memory, cpu, OpDp(cpu, DP_SCRATCH_A), -1);
            if (cpu->zero) {
                OpTxa(cpu);                                  /* 9D31 */
                cpu->carry = 1;
                return 0x839d33u;
            }
        }
        OpDex(cpu);                                          /* 9D34 */
    } while (!cpu->negative);
    cpu->carry = 0;                                            /* 9D37 */
    return 0x839d38u;
}

void Lufia2CaveFindCell(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveFindCellBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9D39: Append a link while retaining Y. */
static uint32_t Lufia2CaveQueueLinkBody(const Lufia2Memory *memory,
                                        Lufia2CpuState *cpu) {
    OpWriteX(memory, cpu, OpDp(cpu, 0x2au), cpu->y);       /* 9D39 */
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x2du)));
    OpSta(memory, cpu, OpAbsY(cpu, 0xe6f1u));
    OpIny(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, 0x2du), cpu->y);
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x2au)));
    return 0x839d45u; /* 9D45 */
}

void Lufia2CaveQueueLink(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveQueueLinkBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9D46: Replace the upper tile index. */
static uint32_t Lufia2CaveSetUpperTileBody(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu) {
    OpAndValue(cpu, 0x03ffu);                                /* 9D46 */
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpLda(memory, cpu, OpAbsY(cpu, 0x3f0eu));
    OpAndValue(cpu, 0xfc00u);
    OpOra(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
    OpSta(memory, cpu, OpAbsY(cpu, 0x3f0eu));
    return 0x839d56u; /* 9D56 */
}

void Lufia2CaveSetUpperTile(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                            uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    (void)Lufia2CaveSetUpperTileBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9D88: draw block $22 at tile offset Y. */
void Lufia2CaveDrawBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->y);       /* 9D88 */
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x22u));
    OpRepWidths(cpu, 0x20u);
    AslA16(cpu);
    AslA16(cpu);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x93d69bu));
    OpSta(memory, cpu, OpDp(cpu, 0x5du));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpLongX(cpu, 0x93d69du));           /* 9D9A */
    OpSta(memory, cpu, OpDp(cpu, 0x5au));
    OpStz(memory, cpu, OpDp(cpu, 0x5bu));
    OpLda(memory, cpu, OpLongX(cpu, 0x93d69eu));
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_C));
    OpStz(memory, cpu, OpDp(cpu, DP_SCRATCH_D));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x5du)));  /* 9DAC */
    do {
        OpLda(memory, cpu, OpDp(cpu, 0x5au));              /* 9DAE */
        OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_E));
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
            OpStepMem(memory, cpu, OpDp(cpu, DP_SCRATCH_E), -1);
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
        OpStepMem(memory, cpu, OpDp(cpu, DP_SCRATCH_C), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);                                       /* 9DE1 */
    SimulateRtsFrame(memory, cpu);
}

static uint8_t CaveGridEntryContext(const Lufia2CpuState *cpu,
                                    unsigned accumulator_mode, unsigned work_bank) {
    return cpu->program_bank == 0x83u && !cpu->direct_page && !cpu->decimal &&
           !cpu->index_is_8_bit &&
           (accumulator_mode == 2u || cpu->accumulator_is_8_bit == accumulator_mode) &&
           (!work_bank || cpu->data_bank == 0x7fu) && cpu->stack >= 0x1f00u &&
           cpu->stack <= 0x1ffcu;
}

Lufia2ExecutionResult Lufia2CaveClearVisitedCells(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 1u))
        return ExecutionHandoff(cpu, 0x839b18u);
    return ExecutionReturned(Lufia2CaveClearVisitedBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveCountOccupiedCells(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 1u))
        return ExecutionHandoff(cpu, 0x839b27u);
    return ExecutionReturned(Lufia2CaveCountCellsBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveOffsetStairPosition(const Lufia2Memory *memory,
                                                    Lufia2CpuState *cpu) {
    (void)memory;
    if (!CaveGridEntryContext(cpu, 1u, 0u))
        return ExecutionHandoff(cpu, 0x839b3cu);
    return ExecutionReturned(Lufia2CaveStairOffsetBody(cpu));
}

Lufia2ExecutionResult Lufia2CaveResolveCellOrigin(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 0u))
        return ExecutionHandoff(cpu, 0x839b48u);
    return ExecutionReturned(Lufia2CaveCellPositionBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveLinkRoomCells(const Lufia2Memory *memory,
                                              Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 1u))
        return ExecutionHandoff(cpu, 0x839b62u);
    return ExecutionReturned(Lufia2CaveLinkRoomsBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveMergeRoomCells(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 1u))
        return ExecutionHandoff(cpu, 0x839bddu);
    return ExecutionReturned(Lufia2CaveMergeRoomBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveFillRoomRectangle(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 0u))
        return ExecutionHandoff(cpu, 0x839ca0u);
    return ExecutionReturned(Lufia2CaveFillRoomBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveResolveCellIndex(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 0u))
        return ExecutionHandoff(cpu, 0x839ccbu);
    return ExecutionReturned(Lufia2CaveCellIndexBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveResolveBlockOffset(const Lufia2Memory *memory,
                                                   Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 0u))
        return ExecutionHandoff(cpu, 0x839cd6u);
    return ExecutionReturned(Lufia2CaveBlockOffsetXBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveResolveTileOffset(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 0u))
        return ExecutionHandoff(cpu, 0x839cf3u);
    return ExecutionReturned(Lufia2CaveTileOffsetYBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveResolveWordTileOffset(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 2u, 0u))
        return ExecutionHandoff(cpu, 0x839d68u);
    return ExecutionReturned(Lufia2CaveTileOffsetY2Body(memory, cpu));
}

Lufia2ExecutionResult Lufia2CavePickUnvisitedCell(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 1u))
        return ExecutionHandoff(cpu, 0x839d11u);
    return ExecutionReturned(Lufia2CavePickCellBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveFindUnvisitedCell(const Lufia2Memory *memory,
                                                  Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 1u))
        return ExecutionHandoff(cpu, 0x839d23u);
    return ExecutionReturned(Lufia2CaveFindCellBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveAppendRoomLink(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 1u, 1u))
        return ExecutionHandoff(cpu, 0x839d39u);
    return ExecutionReturned(Lufia2CaveQueueLinkBody(memory, cpu));
}

Lufia2ExecutionResult Lufia2CaveReplaceUpperTileIndex(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    if (!CaveGridEntryContext(cpu, 0u, 1u))
        return ExecutionHandoff(cpu, 0x839d46u);
    return ExecutionReturned(Lufia2CaveSetUpperTileBody(memory, cpu));
}
