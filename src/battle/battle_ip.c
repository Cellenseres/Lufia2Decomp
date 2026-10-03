/* Battle IP table ($81:C129) and list rows ($81:DFA2). */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"
#include "lufia2/item.h"
#include "system/wram.h"

enum {
    TABLE = 0xdf00u,                    /* $7E, $30 bytes per slot */
    EQUIPMENT = 0x1357u,                /* 6 item words */
    LEVEL = 0xcau,
};

/* $81:F45E: A = IP skill record of skill A ($84:8F10), 0 for none. */
static void SkillRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    if (cpu->negative) {
        TransferDirectToA(cpu);
        return;
    }
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x848f10u, cpu->x)));
    cpu->carry = 0;
}

/* Stores A at TABLE + offset + Y. */
static void StoreTable16(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t offset) {
    StoreAAbsolute16(memory, cpu, (uint16_t)(TABLE + offset), cpu->y);
}

/* Stores the low byte of A at TABLE + offset + Y. */
static void StoreTable8(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint16_t offset) {
    StoreAAbsolute8(memory, cpu, (uint16_t)(TABLE + offset), cpu->y);
}

/* $81:C129: IP skills of member $1BE8's equipment. */
Lufia2ExecutionResult Lufia2BattleIpSkills(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;
    int usable;

    LoadX16(cpu, TABLE);
    StoreWordAbsolute(memory, cpu, SNES_WMADDL, cpu->x); /* WRAM port */
    StoreZeroAbsolute8(memory, cpu, SNES_WMADDH, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, WRAM_BATTLE_PARTY_SLOT));
    AslA16(cpu);
    TransferAToX(cpu);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_BATTLE_PARTY_RECORDS, cpu->x));
    LoadA16(cpu, (uint16_t)(Read16AbsoluteIndexed(memory, cpu, 0x00bcu, cpu->x) & 0x00ffu));
    StoreADirect16(memory, cpu, LEVEL);
    for (i = 0; i < 6u; ++i) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, (uint16_t)(0x0066u + 2u * i), cpu->x));
        StoreAAbsolute16(memory, cpu, (uint16_t)(EQUIPMENT + 2u * i), 0);
    }
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, 0x0000u);
    LoadY16(cpu, cpu->x);
    do {
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859e8fu, cpu->x)));   /* C177 */
        StoreTable16(memory, cpu, 0x06u);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, EQUIPMENT, cpu->x));
        StoreAAbsolute16(memory, cpu, WRAM_ITEM_RECORD_ID, 0);
        PushIndex(memory, cpu);
        PushY(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x81u, 0xc189u);
        (void)Lufia2LoadItemRecord(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        cpu->y = PullIndexValue(memory, cpu);
        for (i = 0; i < 6u; ++i) {                             /* name */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
                (uint16_t)(WRAM_RECORD_BUFFER + 2u * i), 0));
            StoreTable16(memory, cpu, (uint16_t)(0x07u + 2u * i));
        }
        TransferDirectToA(cpu);
        StoreTable16(memory, cpu, 0x13u);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0babu, 0));
        StoreTable16(memory, cpu, 0x01u);
        if (!cpu->zero) {
            PushIndex(memory, cpu);
            SimulateJslFrame(memory, cpu, 0x81u, 0xc1bfu);
            SkillRecord(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x840003u, cpu->x)));
            StoreTable16(memory, cpu, 0x03u);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x840005u, cpu->x)));
            StoreTable8(memory, cpu, 0x05u);
            PushY(memory, cpu);
            LoadA8(cpu, 0x0du);
            StoreADirect8(memory, cpu, 0x54u);
            {
                int ended = 0;

                do {
                    if (!ended) {
                        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x840006u, cpu->x)));
                        ended = cpu->zero;
                    }
                    StoreTable8(memory, cpu, 0x14u);
                    IncrementX16(cpu);
                    IncrementY16(cpu);
                    DecrementDirect8(memory, cpu, 0x54u);
                } while (!cpu->zero);
            }
        } else {
            SetAccumulatorWidth(cpu, 1);                       /* C1F2 */
            PushIndex(memory, cpu);
            PushY(memory, cpu);
            TransferDirectToA(cpu);
            StoreTable8(memory, cpu, 0x14u);
            StoreTable8(memory, cpu, 0x15u);
            LoadA8(cpu, 0x0bu);
            StoreADirect8(memory, cpu, 0x54u);
            do {
                TransferDirectToA(cpu);
                StoreTable8(memory, cpu, 0x16u);
                IncrementX16(cpu);
                IncrementY16(cpu);
                DecrementDirect8(memory, cpu, 0x54u);
            } while (!cpu->zero);
        }
        cpu->y = PullIndexValue(memory, cpu);                  /* C20B */
        cpu->x = PullIndexValue(memory, cpu);
        LoadAAbsolute8(memory, cpu, (uint16_t)(TABLE + 1u), cpu->y);
        Or8(cpu, AbsoluteByte(memory, cpu, (uint16_t)(TABLE + 2u), cpu->y));
        usable = 0;
        if (!cpu->zero) {
            LoadA8(cpu, DirectByte(memory, cpu, LEVEL));
            Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, (uint16_t)(TABLE + 5u), cpu->y));
            usable = cpu->carry;
        }
        if (usable) {
            TransferDirectToA(cpu);
            StoreTable8(memory, cpu, 0x00u);                   /* usable */
        } else {
            LoadA8(cpu, 0x01u);
            StoreTable8(memory, cpu, 0x00u);
        }
        cpu->x = PullIndexValue(memory, cpu);                  /* C227 */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, cpu->y);
        cpu->carry = 0;
        Add16Value(cpu, 0x0030u);
        TransferAToY(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x000cu);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x0bu);
    return ExecutionReturned(0x81c23fu);
}

