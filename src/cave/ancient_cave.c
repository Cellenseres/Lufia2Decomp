/* Ancient Cave floor entry ($83:9E31) and builder ($83:9013). */

#include "cave/cave_internal.h"
#include "field/event_script_internal.h"
#include "field/field_internal.h"
#include "lufia2/field.h"
#include "party/party_internal.h"
#include "lufia2/system.h"
#include "system/system_internal.h"
#include "text/text_internal.h"

#define BFAA_HANDOFF 0x80bfbcu

static void CaveRandomByte(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    Lufia2CallRandomByte(memory, cpu, (uint16_t)(site + 3u));
}

/* JSL to a whole-function RTL body in the decomp library. */
static void CaveCallLong(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t site,
    Lufia2ExecutionResult (*body)(const Lufia2Memory *, Lufia2CpuState *)) {
    SimulateJslFrame(memory, cpu, 0x83u, (uint16_t)(site + 3u));
    (void)body(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $83:9013-$83:9032: clear the grid to D and the shapes to $1C. */
static void CaveClearGrid(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);                                 /* 9013 */
    OpSepWidths(cpu, 0x20u);
    OpRepWidths(cpu, 0x10u);
    OpSetDataBank(memory, cpu, 0x7eu);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x0000u);
    do {
        TransferDirectToA(cpu);                                /* 9021 */
        OpSta(memory, cpu, OpLongX(cpu, CAVE_ROOM_GRID_LONG));
        LoadA16(cpu, 0x1c1cu);
        OpSta(memory, cpu, OpLongX(cpu, CAVE_SHAPE_GRID_LONG));
        OpInx(cpu);
        OpInx(cpu);
        OpCpx(cpu, 0x0100u);
    } while (!cpu->carry);
}

/* $83:9034-$83:90C3: split item records by floor limit. */
static void CaveCollectItems(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);                                         /* 9034 */
    OpSetDataBank(memory, cpu, 0x96u);
    OpRepWidths(cpu, 0x20u);
    OpSepWidths(cpu, 0x20u);
    OpLdx(cpu, 0x0000u);                                       /* 903E */
    OpWriteX(memory, cpu, OpDp(cpu, 0x5du), cpu->x);
    OpLdx(cpu, 0x1000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9048 */
    OpCmpValue(cpu, 0x3cu);
    if (cpu->carry) {
        LoadA8(cpu, 0xffu);
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        OpSta(memory, cpu, OpDp(cpu, 0x55u));
    } else {
        /* Limit = 1000 * (floor + 1) through the PPU multiplier. */
        LoadA8(cpu, 0xe8u);                                    /* 9058 */
        OpSta(memory, cpu, OpAbs(cpu, 0x211bu));
        LoadA8(cpu, 0x03u);
        OpSta(memory, cpu, OpAbs(cpu, 0x211bu));
        OpLda(memory, cpu, CAVE_FLOOR_LONG);
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, 0x211cu));
        OpLda(memory, cpu, OpAbs(cpu, 0x2134u));
        OpSta(memory, cpu, OpDp(cpu, 0x54u));
        OpLda(memory, cpu, OpAbs(cpu, 0x2135u));
        OpSta(memory, cpu, OpDp(cpu, 0x55u));
    }
    OpLdy(cpu, 0x0000u);                                       /* 9074 */
    do {
        OpRepWidths(cpu, 0x20u);                                     /* 9077 */
        OpLda(memory, cpu, OpAbsY(cpu, 0xcf69u));
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLda(memory, cpu, OpAbsX(cpu, 0xcf69u));
        OpBitValue(cpu, 0x02u);
        if (cpu->zero)
            goto next;
        OpBitValue(cpu, 0x20u);
        if (!cpu->zero)
            goto next;
        OpLda(memory, cpu, OpAbsX(cpu, 0xcf6au));
        OpBitValue(cpu, 0x20u);
        if (!cpu->zero)
            goto next;
        OpRepWidths(cpu, 0x20u);                                     /* 9091 */
        OpLda(memory, cpu, OpAbsX(cpu, 0xcf6eu));
        OpCmp(memory, cpu, OpDp(cpu, 0x54u));
        if (cpu->carry)
            goto next;
        OpLda(memory, cpu, OpAbsX(cpu, 0xcf70u));
        OpBitValue(cpu, 0x0001u);
        {
            const uint8_t list = cpu->zero ? 0x60u : 0x5du;    /* 90A2/90B0 */

            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, list)));
            OpTya(cpu);
            OpLsrA(cpu);
            OpSta(memory, cpu, OpLongX(cpu, 0x7f0000u));
            OpInx(cpu);
            OpInx(cpu);
            OpWriteX(memory, cpu, OpDp(cpu, list), cpu->x);
        }
next:
        OpSepWidths(cpu, 0x20u);                                     /* 90BC */
        OpIny(cpu);
        OpIny(cpu);
        OpCpy(cpu, 0x03a4u);
    } while (!cpu->carry);
}

/* $83:90C5-$83:9141: optional first chests; Y = next slot. */
static void CaveFirstChests(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7fu);                         /* 90C5 */
    OpLdy(cpu, 0x0000u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, 0xe75du));
    CaveRandomByte(memory, cpu, 0x90d0u);
    OpCmpValue(cpu, 0x05u);
    if (!cpu->carry) {
        LoadA8(cpu, 0x09u);                                    /* 90D8 */
        Lufia2CaveRandomBelow(memory, cpu, 0x90dau);
        OpSta(memory, cpu, OpDp(cpu, 0x58u));
        Lufia2EventFlagBitFrom(memory, cpu, 0x83u, 0x90e2u);
        OpLda(memory, cpu, OpLongX(cpu, 0x7fe75eu));           /* 90E3 */
        OpBit(memory, cpu, OpDp(cpu, 0x55u));
        if (cpu->zero) {
            OpOra(memory, cpu, OpDp(cpu, 0x55u));
            OpSta(memory, cpu, OpLongX(cpu, 0x7fe75eu));
            OpLda(memory, cpu, OpDp(cpu, 0x58u));
            cpu->carry = 0;
            OpAdcValue(cpu, 0xc8u);
            /* JSL $80:BE1A = JSR $80:BE1E; RTL. */
            SimulateJslFrame(memory, cpu, 0x83u, 0x90f9u);     /* 90F6 */
            Lufia2TextTestFlag(memory, cpu, 0xbe1cu);
            SimulateRtlFrame(memory, cpu);
            if (cpu->zero) {
                TransferDirectToA(cpu);                        /* 90FC */
                OpLda(memory, cpu, OpDp(cpu, 0x58u));
                OpRepWidths(cpu, 0x20u);
                OpAslA(cpu);
                OpTax(cpu);
                OpLda(memory, cpu, OpLongX(cpu, 0x91ffcau));
                OpOraValue(cpu, 0x0200u);
                OpSta(memory, cpu, OpAbs(cpu, 0xe746u));
                OpSepWidths(cpu, 0x20u);
                LoadA8(cpu, 0x80u);
                OpSta(memory, cpu, OpAbs(cpu, 0xe75du));
                goto take_slot;
            }
        }
    }
    OpLda(memory, cpu, 0x7fe75bu);                             /* 9116 */
    if (cpu->negative)
        return;
    TransferDirectToA(cpu);
    OpSta(memory, cpu, 0x7fe75bu);
    OpLda(memory, cpu, CAVE_FLOOR_LONG);
    OpCmpValue(cpu, 0x15u);
    if (!cpu->carry)
        return;
    LoadA8(cpu, 0x3cu);                                        /* 9129 */
    Lufia2CaveRandomBelow(memory, cpu, 0x912bu);
    OpCmp(memory, cpu, CAVE_FLOOR_LONG);
    if (cpu->carry)
        return;
    OpLdx(cpu, 0x022du);                                       /* 9134 */
    OpWriteX(memory, cpu, OpAbs(cpu, 0xe746u), cpu->x);
    LoadA8(cpu, 0x01u);
    OpSta(memory, cpu, 0x7fe75bu);
take_slot:
    OpIny(cpu);                                                /* 9140 */
    OpIny(cpu);
}

