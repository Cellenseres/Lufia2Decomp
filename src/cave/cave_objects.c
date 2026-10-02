/* Ancient Cave objects and chests ($83:99C8-$83:9B17). */

#include "cave/cave_internal.h"

/* $83:99C8: $91 = A, $8F = B, Y = tile offset; returns M0. */
void Lufia2CaveTileAt(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));                  /* 99C8 */
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    Lufia2CaveTileOffsetY(memory, cpu, 0x99cdu);
    OpRepWidths(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);                             /* 99D2 */
}

/* $83:99D3: carry when $8F,$91 is taken; X kept. */
void Lufia2CaveNearStartOrPlaced(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    uint8_t found = 0;

    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpPushX(memory, cpu);                                    /* 99D3 */
    OpLda(memory, cpu, OpAbs(cpu, CAVE_START_COLUMN));
    OpDecA(cpu);
    OpDecA(cpu);
    OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    if (!cpu->carry) {
        cpu->carry = 0;
        OpAdcValue(cpu, 0x04u);
        OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
        if (cpu->carry) {
            OpLda(memory, cpu, OpAbs(cpu, CAVE_START_ROW));       /* 99E4 */
            OpDecA(cpu);
            OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            if (!cpu->carry) {
                OpIncA(cpu);
                OpIncA(cpu);
                OpIncA(cpu);
                OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
                found = cpu->carry;
            }
        }
    }
    if (!found) {
        OpLdx(cpu, 0x0000u);                                 /* 99F6 */
        do {
            uint8_t column;

            OpLda(memory, cpu, OpAbsX(cpu, CAVE_OBJECT_COLUMNS));      /* 99F9 */
            OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            column = cpu->zero;
            if (!column) {
                OpLda(memory, cpu, OpAbsX(cpu, CAVE_OBJECT_SIZES));
                OpBitValue(cpu, 0x01u);
                if (!cpu->zero) {
                    OpIncA(cpu);
                    OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
                    column = cpu->zero;
                }
            }
            if (column) {
                OpLda(memory, cpu, OpAbsX(cpu, CAVE_OBJECT_ROWS));  /* 9A0C */
                OpCmp(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
                if (cpu->zero) {
                    found = 1;
                    break;
                }
            }
            OpInx(cpu);                                      /* 9A13 */
            OpCompareIndex(cpu, cpu->x,
                OpReadX(memory, cpu, OpAbs(cpu, CAVE_OBJECT_COUNT)));
        } while (!cpu->carry);
    }
    cpu->carry = found;                                        /* 99F3/9A19 */
    OpPullX(memory, cpu);
    SimulateRtsFrame(memory, cpu);                             /* 9A1B */
}

/* $83:9A1C: add a random object at $8F,$91; X kept. */
void Lufia2CaveAddObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpPushX(memory, cpu);                                    /* 9A1C */
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, CAVE_OBJECT_COUNT)));
    OpCpx(cpu, 0x0014u);
    if (!cpu->carry) {
        OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
        OpSta(memory, cpu, OpAbsX(cpu, CAVE_OBJECT_COLUMNS));
        OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
        OpSta(memory, cpu, OpAbsX(cpu, CAVE_OBJECT_ROWS));
        OpTxy(cpu);
        OpLda(memory, cpu, OpAbs(cpu, CAVE_FLOOR));           /* 9A30 */
        OpDecA(cpu);
        OpAslA(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        LoadA8(cpu, 0x06u);
        Lufia2CaveRandomBelow(memory, cpu, 0x9a39u);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, 0x54u));
        OpSepWidths(cpu, 0x30u);                                   /* 9A3F */
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x94d95cu));
        OpSta(memory, cpu, OpAbsY(cpu, 0xf966u));
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x94ddf6u));
        OpRepWidths(cpu, 0x30u);                                   /* 9A4E */
        OpAndValue(cpu, 0x00ffu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0xcff000u));
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0xcf0000u));       /* 9A5C */
        OpAndValue(cpu, 0x03u);
        OpSta(memory, cpu, OpAbsY(cpu, CAVE_OBJECT_SIZES));
        OpIny(cpu);
        OpWriteX(memory, cpu, OpAbs(cpu, CAVE_OBJECT_COUNT), cpu->y);
    }
    OpPullX(memory, cpu);                                    /* 9A69 */
    SimulateRtsFrame(memory, cpu);
}

