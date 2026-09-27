/* Ancient Cave object and chest placement ($83:99C8-$83:9B17).
 *
 * Tile coordinates are $8F (column) and $91 (row). Objects live at
 * $7F:E6B1/E6D1 (count $E731, at most $14), chests at $7F:E736/E73E
 * (count $E734, at most 8). The start cell origin is $E6A9/$E6AA and the
 * stairs are at $E6AB/$E6AC.
 */

#include "cave/cave_internal.h"

/* $83:99C8: $91 = A, $8F = B, Y = tile offset; returns M0. */
void Lufia2CaveTileAt(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpSta(memory, cpu, OpDp(cpu, 0x91u));                  /* 99C8 */
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x8fu));
    Lufia2CaveTileOffsetY(memory, cpu, 0x99cdu);
    OpRep(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);                             /* 99D2 */
}

/* $83:99D3: carry set when $8F,$91 is near the start cell or on an object;
   X kept. For objects with $E216 bit 0 the ROM also compares the size value
   + 1 (not the column + 1) with $8F; that never matches because generated
   columns are at least 7. */
void Lufia2CaveNearStartOrPlaced(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    uint8_t found = 0;

    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpPushX(memory, cpu);                                    /* 99D3 */
    OpLda(memory, cpu, OpAbs(cpu, 0xe6a9u));
    OpDecA(cpu);
    OpDecA(cpu);
    OpCmp(memory, cpu, OpDp(cpu, 0x8fu));
    if (!cpu->carry) {
        cpu->carry = 0;
        OpAdcValue(cpu, 0x04u);
        OpCmp(memory, cpu, OpDp(cpu, 0x8fu));
        if (cpu->carry) {
            OpLda(memory, cpu, OpAbs(cpu, 0xe6aau));       /* 99E4 */
            OpDecA(cpu);
            OpCmp(memory, cpu, OpDp(cpu, 0x91u));
            if (!cpu->carry) {
                OpIncA(cpu);
                OpIncA(cpu);
                OpIncA(cpu);
                OpCmp(memory, cpu, OpDp(cpu, 0x91u));
                found = cpu->carry;
            }
        }
    }
    if (!found) {
        OpLdx(cpu, 0x0000u);                                 /* 99F6 */
        do {
            uint8_t column;

            OpLda(memory, cpu, OpAbsX(cpu, 0xe6b1u));      /* 99F9 */
            OpCmp(memory, cpu, OpDp(cpu, 0x8fu));
            column = cpu->zero;
            if (!column) {
                OpLda(memory, cpu, OpAbsX(cpu, 0xe216u));
                OpBitValue(cpu, 0x01u);
                if (!cpu->zero) {
                    OpIncA(cpu);
                    OpCmp(memory, cpu, OpDp(cpu, 0x8fu));
                    column = cpu->zero;
                }
            }
            if (column) {
                OpLda(memory, cpu, OpAbsX(cpu, 0xe6d1u));  /* 9A0C */
                OpCmp(memory, cpu, OpDp(cpu, 0x91u));
                if (cpu->zero) {
                    found = 1;
                    break;
                }
            }
            OpInx(cpu);                                      /* 9A13 */
            OpCompareIndex(cpu, cpu->x,
                OpReadX(memory, cpu, OpAbs(cpu, 0xe731u)));
        } while (!cpu->carry);
    }
    cpu->carry = found;                                        /* 99F3/9A19 */
    OpPullX(memory, cpu);
    SimulateRtsFrame(memory, cpu);                             /* 9A1B */
}

/* $83:9A1C: add an object at $8F,$91 (up to 20) with a random kind from
   the floor's row of $94:D95C; X kept. */
void Lufia2CaveAddObject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpPushX(memory, cpu);                                    /* 9A1C */
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0xe731u)));
    OpCpx(cpu, 0x0014u);
    if (!cpu->carry) {
        OpLda(memory, cpu, OpDp(cpu, 0x8fu));
        OpSta(memory, cpu, OpAbsX(cpu, 0xe6b1u));
        OpLda(memory, cpu, OpDp(cpu, 0x91u));
        OpSta(memory, cpu, OpAbsX(cpu, 0xe6d1u));
        OpTxy(cpu);
        OpLda(memory, cpu, OpAbs(cpu, 0xe696u));           /* 9A30 */
        OpDecA(cpu);
        OpAslA(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        LoadA8(cpu, 0x06u);
        Lufia2CaveRandomBelow(memory, cpu, 0x9a39u);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, 0x54u));
        OpSep(cpu, 0x30u);                                   /* 9A3F */
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x94d95cu));
        OpSta(memory, cpu, OpAbsY(cpu, 0xf966u));
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x94ddf6u));
        OpRep(cpu, 0x30u);                                   /* 9A4E */
        OpAndValue(cpu, 0x00ffu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0xcff000u));
        OpTax(cpu);
        OpSep(cpu, 0x20u);
        OpLda(memory, cpu, OpLongX(cpu, 0xcf0000u));       /* 9A5C */
        OpAndValue(cpu, 0x03u);
        OpSta(memory, cpu, OpAbsY(cpu, 0xe216u));
        OpIny(cpu);
        OpWriteX(memory, cpu, OpAbs(cpu, 0xe731u), cpu->y);
    }
    OpPullX(memory, cpu);                                    /* 9A69 */
    SimulateRtsFrame(memory, cpu);
}