/* Chest word from a ROM table at X. */
static void CaveChestFromTable(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint32_t table,
    uint8_t high_or) {
    OpLda(memory, cpu, OpLongX(cpu, table));
    OpSta(memory, cpu, OpAbsY(cpu, 0xe746u));
    OpLda(memory, cpu, OpLongX(cpu, table + 1u));
    if (high_or)
        OpOraValue(cpu, high_or);
    OpSta(memory, cpu, OpAbsY(cpu, 0xe747u));
}

/* $83:91AD: common item from $91:FFDC. */
static void CaveCommonChest(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x09u);                                        /* 91AD */
    Lufia2CaveRandomMean(memory, cpu, 0x91afu);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    OpAslA(cpu);
    OpTax(cpu);
    CaveChestFromTable(memory, cpu, 0x91ffdcu, 0);
}

/* $83:916F/$83:9176: random word of an item list. */
static void CaveChestFromList(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t base,
    uint8_t end) {
    OpLdx(cpu, base);
    OpLda(memory, cpu, OpDp(cpu, end));
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);           /* 917B */
    OpLsrA(cpu);
    Lufia2CaveRandomIndex(memory, cpu, 0x917eu);
    OpRepWidths(cpu, 0x20u);
    OpTxa(cpu);
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, 0x54u));
    OpTax(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, 0x0000u));
    OpSta(memory, cpu, OpAbsY(cpu, 0xe746u));
    OpSepWidths(cpu, 0x20u);
}

/* $83:9192: spell $00-$22 unless a party member already has it. */
static void CaveSpellChest(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x23u);                                        /* 9192 */
    Lufia2CaveRandomBelow(memory, cpu, 0x9194u);
    OpSta(memory, cpu, OpDp(cpu, 0x55u));
    PushY(memory, cpu);
    CaveCallLong(memory, cpu, 0x919au, Lufia2PartyListHasEntry);
    OpPullY(memory, cpu);
    if (cpu->carry) {
        CaveCommonChest(memory, cpu);
        return;
    }
    OpLda(memory, cpu, OpDp(cpu, 0x55u));                      /* 91A1 */
    OpSta(memory, cpu, OpAbsY(cpu, 0xe746u));
    LoadA8(cpu, 0x80u);
    OpSta(memory, cpu, OpAbsY(cpu, 0xe747u));
}

/* $83:9142-$83:91E4: fill the chest words $E746,Y. */
static void CaveChestContents(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    do {
        CaveRandomByte(memory, cpu, 0x9142u);                  /* 9142 */
        OpCmpValue(cpu, 0xaeu);
        if (cpu->carry) {
            CaveChestFromList(memory, cpu, 0x0000u, 0x5du);    /* 916F */
            goto next;
        }
        OpCmpValue(cpu, 0x81u);
        if (cpu->carry) {
            CaveChestFromList(memory, cpu, 0x1000u, 0x60u);    /* 9176 */
            goto next;
        }
        OpCmpValue(cpu, 0x63u);
        if (cpu->carry) {
            CaveSpellChest(memory, cpu);
            goto next;
        }
        OpCmpValue(cpu, 0x5eu);
        if (cpu->carry) {
            LoadA8(cpu, 0x29u);                                /* 91C8 */
            Lufia2CaveRandomIndex(memory, cpu, 0x91cau);
            CaveChestFromTable(memory, cpu, 0x94eea0u, 0x40u);
            goto next;
        }
        OpCmpValue(cpu, 0x24u);
        if (cpu->carry) {
            CaveCommonChest(memory, cpu);                      /* 91AD */
            goto next;
        }
        LoadA8(cpu, 0x1fu);                                    /* 915A */
        Lufia2CaveRandomIndex(memory, cpu, 0x915cu);
        CaveChestFromTable(memory, cpu, 0x94f13du, 0);
next:
        OpIny(cpu);                                            /* 91DD */
        OpIny(cpu);
        OpCpy(cpu, 0x0010u);
    } while (!cpu->carry);
}

/* $83:91E7-$83:9265: up to 7 room rectangles. */
static void CavePlaceRooms(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);                                         /* 91E7 */
    OpSetDataBank(memory, cpu, 0x83u);
    LoadA8(cpu, 0x01u);
    OpSta(memory, cpu, OpDp(cpu, 0x5au));
    LoadA8(cpu, 0x04u);
    OpSta(memory, cpu, OpDp(cpu, 0x5bu));
    OpStz(memory, cpu, OpDp(cpu, 0x56u));
    OpStz(memory, cpu, OpDp(cpu, 0x63u));
    for (;;) {
        OpLda(memory, cpu, OpDp(cpu, 0x5au));                  /* 91F9 */
        OpSta(memory, cpu, OpDp(cpu, 0x26u));
        LoadA8(cpu, 0x03u);
        Lufia2CaveRandomBelow(memory, cpu, 0x91ffu);
        OpDecA(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, 0x5bu));
        OpSta(memory, cpu, OpDp(cpu, 0x27u));
        LoadA8(cpu, 0x04u);
        Lufia2CaveRandomMean(memory, cpu, 0x920au);
        OpIncA(cpu);
        OpSta(memory, cpu, OpDp(cpu, 0x24u));
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, 0x26u));
        OpCmpValue(cpu, 0x09u);
        if (!cpu->carry) {
            LoadA8(cpu, 0x04u);                                /* 9217 */
            Lufia2CaveRandomMean(memory, cpu, 0x9219u);
            OpIncA(cpu);
            OpSta(memory, cpu, OpDp(cpu, 0x25u));
            cpu->carry = 0;
            OpAdc(memory, cpu, OpDp(cpu, 0x27u));
            OpCmpValue(cpu, 0x0fu);
            if (!cpu->carry) {
                OpLda(memory, cpu, OpDp(cpu, 0x25u));          /* 9226 */
                OpCmp(memory, cpu, OpDp(cpu, 0x63u));
                if (cpu->carry)
                    OpSta(memory, cpu, OpDp(cpu, 0x63u));
                OpLda(memory, cpu, OpDp(cpu, 0x24u));          /* 922E */
                cpu->carry = 0;
                OpAdc(memory, cpu, OpDp(cpu, 0x25u));
                OpCmpValue(cpu, 0x03u);
                if (!cpu->carry)
                    OpStepMem(memory, cpu, OpDp(cpu, 0x24u), 1);
                OpLda(memory, cpu, OpDp(cpu, 0x56u));          /* 9239 */
                OpIncA(cpu);
                OpSta(memory, cpu, OpDp(cpu, 0x56u));
                OpSta(memory, cpu, OpDp(cpu, 0x54u));
                OpCmpValue(cpu, 0x08u);
                if (cpu->carry)
                    return;                                    /* 9266 */
                Lufia2CaveFillRoom(memory, cpu, 0x9244u);
            }
        }
        OpLda(memory, cpu, OpDp(cpu, 0x5au));                  /* 9247 */
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, 0x24u));
        OpSta(memory, cpu, OpDp(cpu, 0x5au));
        OpCmpValue(cpu, 0x09u);
        if (!cpu->carry)
            continue;
        LoadA8(cpu, 0x01u);                                    /* 9252 */
        OpSta(memory, cpu, OpDp(cpu, 0x5au));
        LoadA8(cpu, 0x04u);
        Lufia2CaveRandomMean(memory, cpu, 0x9258u);
        OpIncA(cpu);
        OpIncA(cpu);
        cpu->carry = 0;
        OpAdc(memory, cpu, OpDp(cpu, 0x5bu));
        OpSta(memory, cpu, OpDp(cpu, 0x5bu));
        OpCmpValue(cpu, 0x0fu);
        if (cpu->carry)
            return;
    }
}

/* Corridor bit for one cell pair. */
static void CaveOpenCorridor(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint8_t id_offset,
    uint16_t opposite, uint16_t pair, uint16_t mark) {
    OpSta(memory, cpu, OpDp(cpu, id_offset));
    OpLda(memory, cpu, OpAbsX(cpu, opposite));
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, id_offset));
        OpOraValue(cpu, 0x40u);
        OpSta(memory, cpu, OpAbsX(cpu, pair));
        OpSta(memory, cpu, OpAbsX(cpu, mark));
    } else if (!cpu->negative) {
        return;
    } else {
        OpSta(memory, cpu, OpAbsX(cpu, mark));
    }
}