/* $81:E835: character A to battle tiles: A top, B bottom. */
Lufia2ExecutionResult Lufia2BattleGlyph(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const uint8_t c = A8(cpu);

    Compare8(cpu, c, 0xcdu);
    if (!cpu->carry) {
        Compare8(cpu, c, 0x10u);
        if (!cpu->carry) {
            Compare8(cpu, c, 0x00u);
            if (!cpu->zero) {
                cpu->carry = 0;
                Adc8(cpu, 0xdfu);
            }
        }
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x00u);
        return ExecutionReturned(0x81e847u);
    }
    PushIndex(memory, cpu);                                    /* E848 */
    cpu->carry = 1;
    Sbc8(cpu, 0xcdu);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    PushAccumulator8(memory, cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x97b2c9u, cpu->x)));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    cpu->x = PullIndexValue(memory, cpu);
    {
        const uint8_t n = A8(cpu);

        Compare8(cpu, n, 0x14u);
        if (cpu->carry) {
            Compare8(cpu, n, 0x19u);
            if (!cpu->carry) {
                LoadA8(cpu, 0xceu);
                return ExecutionReturned(0x81e86eu);
            }
            Compare8(cpu, n, 0x2du);
            if (cpu->carry) {
                Compare8(cpu, n, 0x32u);
                if (!cpu->zero) {
                    LoadA8(cpu, 0xceu);
                    return ExecutionReturned(0x81e86eu);
                }
            }
        }
        LoadA8(cpu, 0xcdu);
    }
    return ExecutionReturned(0x81e871u);
}

/* Writes a two-row tile at Y: the low byte of A then the high byte one
 * tilemap row ($40) below, each followed by attribute. Y advances by 2. */
static void TileRow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t attribute) {
    StoreAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, 0x0040u, cpu->y);
    LoadA8(cpu, attribute);
    StoreAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    StoreAAbsolute8(memory, cpu, 0x0041u, cpu->y);
    IncrementY16(cpu);
    IncrementY16(cpu);
}