/* $83:9A6B: carry set when a chest is at $8F,$91, one column left, two
   columns right, one row up or two rows down. */
void Lufia2CaveNearChest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpLda(memory, cpu, OpDp(cpu, 0x91u));                  /* 9A6B */
    OpSta(memory, cpu, OpDp(cpu, 0x55u));
    OpLda(memory, cpu, OpDp(cpu, 0x8fu));
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    Lufia2CaveChestAt(memory, cpu, 0x9a73u);
    if (cpu->carry)
        goto done;
    OpStepMem(memory, cpu, OpDp(cpu, 0x54u), -1);          /* 9A78 */
    Lufia2CaveChestAt(memory, cpu, 0x9a7au);
    if (cpu->carry)
        goto done;
    OpStepMem(memory, cpu, OpDp(cpu, 0x54u), 1);           /* 9A7F */
    OpStepMem(memory, cpu, OpDp(cpu, 0x54u), 1);
    OpStepMem(memory, cpu, OpDp(cpu, 0x54u), 1);
    Lufia2CaveChestAt(memory, cpu, 0x9a85u);
    if (cpu->carry)
        goto done;
    OpLda(memory, cpu, OpDp(cpu, 0x8fu));                  /* 9A8A */
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpStepMem(memory, cpu, OpDp(cpu, 0x55u), -1);
    Lufia2CaveChestAt(memory, cpu, 0x9a90u);
    if (cpu->carry)
        goto done;
    OpStepMem(memory, cpu, OpDp(cpu, 0x55u), 1);           /* 9A95 */
    OpStepMem(memory, cpu, OpDp(cpu, 0x55u), 1);
    OpStepMem(memory, cpu, OpDp(cpu, 0x55u), 1);
    Lufia2CaveChestAt(memory, cpu, 0x9a9bu);
done:
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
            OpReadX(memory, cpu, OpAbs(cpu, 0xe734u)));
        if (cpu->carry) {
            cpu->carry = 0;                                    /* 9ABA */
            break;
        }
        OpLda(memory, cpu, OpAbsX(cpu, 0xe736u));
        OpCmp(memory, cpu, OpDp(cpu, 0x54u));
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, 0xe73eu));
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

/* $83:9ABC: try one chest in the room of cell A; carry set when it was
   rejected (limit, start, stairs or a nearby chest). */
void Lufia2CaveAddChest(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpSta(memory, cpu, OpAbs(cpu, 0xe6a8u));               /* 9ABC */
    OpLda(memory, cpu, OpAbs(cpu, 0xe734u));
    OpCmpValue(cpu, 0x08u);
    if (cpu->carry)
        goto done;
    LoadA8(cpu, 0x07u);                                        /* 9AC6 */
    Lufia2CaveRandomBelow(memory, cpu, 0x9ac8u);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x02u);
    OpSta(memory, cpu, OpDp(cpu, 0x8fu));
    LoadA8(cpu, 0x06u);
    Lufia2CaveRandomBelow(memory, cpu, 0x9ad2u);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x04u);
    OpSta(memory, cpu, OpDp(cpu, 0x91u));
    OpLda(memory, cpu, OpAbs(cpu, 0xe6a8u));               /* 9ADA */
    Lufia2CaveCellPosition(memory, cpu, 0x9addu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, 0x91u));
    OpSta(memory, cpu, OpDp(cpu, 0x91u));
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, 0x8fu));
    OpSta(memory, cpu, OpDp(cpu, 0x8fu));
    Lufia2CaveNearStartOrPlaced(memory, cpu, 0x9aebu);
    if (cpu->carry)
        goto done;
    OpLda(memory, cpu, OpDp(cpu, 0x8fu));                  /* 9AF0 */
    OpCmp(memory, cpu, OpAbs(cpu, 0xe6abu));
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0x91u));
        OpCmp(memory, cpu, OpAbs(cpu, 0xe6acu));
        if (cpu->zero) {
            cpu->carry = 1;                                    /* 9AFE */
            goto done;
        }
    }
    Lufia2CaveNearChest(memory, cpu, 0x9b01u);                 /* 9B01 */
    if (cpu->carry)
        goto done;
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, 0xe734u)));
    OpLda(memory, cpu, OpDp(cpu, 0x8fu));
    OpSta(memory, cpu, OpAbsY(cpu, 0xe736u));
    OpLda(memory, cpu, OpDp(cpu, 0x91u));
    OpSta(memory, cpu, OpAbsY(cpu, 0xe73eu));
    OpStepMem(memory, cpu, OpAbs(cpu, 0xe734u), 1);
    cpu->carry = 0;                                            /* 9B16 */
done:
    SimulateRtsFrame(memory, cpu);                             /* 9B17 */
}