/* $83:9266-$83:9387: merge and link rooms, open corridors. */
static void CaveLinkFloor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSetDataBank(memory, cpu, 0x7fu);                         /* 9266 */
    LoadA8(cpu, 0x08u);
    OpSta(memory, cpu, OpDp(cpu, 0x2au));
    do {
        Lufia2CaveMergeRoom(memory, cpu, 0x926eu);             /* 926E */
        OpStepMem(memory, cpu, OpDp(cpu, 0x2au), -1);
    } while (!cpu->zero);
    Lufia2CaveCountCells(memory, cpu, 0x9275u);
    Lufia2CavePickCell(memory, cpu, 0x9278u);
    OpWriteX(memory, cpu, OpAbs(cpu, CAVE_START_COLUMN), cpu->x);
    Lufia2CaveLinkRooms(memory, cpu, 0x927eu);
    OpLdx(cpu, 0x00ffu);                                       /* 9281 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));              /* 9284 */
        if (cpu->zero)
            goto next;
        OpBitValue(cpu, 0xc0u);
        if (!cpu->zero)
            goto next;
        OpSta(memory, cpu, OpDp(cpu, 0x54u));                  /* 9290 */
        OpLda(memory, cpu, OpAbsX(cpu, 0xea01u));
        if (cpu->zero) {
            /* $92A9: empty right neighbour beside a room below or above. */
            OpLda(memory, cpu, OpAbsX(cpu, 0xe9f1u));
            if (cpu->zero)
                OpLda(memory, cpu, OpAbsX(cpu, 0xea11u));
            if (!cpu->zero) {
                OpLda(memory, cpu, OpDp(cpu, 0x54u));          /* 92B6 */
                OpOraValue(cpu, 0x40u);
                OpSta(memory, cpu, OpAbsX(cpu, 0xea01u));
            }
            goto next;
        }
        OpLda(memory, cpu, OpAbsX(cpu, 0xe9ffu));              /* 9297 */
        if (cpu->zero) {
            OpLda(memory, cpu, OpAbsX(cpu, 0xe9efu));          /* 92C0 */
            if (cpu->zero)
                OpLda(memory, cpu, OpAbsX(cpu, 0xea0fu));
            if (!cpu->zero) {
                OpLda(memory, cpu, OpDp(cpu, 0x54u));          /* 92CD */
                OpOraValue(cpu, 0x40u);
                OpSta(memory, cpu, OpAbsX(cpu, 0xe9ffu));
            }
            goto next;
        }
        OpLda(memory, cpu, OpAbsX(cpu, 0xea10u));              /* 929C */
        if (cpu->zero) {
            OpTxa(cpu);                                        /* 92D7 */
            OpAndValue(cpu, 0xf0u);
            OpCmpValue(cpu, 0xe0u);
            if (cpu->carry)
                goto next;
            OpLda(memory, cpu, OpAbsX(cpu, 0xea01u));
            if (cpu->negative) {
                CaveOpenCorridor(memory, cpu, 0x55u, 0xea11u,  /* 9304 */
                    0xea11u, 0xea10u);
            } else {
                OpLda(memory, cpu, OpAbsX(cpu, 0xe9ffu));      /* 92E3 */
                if (cpu->negative)
                    CaveOpenCorridor(memory, cpu, 0x55u, 0xea0fu,
                        0xea0fu, 0xea10u);                     /* 92EA */
            }
            goto next;
        }
        OpLda(memory, cpu, OpAbsX(cpu, 0xe9f0u));              /* 92A1 */
        if (cpu->zero) {
            OpTxa(cpu);                                        /* 931E */
            OpAndValue(cpu, 0xf0u);
            OpCmpValue(cpu, 0x30u);
            if (!cpu->carry)
                goto next;
            OpLda(memory, cpu, OpAbsX(cpu, 0xea01u));
            if (cpu->negative) {
                CaveOpenCorridor(memory, cpu, 0x55u, 0xe9f1u,  /* 934B */
                    0xe9f1u, 0xe9f0u);
            } else {
                OpLda(memory, cpu, OpAbsX(cpu, 0xe9ffu));      /* 932A */
                if (cpu->negative)
                    CaveOpenCorridor(memory, cpu, 0x55u, 0xe9efu,
                        0xe9efu, 0xe9f0u);                     /* 9331 */
            }
        }
next:
        OpDex(cpu);                                            /* 9363 */
        OpCpx(cpu, 0x0010u);
    } while (cpu->carry);
    Lufia2CaveClearVisited(memory, cpu, 0x936cu);
    Lufia2CaveLinkRooms(memory, cpu, 0x936fu);
    OpLdx(cpu, 0x00ffu);                                       /* 9372 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        if (!cpu->zero) {
            if (!cpu->negative)
                TransferDirectToA(cpu);                        /* 937C */
            OpAndValue(cpu, 0x3fu);
            OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        }
        OpDex(cpu);                                            /* 9382 */
        OpCpx(cpu, 0x0010u);
    } while (cpu->carry);
}

/* $83:9388-$83:940D: keep one random link per room pair. */
static void CaveDedupeLinks(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x0000u);                                       /* 9388 */
    for (;;) {
        OpCompareIndex(cpu, cpu->x,                            /* 938B */
            OpReadX(memory, cpu, OpDp(cpu, 0x2du)));
        if (cpu->carry)
            break;
        OpWriteX(memory, cpu, OpDp(cpu, 0x5au), cpu->x);
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, 0xe6f1u));
        if (!cpu->zero) {
            OpSta(memory, cpu, OpAbs(cpu, 0x0400u));           /* 9397 */
            OpTay(cpu);
            OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));
            OpSta(memory, cpu, OpDp(cpu, 0x54u));
            OpLda(memory, cpu, OpAbsY(cpu, 0xea10u));
            OpSta(memory, cpu, OpDp(cpu, 0x55u));
            LoadA8(cpu, 0x01u);
            OpSta(memory, cpu, OpDp(cpu, 0x56u));
            OpStz(memory, cpu, OpDp(cpu, 0x57u));
            TransferDirectToA(cpu);
            for (;;) {
                OpInx(cpu);                                    /* 93AC */
                OpCompareIndex(cpu, cpu->x,
                    OpReadX(memory, cpu, OpDp(cpu, 0x2du)));
                if (cpu->carry)
                    break;
                OpLda(memory, cpu, OpAbsX(cpu, 0xe6f1u));
                OpTay(cpu);
                OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));
                OpCmp(memory, cpu, OpDp(cpu, 0x54u));
                if (!cpu->zero)
                    continue;
                OpLda(memory, cpu, OpAbsY(cpu, 0xea10u));
                OpCmp(memory, cpu, OpDp(cpu, 0x55u));
                if (!cpu->zero)
                    continue;
                OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x56u)));  /* 93C3 */
                OpLda(memory, cpu, OpAbsX(cpu, 0xe6f1u));
                OpSta(memory, cpu, OpAbsY(cpu, 0x0400u));
                OpIny(cpu);
                OpWriteX(memory, cpu, OpDp(cpu, 0x56u), cpu->y);
            }
            OpLda(memory, cpu, OpDp(cpu, 0x56u));              /* 93D0 */
            OpCmpValue(cpu, 0x01u);
            if (!cpu->zero) {
                Lufia2CaveRandomBelow(memory, cpu, 0x93d6u);
                ExchangeAccumulatorBytes(cpu);
                LoadA8(cpu, 0x00u);
                ExchangeAccumulatorBytes(cpu);
                OpTax(cpu);
                LoadA8(cpu, 0xffu);
                OpSta(memory, cpu, OpAbsX(cpu, 0x0400u));
                OpLdx(cpu, 0x0000u);                           /* 93E3 */
                do {
                    OpLda(memory, cpu, OpAbsX(cpu, 0xe6f1u));
                    OpLdy(cpu, 0x0000u);
                    do {
                        OpCmp(memory, cpu, OpAbsY(cpu, 0x0400u));  /* 93EC */
                        if (cpu->zero) {
                            OpStz(memory, cpu, OpAbsX(cpu, 0xe6f1u));
                            break;
                        }
                        OpIny(cpu);
                        OpCompareIndex(cpu, cpu->y,
                            OpReadX(memory, cpu, OpDp(cpu, 0x56u)));
                    } while (!cpu->carry);
                    OpInx(cpu);                                /* 93FB */
                    OpCompareIndex(cpu, cpu->x,
                        OpReadX(memory, cpu, OpDp(cpu, 0x2du)));
                } while (!cpu->carry);
            }
        }
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x5au)));    /* 9400 */
        OpInx(cpu);
    }
    TransferDirectToA(cpu);                                    /* 9405 */
    OpLda(memory, cpu, OpDp(cpu, 0x2du));
    OpTay(cpu);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsY(cpu, 0xe6f1u));
}