/* Y += bytes (16-bit), leaving the accumulator 8-bit. */
static void AddY(Lufia2CpuState *cpu, uint16_t bytes) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, bytes);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $54 characters from $DF00,X. */
static void EntryChars(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    uint8_t count, uint16_t glyph_return) {
    LoadA8(cpu, count);
    StoreADirect8(memory, cpu, 0x54u);
    do {
        LoadAAbsolute8(memory, cpu, TABLE, cpu->x);
        IncrementX16(cpu);
        SimulateJsrFrame(memory, cpu, glyph_return);
        Lufia2BattleGlyph(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        TileRow(memory, cpu, DirectByte(memory, cpu, 0x55u));
        DecrementDirect8(memory, cpu, 0x54u);
    } while (!cpu->zero);
}

/* Palette from the entry's first byte, then its icon tile. */
static int EntryHead(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    unsigned skip, int glyph, uint16_t glyph_return) {
    LoadAAbsolute8(memory, cpu, TABLE, cpu->x);
    if (cpu->negative)
        return 0;
    AslA8(cpu);
    AslA8(cpu);
    Or8(cpu, 0x20u);
    StoreADirect8(memory, cpu, 0x55u);
    cpu->x = (uint16_t)(cpu->x + skip);
    LoadAAbsolute8(memory, cpu, TABLE, cpu->x);
    IncrementX16(cpu);
    if (glyph) {
        SimulateJsrFrame(memory, cpu, glyph_return);
        Lufia2BattleGlyph(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        ExchangeAccumulatorBytes(cpu);
        StoreAAbsolute8(memory, cpu, 0x0040u, cpu->y);
        LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    } else {
        StoreAAbsolute8(memory, cpu, 0x0040u, cpu->y);
        TransferDirectToA(cpu);
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    }
    Or8(cpu, 0x20u);
    StoreAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    StoreAAbsolute8(memory, cpu, 0x0041u, cpu->y);
    IncrementY16(cpu);
    IncrementY16(cpu);
    return 1;
}

/* Pushes the data bank, then sets it to $7E. */
static void Bank7E(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* $81:E019/E094/E100: IP, spell or item entry X. */
static void BattleEntry(const Lufia2Memory *memory, Lufia2CpuState *cpu,
    unsigned kind) {
    static const uint16_t kSkip[3] = {0x0018u, 0x0036u, 0x0034u};

    Bank7E(memory, cpu);
    TransferXToA(cpu);
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x17u));
    if (!cpu->carry) {
        SetAccumulatorWidth(cpu, 0);
        TransferXToA(cpu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        if (kind == 1u) {                                      /* * 24 */
            PushAccumulator16(memory, cpu);
            AslA16(cpu);
            Add16Value(cpu, (uint16_t)(Read8(memory, (uint16_t)(cpu->stack + 1u)) |
                (Read8(memory, (uint16_t)(cpu->stack + 2u)) << 8)));
            TransferAToX(cpu);
            PullAccumulator16(memory, cpu);
        } else {
            AslA16(cpu);
            TransferAToX(cpu);
        }
        SetAccumulatorWidth(cpu, 1);
        if (kind == 0u) {
            if (EntryHead(memory, cpu, 3u, DirectByte(memory, cpu, 0x1bu) != 0, 0xe045u)) {
                EntryChars(memory, cpu, 0x0bu, 0xe070u);
                PullDataBank(memory, cpu);
                return;
            }
        } else if (kind == 1u) {
            if (EntryHead(memory, cpu, 6u, 0, 0)) {
                EntryChars(memory, cpu, 0x1au, 0xe0dcu);
                PullDataBank(memory, cpu);
                return;
            }
        } else if (EntryHead(memory, cpu, 3u, 0, 0)) {
            EntryChars(memory, cpu, 0x0fu, 0xe141u);
            AddY(cpu, 0x0014u);
            PullDataBank(memory, cpu);
            return;
        }
    }
    AddY(cpu, kSkip[kind]);
    PullDataBank(memory, cpu);
}

/* $81:DFB2: clear the row through WMDATA, then its entries by mode $1B. */
static void BattleListRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    unsigned i;
    uint8_t mode;

    StoreWordAbsolute(memory, cpu, SNES_WMADDL, cpu->y);
    StoreZeroAbsolute8(memory, cpu, SNES_WMADDH, 0);
    LoadA8(cpu, 0x80u);
    for (i = 0; i < 0x80u; ++i)
        StoreZeroAbsolute8(memory, cpu, SNES_WMDATA, 0);
    LoadA8(cpu, 0x00u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x1bu));
    mode = A8(cpu);
    for (i = 0; i < 6u; ++i)
        IncrementY16(cpu);
    if (mode != 0u && mode != 2u) {
        Compare8(cpu, mode, 0x02u);
        PushIndex(memory, cpu);
        SimulateJsrFrame(memory, cpu, 0xdfd1u);
        BattleEntry(memory, cpu, 0u);
        SimulateRtsFrame(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        for (i = 0; i < 4u; ++i)
            IncrementY16(cpu);
        PushIndex(memory, cpu);
        SimulateJsrFrame(memory, cpu, 0xdfdbu);
        BattleEntry(memory, cpu, 0u);
        SimulateRtsFrame(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
        IncrementX16(cpu);
        AddY(cpu, 0x0046u);
        return;
    }
    if (mode == 2u)
        Compare8(cpu, mode, 0x02u);
    PushIndex(memory, cpu);
    SimulateJsrFrame(memory, cpu, mode == 2u ? 0xdff2u : 0xe00au);
    BattleEntry(memory, cpu, mode == 2u ? 1u : 2u);
    SimulateRtsFrame(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
    AddY(cpu, mode == 2u ? 0x0044u : 0x0046u);
}

/* $81:DFA2: eight battle list rows from entry X - 2 at $7E:3080. */
Lufia2ExecutionResult Lufia2BattleListRows(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x08u);
    LoadY16(cpu, 0x3080u);
    cpu->x = (uint16_t)(cpu->x - 2u);
    SetNz16(cpu, cpu->x);
    do {
        PushAccumulator8(memory, cpu);
        SimulateJsrFrame(memory, cpu, 0xdfacu);
        BattleListRow(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x81dfb1u);
}