/* Probe the centre cell, then the neighbours, stopping at the first chest. */
static void CaveNearChestBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));                  /* 9A6B */
    OpSta(memory, cpu, OpDp(cpu, 0x55u));
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    Lufia2CaveChestAt(memory, cpu, 0x9a73u);
    if (cpu->carry)
        return;
    OpStepMem(memory, cpu, OpDp(cpu, 0x54u), -1);          /* 9A78 */
    Lufia2CaveChestAt(memory, cpu, 0x9a7au);
    if (cpu->carry)
        return;
    OpStepMem(memory, cpu, OpDp(cpu, 0x54u), 1);           /* 9A7F */
    OpStepMem(memory, cpu, OpDp(cpu, 0x54u), 1);
    OpStepMem(memory, cpu, OpDp(cpu, 0x54u), 1);
    Lufia2CaveChestAt(memory, cpu, 0x9a85u);
    if (cpu->carry)
        return;
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));                  /* 9A8A */
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpStepMem(memory, cpu, OpDp(cpu, 0x55u), -1);
    Lufia2CaveChestAt(memory, cpu, 0x9a90u);
    if (cpu->carry)
        return;
    OpStepMem(memory, cpu, OpDp(cpu, 0x55u), 1);           /* 9A95 */
    OpStepMem(memory, cpu, OpDp(cpu, 0x55u), 1);
    OpStepMem(memory, cpu, OpDp(cpu, 0x55u), 1);
    Lufia2CaveChestAt(memory, cpu, 0x9a9bu);
}

/* $83:9A6B: carry when a chest is near $8F,$91. */
void Lufia2CaveNearChest(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    CaveNearChestBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);                             /* 9A9E */
}

/* $83:9A9F: carry set when a chest is at $54,$55. */
void Lufia2CaveChestAt(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpLdx(cpu, 0x0000u);                                     /* 9A9F */
    for (;;) {
        OpCompareIndex(cpu, cpu->x,                          /* 9AA2 */
            OpReadX(memory, cpu, OpAbs(cpu, CAVE_CHEST_COUNT)));
        if (cpu->carry) {
            cpu->carry = 0;                                    /* 9ABA */
            break;
        }
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_CHEST_COLUMNS));
        OpCmp(memory, cpu, OpDp(cpu, 0x54u));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, CAVE_CHEST_ROWS));
            OpCmp(memory, cpu, OpDp(cpu, 0x55u));
            if (cpu->zero) {
                cpu->carry = 1;                                /* 9AB5 */
                break;
            }
        }
        OpInx(cpu);                                          /* 9AB7 */
    }
    SimulateRtsFrame(memory, cpu);
}

/* Roll a position and keep it unless it is rejected; carry set = rejected. */
static void CaveAddChestBody(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbs(cpu, 0xe6a8u));               /* 9ABC */
    OpLda(memory, cpu, OpAbs(cpu, CAVE_CHEST_COUNT));
    OpCmpValue(cpu, 0x08u);
    if (cpu->carry)
        return;
    LoadA8(cpu, 0x07u);                                        /* 9AC6 */
    Lufia2CaveRandomBelow(memory, cpu, 0x9ac8u);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x02u);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    LoadA8(cpu, 0x06u);
    Lufia2CaveRandomBelow(memory, cpu, 0x9ad2u);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x04u);
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    OpLda(memory, cpu, OpAbs(cpu, 0xe6a8u));               /* 9ADA */
    Lufia2CaveCellPosition(memory, cpu, 0x9addu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    Lufia2CaveNearStartOrPlaced(memory, cpu, 0x9aebu);
    if (cpu->carry)
        return;
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));                  /* 9AF0 */
    OpCmp(memory, cpu, OpAbs(cpu, CAVE_STAIR_COLUMN));
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
        OpCmp(memory, cpu, OpAbs(cpu, CAVE_STAIR_ROW));
        if (cpu->zero) {
            cpu->carry = 1;                                    /* 9AFE */
            return;
        }
    }
    Lufia2CaveNearChest(memory, cpu, 0x9b01u);                 /* 9B01 */
    if (cpu->carry)
        return;
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, CAVE_CHEST_COUNT)));
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_CHEST_COLUMNS));
    OpLda(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    OpSta(memory, cpu, OpAbsY(cpu, CAVE_CHEST_ROWS));
    OpStepMem(memory, cpu, OpAbs(cpu, CAVE_CHEST_COUNT), 1);
    cpu->carry = 0;                                            /* 9B16 */
}

/* $83:9ABC: try one chest in room A; carry = rejected. */
void Lufia2CaveAddChest(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                        uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    CaveAddChestBody(memory, cpu);
    SimulateRtsFrame(memory, cpu);                             /* 9B17 */
}