/* $83:940E-$83:949C: start cell, link marks and stairs. */
static void CavePlaceStart(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Lufia2CaveCountCells(memory, cpu, 0x940eu);                /* 940E */
    Lufia2CavePickCell(memory, cpu, 0x9411u);
    Lufia2CaveCellPosition(memory, cpu, 0x9414u);
    OpIncA(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, 0x0005b1u);
    OpSta(memory, cpu, CAVE_START_ROW_LONG);
    ExchangeAccumulatorBytes(cpu);
    OpIncA(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, 0x0005b0u);
    OpSta(memory, cpu, CAVE_START_COLUMN_LONG);
    OpLdx(cpu, 0x0000u);                                       /* 942D */
    for (;;) {
        TransferDirectToA(cpu);                                /* 9430 */
        OpLda(memory, cpu, OpAbsX(cpu, 0xe6f1u));
        if (!cpu->zero) {
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero)
                break;
            OpTay(cpu);                                        /* 943A */
            OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));
            OpOraValue(cpu, 0x80u);
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));
            OpLda(memory, cpu, OpAbsY(cpu, 0xea10u));
            OpOraValue(cpu, 0x80u);
            OpSta(memory, cpu, OpAbsY(cpu, 0xea10u));
        }
        OpInx(cpu);                                            /* 944B */
    }
    Lufia2CavePickCell(memory, cpu, 0x944eu);                  /* 944E */
    if (!cpu->carry) {
        Lufia2CaveClearVisited(memory, cpu, 0x9453u);
        Lufia2CavePickCell(memory, cpu, 0x9456u);
    }
    Lufia2CaveCellPosition(memory, cpu, 0x9459u);              /* 9459 */
    Lufia2CaveStairOffset(memory, cpu, 0x945cu);
    OpSta(memory, cpu, CAVE_STAIR_ROW_LONG);
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, CAVE_STAIR_COLUMN_LONG);
    LoadA8(cpu, 0xffu);
    OpSta(memory, cpu, 0x7fe6adu);
    OpSta(memory, cpu, 0x7fe6aeu);
    CaveRandomByte(memory, cpu, 0x9472u);
    OpCmpValue(cpu, 0x10u);
    if (!cpu->carry) {
        Lufia2CavePickCell(memory, cpu, 0x947au);
        if (cpu->carry) {
            Lufia2CaveCellPosition(memory, cpu, 0x947fu);      /* 947F */
            Lufia2CaveStairOffset(memory, cpu, 0x9482u);
            OpSta(memory, cpu, 0x7fe6aeu);
            ExchangeAccumulatorBytes(cpu);
            OpIncA(cpu);
            OpSta(memory, cpu, 0x7fe6adu);
        }
    }
    Lufia2CaveClearVisited(memory, cpu, 0x948fu);              /* 948F */
    OpLdx(cpu, 0x0013u);
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpLongX(cpu, CAVE_OBJECT_COLUMNS_LONG));           /* 9496 */
        OpDex(cpu);
    } while (!cpu->negative);
}

/* $83:949D-$83:9514: rare 2x2 treasure room. */
static void CaveTreasureRoom(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    CaveRandomByte(memory, cpu, 0x949du);                      /* 949D */
    OpCmpValue(cpu, 0x10u);
    if (cpu->carry)
        return;
    LoadA8(cpu, 0xeau);
    OpSta(memory, cpu, OpDp(cpu, 0x57u));
    OpLdy(cpu, 0x0010u);
    for (;;) {
        OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));              /* 94AC */
        if (!cpu->zero &&
            (OpCmp(memory, cpu, OpAbsY(cpu, 0xea01u)), cpu->zero) &&
            (OpCmp(memory, cpu, OpAbsY(cpu, 0xea10u)), cpu->zero) &&
            (OpCmp(memory, cpu, OpAbsY(cpu, 0xea11u)), cpu->zero) &&
            (CaveRandomByte(memory, cpu, 0x94c0u),
             OpCmpValue(cpu, 0x80u), !cpu->carry)) {
            OpTya(cpu);                                        /* 94C8 */
            OpSta(memory, cpu, OpAbs(cpu, 0xe6a8u));
            Lufia2CaveCellPosition(memory, cpu, 0x94ccu);
            OpSta(memory, cpu, OpDp(cpu, 0x5bu));
            ExchangeAccumulatorBytes(cpu);
            OpSta(memory, cpu, OpDp(cpu, 0x5au));
            OpLdx(cpu, 0x000cu);
            do {
                OpPushX(memory, cpu);                          /* 94D7 */
                LoadA8(cpu, 0x08u);
                Lufia2CaveRandomBelow(memory, cpu, 0x94dau);
                cpu->carry = 0;
                OpAdc(memory, cpu, OpDp(cpu, 0x5au));
                OpAdcValue(cpu, 0x01u);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
                LoadA8(cpu, 0x08u);
                Lufia2CaveRandomBelow(memory, cpu, 0x94e6u);
                cpu->carry = 0;
                OpAdc(memory, cpu, OpDp(cpu, 0x5bu));
                OpAdcValue(cpu, 0x03u);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
                Lufia2CaveNearStartOrPlaced(memory, cpu, 0x94f0u);
                if (!cpu->carry)
                    Lufia2CaveAddObject(memory, cpu, 0x94f5u);
                OpPullX(memory, cpu);                          /* 94F8 */
                OpDex(cpu);
            } while (!cpu->negative);
            OpLdx(cpu, 0x0008u);                               /* 94FC */
            do {
                OpPushX(memory, cpu);
                OpLda(memory, cpu, OpAbs(cpu, 0xe6a8u));
                Lufia2CaveAddChest(memory, cpu, 0x9503u);
                OpPullX(memory, cpu);
                OpDex(cpu);
            } while (!cpu->zero);
            break;
        }
        OpIny(cpu);                                            /* 950C */
        OpCpy(cpu, 0x00c0u);
        if (cpu->carry)
            break;
    }
    Lufia2CaveClearVisited(memory, cpu, 0x9512u);              /* 9512 */
}

/* $83:9515-$83:959C: 4-7 random objects, then chests in 2x2 rooms. */
static void CaveObjectsAndChests(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x04u);                                        /* 9515 */
    Lufia2CaveRandomBelow(memory, cpu, 0x9517u);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x04u);
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    OpStz(memory, cpu, OpDp(cpu, 0x23u));
    OpLdx(cpu, 0x0000u);
    do {
        LoadA8(cpu, 0x02u);                                    /* 9524 */
        Lufia2CaveRandomBelow(memory, cpu, 0x9526u);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
        LoadA8(cpu, 0x02u);
        Lufia2CaveRandomBelow(memory, cpu, 0x952du);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
        OpWriteX(memory, cpu, OpDp(cpu, 0x24u), cpu->x);
        Lufia2CavePickCell(memory, cpu, 0x9534u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x24u)));
        if (cpu->carry) {
            Lufia2CaveCellPosition(memory, cpu, 0x953bu);      /* 953B */
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x24u)));
            cpu->carry = 1;
            OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            OpIncA(cpu);
            OpIncA(cpu);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            ExchangeAccumulatorBytes(cpu);
            cpu->carry = 1;
            OpAdc(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            Lufia2CaveNearStartOrPlaced(memory, cpu, 0x954du);
            if (!cpu->carry)
                Lufia2CaveAddObject(memory, cpu, 0x9552u);
        }
        OpInx(cpu);                                            /* 9555 */
        OpCompareIndex(cpu, cpu->x, OpReadX(memory, cpu, OpDp(cpu, 0x22u)));
    } while (!cpu->carry);
    OpLdx(cpu, 0x0010u);                                       /* 955A */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));              /* 955D */
        if (!cpu->zero && !cpu->negative &&
            (OpCmp(memory, cpu, OpAbsX(cpu, 0xea01u)), cpu->zero) &&
            (OpCmp(memory, cpu, OpAbsX(cpu, 0xea10u)), cpu->zero) &&
            (OpCmp(memory, cpu, OpAbsX(cpu, 0xea11u)), cpu->zero)) {
            OpOraValue(cpu, 0x80u);                            /* 9573 */
            OpSta(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
            OpSta(memory, cpu, OpAbsX(cpu, 0xea01u));
            OpSta(memory, cpu, OpAbsX(cpu, 0xea10u));
            OpSta(memory, cpu, OpAbsX(cpu, 0xea11u));
            OpTxa(cpu);
            Lufia2CaveAddChest(memory, cpu, 0x9582u);
            if (!cpu->carry) {
                OpLda(memory, cpu, OpAbs(cpu, CAVE_CHEST_COUNT));       /* 9587 */
                OpCmpValue(cpu, 0x08u);
                if (cpu->carry)
                    break;
                /* Never ends the loop: $E734 reaches 8 first. */
                OpLda(memory, cpu, OpDp(cpu, 0x59u));
                OpIncA(cpu);
                OpSta(memory, cpu, OpDp(cpu, 0x59u));
                OpCmpValue(cpu, 0x08u);
                if (cpu->carry)
                    break;
            }
        }
        OpInx(cpu);                                            /* 9597 */
        OpCpx(cpu, 0x00f0u);
    } while (!cpu->carry);
    Lufia2CaveClearVisited(memory, cpu, 0x959du);              /* 959D */
}

/* $83:95A0-$83:9653: block shape of each cell. */
static void CaveShapeCells(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t kNeighbours[8] = {
        0xea11u, 0xea01u, 0xe9f1u, 0xea10u, 0xe9f0u, 0xea0fu, 0xe9ffu, 0xe9efu,
    };
    static const uint8_t kSide[4] = {0x80u, 0x04u, 0x01u, 0x20u};
    static const uint8_t kCorner[4] = {0x50u, 0x12u, 0x0au, 0x48u};

    LoadA8(cpu, 0x01u);                                        /* 95A0 */
    OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
    OpStz(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
    Lufia2CaveCellIndex(memory, cpu, 0x95a6u);
    OpTxy(cpu);
    do {
        OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));              /* 95AA */
        if (!cpu->zero) {
            OpSta(memory, cpu, OpDp(cpu, 0x54u));
            OpStz(memory, cpu, OpDp(cpu, 0x55u));
            OpLda(memory, cpu, OpDp(cpu, 0x54u));
            for (unsigned i = 0; i < 8; ++i) {
                if (i == 5)
                    OpLda(memory, cpu, OpDp(cpu, 0x54u));      /* 95E0 */
                OpCmp(memory, cpu, OpAbsY(cpu, kNeighbours[i]));
                if (!cpu->zero)
                    cpu->carry = 0;
                OpRolMem8(memory, cpu, OpDp(cpu, 0x55u));
            }
            for (unsigned i = 0; i < 4; ++i) {
                OpLda(memory, cpu, OpDp(cpu, 0x55u));          /* 95FA */
                if (i == 0) {
                    if (cpu->negative) {
                        OpAndValue(cpu, kCorner[i]);
                        if (cpu->zero)
                            OpTestBits(memory, cpu, OpDp(cpu, 0x55u), 0);
                    }
                    continue;
                }
                OpBitValue(cpu, kSide[i]);
                if (!cpu->zero) {
                    OpAndValue(cpu, kCorner[i]);
                    if (cpu->zero)
                        OpTestBits(memory, cpu, OpDp(cpu, 0x55u), 0);
                }
            }
            TransferDirectToA(cpu);                            /* 9628 */
            OpLda(memory, cpu, OpDp(cpu, 0x55u));
            OpTax(cpu);
            OpLda(memory, cpu, OpLongX(cpu, 0x93d59bu));
            OpSta(memory, cpu, OpAbsY(cpu, CAVE_SHAPE_GRID));
        }
        OpIny(cpu);                                            /* 9633 */
        OpCpy(cpu, 0x00f0u);
    } while (!cpu->carry);
    OpLdy(cpu, 0x0010u);                                       /* 963C */
    OpLdx(cpu, 0x0000u);
    do {
        OpTxa(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x33u);
        OpSta(memory, cpu, OpAbsY(cpu, CAVE_SHAPE_GRID));
        OpTya(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x10u);
        OpTay(cpu);
        OpInx(cpu);
        OpCpx(cpu, 0x0006u);
    } while (!cpu->carry);
}

/* $83:9654-$83:9692: block set, cleared map, templates. */
static void CaveTileMapBase(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x30u);                                         /* 9654 */
    OpLda(memory, cpu, OpAbs(cpu, 0xe699u));
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    LoadA16(cpu, 0x4000u);
    OpSta(memory, cpu, OpDp(cpu, 0x60u));
    OpSepWidths(cpu, 0x20u);
    LoadA8(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    CaveCallLong(memory, cpu, 0x9666u, Lufia2DecompressResource);
    OpRepWidths(cpu, 0x20u);                                         /* 966A */
    OpStz(memory, cpu, OpAbs(cpu, 0x0000u));
    OpLdx(cpu, 0x0000u);
    OpLdy(cpu, 0x0002u);
    LoadA16(cpu, 0x7e1au);
    OpMoveNext(memory, cpu, 0x7fu, 0x7fu);
    OpLdx(cpu, 0x9f3fu);                                       /* 967B */
    OpLdy(cpu, 0x0000u);
    LoadA16(cpu, 0x0009u);
    OpMoveNext(memory, cpu, 0x7fu, 0x83u);
    OpLdx(cpu, 0x9f45u);                                       /* 9687 */
    OpLdy(cpu, 0x3f0au);
    LoadA16(cpu, 0x0003u);
    OpMoveNext(memory, cpu, 0x7fu, 0x83u);
}

/* $83:9693-$83:96F4: draw each cell's block, then the link blocks ($39). */
static void CaveDrawCells(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);                                         /* 9693 */
    LoadA8(cpu, 0x10u);
    OpSta(memory, cpu, OpDp(cpu, 0x23u));
    LoadA8(cpu, 0x0eu);
    OpSta(memory, cpu, OpDp(cpu, 0x27u));
    OpLdx(cpu, 0x0010u);
    do {
        LoadA8(cpu, 0x10u);                                    /* 96A0 */
        OpSta(memory, cpu, OpDp(cpu, 0x26u));
        do {
            OpLda(memory, cpu, OpLongX(cpu, CAVE_SHAPE_GRID_LONG));       /* 96A4 */
            OpCmpValue(cpu, 0x1cu);
            if (!cpu->zero) {
                OpSta(memory, cpu, OpDp(cpu, 0x22u));
                OpPushX(memory, cpu);
                OpLda(memory, cpu, OpDp(cpu, 0x23u));
                Lufia2CaveCellPosition(memory, cpu, 0x96b1u);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
                ExchangeAccumulatorBytes(cpu);
                OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
                Lufia2CaveTileOffsetY2(memory, cpu, 0x96b9u);
                Lufia2CaveDrawBlock(memory, cpu, 0x96bcu);
                OpPullX(memory, cpu);
            }
            OpStepMem(memory, cpu, OpDp(cpu, 0x23u), 1);       /* 96C0 */
            OpInx(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 0x26u), -1);
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpDp(cpu, 0x27u), -1);
    } while (!cpu->zero);
    OpLdx(cpu, 0x0000u);                                       /* 96CB */
    for (;;) {
        OpLda(memory, cpu, OpAbsX(cpu, 0xe6f1u));              /* 96CE */
        if (!cpu->zero) {
            OpCmpValue(cpu, 0xffu);
            if (cpu->zero)
                break;
            OpPushX(memory, cpu);                              /* 96D7 */
            OpLda(memory, cpu, OpAbsX(cpu, 0xe6f1u));
            Lufia2CaveCellPosition(memory, cpu, 0x96dbu);
            cpu->carry = 0;
            OpAdcValue(cpu, 0x04u);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            ExchangeAccumulatorBytes(cpu);
            OpIncA(cpu);
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            Lufia2CaveTileOffsetY2(memory, cpu, 0x96e7u);
            LoadA8(cpu, 0x39u);
            OpSta(memory, cpu, OpDp(cpu, 0x22u));
            Lufia2CaveDrawBlock(memory, cpu, 0x96eeu);
            OpPullX(memory, cpu);
        }
        OpInx(cpu);                                            /* 96F2 */
    }
}

/* JSL $80:BFAA from bank $83; 0 = handoff at $80:BFBC. */
static uint8_t CaveListSearch(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    return Lufia2FieldListSearch(memory, cpu, 0x83u, (uint16_t)(site + 3u));
}

/* $83:96F5-$83:9752: tile sets of the $0A-key entries. */
static uint8_t CaveReadTileSets(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x0000u);                                       /* 96F5 */
    OpStz(memory, cpu, OpDp(cpu, 0x56u));
    OpStz(memory, cpu, OpDp(cpu, 0x57u));
    do {
        OpPushX(memory, cpu);                                  /* 96FC */
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x56u)));
        LoadA8(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsY(cpu, 0xc000u));
        OpSta(memory, cpu, OpAbsY(cpu, 0xc001u));
        LoadA8(cpu, 0x05u);
        ExchangeAccumulatorBytes(cpu);
        OpTxa(cpu);
        OpLdx(cpu, 0x000au);
        if (!CaveListSearch(memory, cpu, 0x970eu))
            return 0;
        if (!cpu->carry) {
            OpLda(memory, cpu, OpLongX(cpu, 0x7ef001u));       /* 9714 */
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            OpLda(memory, cpu, OpLongX(cpu, 0x7ef002u));
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            Lufia2CaveBlockOffsetX(memory, cpu, 0x9720u);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpLongX(cpu, 0x7e400cu));
            OpSta(memory, cpu, OpAbsY(cpu, 0xc100u));
            OpLda(memory, cpu, OpLongX(cpu, 0x7e400eu));
            OpSta(memory, cpu, OpAbsY(cpu, 0xc200u));
            OpLda(memory, cpu, OpLongX(cpu, 0x7e4010u));
            OpSta(memory, cpu, OpAbsY(cpu, 0xc300u));
            OpLda(memory, cpu, OpLongX(cpu, 0x7e400au));
            OpSta(memory, cpu, OpAbsY(cpu, 0xc000u));
            OpTyx(cpu);                                        /* 9741 */
            OpSta(memory, cpu, OpLongX(cpu, 0x000022u));
            OpIny(cpu);
            OpIny(cpu);
            OpWriteX(memory, cpu, OpDp(cpu, 0x56u), cpu->y);
            OpSepWidths(cpu, 0x20u);
        }
        OpPullX(memory, cpu);                                  /* 974C */
        OpInx(cpu);
        OpCpx(cpu, 0x0004u);
    } while (!cpu->carry);
    return 1;
}

/* $83:9753-$83:97E7: apply each cell's tile set. */
static void CaveApplyTileSets(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x0010u);                                       /* 9753 */
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));              /* 9756 */
        if (cpu->zero)
            goto next;
        OpSta(memory, cpu, OpDp(cpu, 0x56u));
        OpPushX(memory, cpu);
        OpTxa(cpu);
        Lufia2CaveCellPosition(memory, cpu, 0x9762u);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
        Lufia2CaveTileOffsetY(memory, cpu, 0x976au);
        OpLda(memory, cpu, OpDp(cpu, 0x56u));
        OpAndValue(cpu, 0x03u);
        if (!cpu->zero) {
            static const uint8_t kKeys[4] = {0x22u, 0x24u, 0x26u, 0x28u};
            static const uint8_t kTiles[4] = {0x11u, 0x13u, 0x15u, 0x17u};

            ExchangeAccumulatorBytes(cpu);                     /* 9773 */
            LoadA8(cpu, 0x00u);
            OpTax(cpu);
            OpRepWidths(cpu, 0x20u);
            for (unsigned i = 0; i < 4; ++i) {
                OpLda(memory, cpu, OpAbsX(cpu, (uint16_t)(0xc000u + 2u * i)));
                OpSta(memory, cpu, OpDp(cpu, kTiles[i]));
            }
            LoadA16(cpu, 0x0006u);
            OpSta(memory, cpu, OpDp(cpu, 0x58u));
            for (;;) {
                LoadA16(cpu, 0x0006u);                         /* 9792 */
                OpSta(memory, cpu, OpDp(cpu, 0x56u));
                for (;;) {
                    OpLda(memory, cpu, OpAbsY(cpu, 0x000au));  /* 9797 */
                    OpAndValue(cpu, 0x03ffu);
                    for (unsigned i = 0; i < 4; ++i) {
                        OpCmp(memory, cpu, OpDp(cpu, kKeys[i]));
                        if (cpu->zero) {
                            OpLda(memory, cpu, OpDp(cpu, kTiles[i]));
                            OpSta(memory, cpu, OpDp(cpu, 0x54u));  /* 97BB */
                            OpLda(memory, cpu, OpAbsY(cpu, 0x000au));
                            OpAndValue(cpu, 0xfc00u);
                            OpOra(memory, cpu, OpDp(cpu, 0x54u));
                            OpSta(memory, cpu, OpAbsY(cpu, 0x000au));
                            break;
                        }
                    }
                    OpStepMem(memory, cpu, OpDp(cpu, 0x56u), -1);  /* 97C8 */
                    if (cpu->zero)
                        break;
                    OpIny(cpu);
                    OpIny(cpu);
                }
                OpStepMem(memory, cpu, OpDp(cpu, 0x58u), -1);  /* 97D0 */
                if (cpu->zero)
                    break;
                OpTya(cpu);
                cpu->carry = 0;
                OpAdcValue(cpu, 0x00b6u);
                OpTay(cpu);
                if (cpu->zero)
                    break;
            }
        }
        OpSepWidths(cpu, 0x20u);                                     /* 97DC */
        OpPullX(memory, cpu);
next:
        OpInx(cpu);                                            /* 97DF */
        OpCpx(cpu, 0x00f0u);
    } while (!cpu->carry);
}

/* $83:97E8-$83:9868: decoration sets into $7F:C000. */
static uint8_t CaveReadDecorations(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, 0x56u));                      /* 97E8 */
    OpStz(memory, cpu, OpDp(cpu, 0x57u));
    OpLdx(cpu, 0x0003u);
    do {
        OpPushX(memory, cpu);                                  /* 97EF */
        LoadA8(cpu, 0x0au);
        ExchangeAccumulatorBytes(cpu);
        OpTxa(cpu);
        OpLdx(cpu, 0x0004u);
        if (!CaveListSearch(memory, cpu, 0x97f7u))
            return 0;
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 0x56u)));    /* 97FB */
        if (cpu->carry) {
            OpRepWidths(cpu, 0x20u);                                 /* 97FF */
            TransferDirectToA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, 0xc000u));
            LoadA16(cpu, 0xffffu);
            OpSta(memory, cpu, OpAbsY(cpu, 0xc100u));
        } else {
            OpLda(memory, cpu, OpLongX(cpu, 0x7ef002u));       /* 980D */
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
            OpLda(memory, cpu, OpLongX(cpu, 0x7ef003u));
            OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
            OpLda(memory, cpu, OpLongX(cpu, 0x7ef008u));
            OpSta(memory, cpu, OpAbsY(cpu, 0xc000u));
            OpSta(memory, cpu, OpDp(cpu, 0x58u));
            OpStz(memory, cpu, OpDp(cpu, 0x59u));
            OpLda(memory, cpu, OpLongX(cpu, 0x7ef009u));
            OpDecA(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, 0xc001u));
            Lufia2CaveBlockOffsetX(memory, cpu, 0x982cu);
            OpRepWidths(cpu, 0x20u);
            do {
                OpTya(cpu);                                    /* 9831 */
                OpAdcValue(cpu, 0x0100u);                      /* no CLC */
                OpTay(cpu);
                OpLda(memory, cpu, OpLongX(cpu, 0x7e400au));
                OpSta(memory, cpu, OpAbsY(cpu, 0xc000u));
                OpLda(memory, cpu, OpLongX(cpu, 0x7e406au));
                OpSta(memory, cpu, OpAbsY(cpu, 0xc002u));
                OpLda(memory, cpu, OpLongX(cpu, 0x7e4fceu));
                OpSta(memory, cpu, OpAbsY(cpu, 0xc004u));
                OpLda(memory, cpu, OpLongX(cpu, 0x7e502eu));
                OpSta(memory, cpu, OpAbsY(cpu, 0xc006u));
                OpInx(cpu);
                OpInx(cpu);
                OpStepMem(memory, cpu, OpDp(cpu, 0x58u), -1);
            } while (!cpu->zero);
        }
        OpLda(memory, cpu, OpDp(cpu, 0x56u));                  /* 9858 */
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0008u);
        OpSta(memory, cpu, OpDp(cpu, 0x56u));
        OpSepWidths(cpu, 0x20u);
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpCpx(cpu, 0x0008u);
    } while (!cpu->carry);
    return 1;
}

/* $83:9869-$83:992F: random tile decorations. */
static void CaveDecorate(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x0010u);                                       /* 9869 */
    do {
        OpPushX(memory, cpu);                                  /* 986C */
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_ROOM_GRID));
        if (cpu->zero)
            goto next;
        OpTxa(cpu);                                            /* 9875 */
        Lufia2CaveCellPosition(memory, cpu, 0x9876u);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_ROW));
        ExchangeAccumulatorBytes(cpu);
        OpSta(memory, cpu, OpDp(cpu, CAVE_DP_TILE_COLUMN));
        Lufia2CaveTileOffsetY(memory, cpu, 0x987eu);
        OpRepWidths(cpu, 0x20u);
        LoadA16(cpu, 0x0006u);
        OpSta(memory, cpu, OpDp(cpu, 0x58u));
        for (;;) {
            LoadA16(cpu, 0x0006u);                             /* 9888 */
            OpSta(memory, cpu, OpDp(cpu, 0x56u));
            for (;;) {
                CaveRandomByte(memory, cpu, 0x988du);          /* 988D */
                OpCmpValue(cpu, 0x0030u);
                if (cpu->carry)
                    goto step;
                OpLda(memory, cpu, OpAbsY(cpu, 0x3f0eu));
                OpAndValue(cpu, 0x03ffu);
                if (!cpu->zero)
                    goto step;
                OpLda(memory, cpu, OpAbsY(cpu, 0x000au));      /* 989E */
                OpAndValue(cpu, 0x03ffu);
                OpSta(memory, cpu, OpDp(cpu, 0x54u));
                OpLdx(cpu, 0x0000u);
                for (;;) {
                    OpLda(memory, cpu, OpDp(cpu, 0x54u));      /* 98A9 */
                    OpCmp(memory, cpu, OpAbsX(cpu, 0xc100u));
                    if (cpu->zero)
                        break;
                    OpTxa(cpu);
                    cpu->carry = 0;
                    OpAdcValue(cpu, 0x0008u);
                    OpTax(cpu);
                    OpCpx(cpu, 0x0028u);
                    if (cpu->carry)
                        goto step;
                }
                OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);   /* 98BD */
                OpSepWidths(cpu, 0x20u);
                OpLda(memory, cpu, OpAbsX(cpu, 0xc001u));
                OpSta(memory, cpu, OpDp(cpu, 0x5au));
                OpStz(memory, cpu, OpDp(cpu, 0x5bu));
                OpLda(memory, cpu, OpAbsX(cpu, 0xc000u));
                Lufia2CaveRandomBelow(memory, cpu, 0x98cbu);
                OpIncA(cpu);
                ExchangeAccumulatorBytes(cpu);
                LoadA8(cpu, 0x00u);
                OpRepWidths(cpu, 0x20u);
                cpu->carry = 0;
                OpAdc(memory, cpu, OpDp(cpu, 0x54u));
                OpTax(cpu);
                OpLda(memory, cpu, OpAbsY(cpu, 0x000au));      /* 98D8 */
                OpAndValue(cpu, 0xfc00u);
                OpOra(memory, cpu, OpAbsX(cpu, 0xc000u));
                OpSta(memory, cpu, OpAbsY(cpu, 0x000au));
                OpLda(memory, cpu, OpAbsY(cpu, 0x3f0eu));
                OpAndValue(cpu, 0xfc00u);
                OpOra(memory, cpu, OpAbsX(cpu, 0xc004u));
                OpSta(memory, cpu, OpAbsY(cpu, 0x3f0eu));
                OpLda(memory, cpu, OpDp(cpu, 0x5au));
                if (!cpu->zero) {
                    OpLda(memory, cpu, OpAbsY(cpu, 0x00cau));  /* 98F4 */
                    OpAndValue(cpu, 0xfc00u);
                    OpOra(memory, cpu, OpAbsX(cpu, 0xc002u));
                    OpSta(memory, cpu, OpAbsY(cpu, 0x00cau));
                    OpLda(memory, cpu, OpAbsY(cpu, 0x3fceu));
                    OpAndValue(cpu, 0xfc00u);
                    OpOra(memory, cpu, OpAbsX(cpu, 0xc006u));
                    OpSta(memory, cpu, OpAbsY(cpu, 0x3fceu));
                }
step:
                OpStepMem(memory, cpu, OpDp(cpu, 0x56u), -1);  /* 990C */
                if (cpu->zero)
                    break;
                OpIny(cpu);
                OpIny(cpu);
            }
            OpStepMem(memory, cpu, OpDp(cpu, 0x58u), -1);      /* 9915 */
            if (cpu->zero)
                break;
            OpTya(cpu);
            cpu->carry = 0;
            OpAdcValue(cpu, 0x00b6u);
            OpTay(cpu);
            if (cpu->zero)
                break;
        }
next:
        OpSepWidths(cpu, 0x20u);                                     /* 9924 */
        OpPullX(memory, cpu);
        OpInx(cpu);
        OpCpx(cpu, 0x00f0u);
    } while (!cpu->carry);
}

/* Upper tile at cell A/B, then $83:9D46. */
static void CaveMarkTile(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site,
    uint16_t upper) {
    Lufia2CaveTileAt(memory, cpu, site);
    OpLda(memory, cpu, OpAbs(cpu, upper));
    Lufia2CaveSetUpperTile(memory, cpu, (uint16_t)(site + 6u));
    OpSepWidths(cpu, 0x20u);
}

/* $83:9930-$83:99C7: start, stair and chest tiles, then the sections. */
static void CaveFinish(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, CAVE_START_COLUMN));                   /* 9930 */
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_START_ROW));
    Lufia2CaveTileAt(memory, cpu, 0x9937u);
    TransferDirectToA(cpu);                                    /* 993A */
    Lufia2CaveSetUpperTile(memory, cpu, 0x993bu);
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, 0xe6adu));                   /* 9940 */
    OpCmpValue(cpu, 0xffu);
    if (!cpu->zero) {
        OpLda(memory, cpu, OpAbs(cpu, 0xe6adu));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbs(cpu, 0xe6aeu));
        CaveMarkTile(memory, cpu, 0x994eu, 0x4390u);
    }
    OpLda(memory, cpu, OpAbs(cpu, CAVE_STAIR_COLUMN));                   /* 9959 */
    ExchangeAccumulatorBytes(cpu);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_STAIR_ROW));
    CaveMarkTile(memory, cpu, 0x9960u, 0x4458u);
    OpLdx(cpu, 0x0000u);                                       /* 996B */
    for (;;) {
        OpTxa(cpu);                                            /* 996E */
        OpCmp(memory, cpu, OpAbs(cpu, CAVE_CHEST_COUNT));
        if (cpu->carry)
            break;
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_CHEST_COLUMNS));
        ExchangeAccumulatorBytes(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_CHEST_ROWS));
        Lufia2CaveTileAt(memory, cpu, 0x997bu);
        OpPushX(memory, cpu);                                  /* 997E */
        OpTxa(cpu);
        OpAslA(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, 0x7fe746u));
        OpBitValue(cpu, 0x4000u);
        OpLda(memory, cpu, OpAbs(cpu, cpu->zero ? 0x474eu : 0x4754u));
        Lufia2CaveSetUpperTile(memory, cpu, 0x9993u);
        OpSepWidths(cpu, 0x20u);
        OpPullX(memory, cpu);
        OpInx(cpu);
    }
    OpStz(memory, cpu, OpAbs(cpu, 0xd038u));                   /* 999C */
    OpStz(memory, cpu, OpAbs(cpu, 0xd039u));
    OpLdx(cpu, 0x0000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x5du), cpu->x);
    CaveCallLong(memory, cpu, 0x99a7u, Lufia2FieldReadSections);
    OpLdx(cpu, 0x7e0eu);                                       /* 99AB */
    OpWriteX(memory, cpu, OpDp(cpu, 0x2du), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpDp(cpu, 0x28u));
    OpLda(memory, cpu, 0x7fe69bu);
    CaveCallLong(memory, cpu, 0x99b8u, Lufia2FieldDecompressMapData);
    CaveCallLong(memory, cpu, 0x99bcu, Lufia2FieldSectionSize);
    OpSepWidths(cpu, 0x20u);
    CaveCallLong(memory, cpu, 0x99c2u, Lufia2FieldPackSectionAttributes);
    PullDataBank(memory, cpu);                                 /* 99C6 */
}

Lufia2ExecutionResult Lufia2CaveBuildFloor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    CaveClearGrid(memory, cpu);
    CaveCollectItems(memory, cpu);
    CaveFirstChests(memory, cpu);
    CaveChestContents(memory, cpu);
    CavePlaceRooms(memory, cpu);
    CaveLinkFloor(memory, cpu);
    CaveDedupeLinks(memory, cpu);
    CavePlaceStart(memory, cpu);
    CaveTreasureRoom(memory, cpu);
    CaveObjectsAndChests(memory, cpu);
    CaveShapeCells(memory, cpu);
    CaveTileMapBase(memory, cpu);
    CaveDrawCells(memory, cpu);
    if (!CaveReadTileSets(memory, cpu))
        return ExecutionHandoff(cpu, BFAA_HANDOFF);
    CaveApplyTileSets(memory, cpu);
    if (!CaveReadDecorations(memory, cpu))
        return ExecutionHandoff(cpu, BFAA_HANDOFF);
    CaveDecorate(memory, cpu);
    CaveFinish(memory, cpu);
    return ExecutionReturned(0x8399c7u);                       /* 99C7 RTS */
}

static Lufia2ExecutionResult CaveChildUnwound(uint32_t site) {
    Lufia2ExecutionResult result = ExecutionReturned(site);

    result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
    return result;
}

/* $83:9E31. */
Lufia2ExecutionResult Lufia2AncientCaveGenerateFloor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2PushedChildCall child,
    void *child_context) {
    Lufia2ExecutionResult result;

    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9E31 */
    OpCmp(memory, cpu, 0x000b75u);
    if (cpu->carry)
        OpSta(memory, cpu, 0x000b75u);
    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9E3F */
    OpDecA(cpu);
    OpSta(memory, cpu, 0x004204u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, 0x004205u);
    LoadA8(cpu, 0x0au);
    OpSta(memory, cpu, 0x004206u);
    LoadA8(cpu, 0xffu);                                        /* 9E53 */
    OpSta(memory, cpu, 0x7fe6f1u);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, CAVE_OBJECT_COUNT_LONG);
    OpSta(memory, cpu, 0x7fe732u);
    OpSta(memory, cpu, CAVE_CHEST_COUNT_LONG);
    OpSta(memory, cpu, 0x7fe735u);
    OpSta(memory, cpu, 0x7fe733u);
    OpLda(memory, cpu, OpDp(cpu, 0x40u));                      /* 9E6E */
    OpAndValue(cpu, 0x1fu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    do {
        CaveRandomByte(memory, cpu, 0x9e75u);
        OpStepMem(memory, cpu, OpDp(cpu, 0x54u), -1);
    } while (!cpu->zero);
    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9E7D */
    OpCmpValue(cpu, 0x63u);
    if (cpu->zero) {
        LoadA8(cpu, 0x0bu);                                    /* 9E85 */
        OpSta(memory, cpu, 0x0005b0u);
        LoadA8(cpu, 0x2eu);
        OpSta(memory, cpu, 0x0005b1u);
        LoadA8(cpu, 0x40u);
        OpSta(memory, cpu, 0x0005b2u);
        LoadA8(cpu, 0xf1u);
        OpSta(memory, cpu, OpAbs(cpu, 0x05acu));
        LoadA8(cpu, 0x01u);
        OpTestBits(memory, cpu, OpAbs(cpu, 0x05b6u), 0);
        return ExecutionReturned(0x839ea1u);                   /* 9EA1 RTL */
    }
    TransferDirectToA(cpu);                                    /* 9EA2 */
    OpSta(memory, cpu, 0x7fe698u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, 0x004214u);                             /* (floor-1)/10 */
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f4fu));
    OpSta(memory, cpu, 0x7fe697u);
    OpAslA(cpu);
    OpTax(cpu);
    OpRepWidths(cpu, 0x20u);                                         /* 9EB7 */
    OpLda(memory, cpu, OpLongX(cpu, 0x839f65u));
    OpSta(memory, cpu, OpDp(cpu, 0x56u));
    OpLda(memory, cpu, OpLongX(cpu, 0x839f49u));
    OpSta(memory, cpu, 0x7fe69bu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f5fu));
    OpSta(memory, cpu, 0x7fe69du);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f59u));
    OpSta(memory, cpu, 0x7fe699u);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f6bu));
    OpSta(memory, cpu, 0x7fe6a2u);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f7fu));
    OpSta(memory, cpu, OpDp(cpu, 0x54u));
    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9EE5 */
    OpAndValue(cpu, 0x0006u);
    OpSta(memory, cpu, OpDp(cpu, 0x58u));
    OpLsrA(cpu);
    OpAdc(memory, cpu, OpDp(cpu, 0x58u));
    OpAdc(memory, cpu, OpDp(cpu, 0x54u));
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f7fu));
    OpSta(memory, cpu, 0x7fe69fu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f80u));
    OpSta(memory, cpu, 0x7fe6a0u);
    OpLda(memory, cpu, 0x7fe697u);                             /* 9F04 */
    OpAslA(cpu);
    OpAdc(memory, cpu, 0x7fe697u);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x839f73u));
    OpSta(memory, cpu, 0x7fe6a5u);
    OpSepWidths(cpu, 0x20u);                                         /* 9F16 */
    OpLda(memory, cpu, OpLongX(cpu, 0x839f75u));
    OpSta(memory, cpu, 0x7fe6a7u);
    OpLda(memory, cpu, OpDp(cpu, 0x56u));                      /* music */
    OpCmp(memory, cpu, OpAbs(cpu, 0x099du));
    if (!cpu->zero) {
        OpSta(memory, cpu, OpAbs(cpu, 0x099du));               /* 9F27 */
        SimulateJslFrame(memory, cpu, 0x83u, 0x9f2du);
        if (!child(child_context, cpu, 0x8093feu, 0x839f2au, 3u))
            return CaveChildUnwound(0x839f2au);
    }
    SimulateJslFrame(memory, cpu, 0x83u, 0x9f31u);             /* 9F2E */
    if (!child(child_context, cpu, 0x83b5d3u, 0x839f2eu, 3u))
        return CaveChildUnwound(0x839f2eu);
    SimulateJsrFrame(memory, cpu, 0x9f34u);                    /* 9F32 */
    result = Lufia2CaveBuildFloor(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    SimulateRtsFrame(memory, cpu);
    OpLda(memory, cpu, CAVE_FLOOR_LONG);                             /* 9F35 */
    OpIncA(cpu);
    OpSta(memory, cpu, CAVE_FLOOR_LONG);
    return ExecutionReturned(0x839f3eu);                       /* 9F3E RTL */
}
