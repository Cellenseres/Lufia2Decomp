/* World map NMI, streaming and regions. */

#include "core/cpu_internal.h"
#include "lufia2/world_map.h"
#include "system/wram.h"

/* $86:D1E8: tile column upload, rows of $0100 words. */
static void WorldMapColumnUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xd1d8u);
    SetAccumulatorWidth(cpu, 0);                               /* D1E8 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->y));
    Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    StoreAAbsolute8(memory, cpu, SNES_A1B(6), 0);
    LoadA8(cpu, 0x40u);
    StoreADirect8(memory, cpu, 0x05u);
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);
    StoreADirect8(memory, cpu, 0x33u);
    LoadAAbsolute8(memory, cpu, 0xd26bu, cpu->x);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x33u));
    StoreADirect8(memory, cpu, 0x35u);
    if (!cpu->zero) {
        LoadA8(cpu, 0xc0u);
        StoreADirect8(memory, cpu, 0x05u);
    }
    LoadAAbsolute8(memory, cpu, 0x0002u, cpu->y);              /* D214 */
    StoreADirect8(memory, cpu, 0x37u);
    cpu->carry = 0;
    do {
        SetAccumulatorWidth(cpu, 0);                           /* D21A */
        LoadADirect16(memory, cpu, 0x33u);
        Write16Absolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
        LoadADirect16(memory, cpu, 0x35u);
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->accumulator);
        LoadADirect16(memory, cpu, 0x39u);
        Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
        cpu->carry = 0;
        Add16Value(cpu, 0x0100u);
        StoreADirect16(memory, cpu, 0x39u);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0x05u));
        StoreAAbsolute8(memory, cpu, SNES_MDMAEN, 0);
        DecrementDirect8(memory, cpu, 0x37u);
    } while (!cpu->zero);
    LoadAAbsolute8(memory, cpu, 0xd26cu, cpu->x);              /* D23C */
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 0x0002u, cpu->y));
    if (!cpu->zero) {
        StoreADirect8(memory, cpu, 0x37u);
        LoadAAbsolute8(memory, cpu, 0xd26bu, cpu->x);
        StoreADirect8(memory, cpu, 0x35u);
        cpu->carry = 0;
        do {
            SetAccumulatorWidth(cpu, 0);                       /* D24D */
            LoadADirect16(memory, cpu, 0x35u);
            Write16Absolute(memory, cpu, SNES_DASL(7), cpu->accumulator);
            LoadADirect16(memory, cpu, 0x39u);
            Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, 0x0100u);
            StoreADirect16(memory, cpu, 0x39u);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
            DecrementDirect8(memory, cpu, 0x37u);
        } while (!cpu->zero);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $86:D271: block upload, two passes of rows $0200 apart. */
static void WorldMapBlockUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned pass;

    SimulateJsrFrame(memory, cpu, 0xd1ddu);
    SetAccumulatorWidth(cpu, 0);                               /* D271 */
    And16(cpu, 0x007fu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->y));
    Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    StoreAAbsolute8(memory, cpu, SNES_A1B(6), 0);
    LoadAAbsolute8(memory, cpu, 0xd2d0u, cpu->x);
    StoreADirect8(memory, cpu, 0x33u);
    LoadAAbsolute8(memory, cpu, 0xd2d1u, cpu->x);
    StoreADirect8(memory, cpu, 0x37u);
    StoreADirect8(memory, cpu, 0x38u);
    for (pass = 0; pass < 2u; ++pass) {
        if (pass)
            IncrementDirect8(memory, cpu, 0x3au);              /* D2B0 */
        LoadXDirect16(memory, cpu, 0x39u);
        cpu->carry = 0;
        do {
            SetAccumulatorWidth(cpu, 0);                       /* D295 */
            LoadADirect16(memory, cpu, 0x33u);
            Write16Absolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
            TransferXToA(cpu);
            Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, 0x0200u);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0x40u, SNES_MDMAEN);
            DecrementDirect8(memory, cpu, pass ? 0x38u : 0x37u);
        } while (!cpu->zero);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $86:D1A1: pending map tile uploads, $1365 entries. */
static void WorldMapTileUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0xcf14u);
    LoadA8(cpu, 0x18u);                                        /* D1A1 */
    StoreAAbsolute8(memory, cpu, SNES_BBAD(6), 0);
    StoreAAbsolute8(memory, cpu, SNES_BBAD(7), 0);
    StoreAImmediate8(memory, cpu, 0x01u, SNES_DMAP(6));
    StoreAImmediate8(memory, cpu, 0x09u, SNES_DMAP(7));
    LoadX16(cpu, 0xd1e7u);
    Write16Absolute(memory, cpu, SNES_A1TL(7), cpu->x);
    StoreAImmediate8(memory, cpu, 0x86u, SNES_A1B(7));
    Write8(memory, DirectAddress(cpu, 0x34u), 0x00u);
    Write8(memory, DirectAddress(cpu, 0x36u), 0x00u);
    LoadX16(cpu, 0x1367u);
    do {
        PushIndex(memory, cpu);                                /* D1C5 */
        LoadY16(cpu, Read16DirectIndexed(memory, cpu, 0x40u, cpu->x));
        if (!cpu->zero) {
            StoreYDirect16(memory, cpu, 0x39u);
            LoadY16(cpu, Read16DirectIndexed(memory, cpu, 0x00u, cpu->x));
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
            if (!cpu->negative)
                WorldMapColumnUpload(memory, cpu);
            else
                WorldMapBlockUpload(memory, cpu);
        }
        cpu->x = PullIndexValue(memory, cpu);                  /* D1DE */
        IncrementX16(cpu);
        IncrementX16(cpu);
        {
            const uint32_t count = AbsoluteIndexedAddress(cpu, 0x1365u, 0);
            const uint8_t left = (uint8_t)(Read8(memory, count) - 1u);

            Write8(memory, count, left);
            SetNz8(cpu, left);
        }
    } while (!cpu->zero);
    SimulateRtsFrame(memory, cpu);
}

/* $86:CFC0: $16E7 palette cycles, five bytes each at $16E8. */
static void WorldMapPaletteCycles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, 0x16e8u);
    do {
        PushAccumulator8(memory, cpu);                         /* CFC8 */
        {
            const uint32_t tick = DirectIndexedAddress(cpu, 0x04u, cpu->x);
            const uint8_t value = (uint8_t)(Read8(memory, tick) + 1u);

            Write8(memory, tick, value);
            SetNz8(cpu, value);
            LoadA8(cpu, value);
        }
        Compare8(cpu, A8(cpu),
            Read8(memory, DirectIndexedAddress(cpu, 0x02u, cpu->x)));
        if (cpu->carry) {
            uint32_t cycle;

            Write8(memory, DirectIndexedAddress(cpu, 0x04u, cpu->x), 0x00u);
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x00u, cpu->x)));
            StoreAAbsolute8(memory, cpu, SNES_CGADD, 0);
            cycle = DirectIndexedAddress(cpu, 0x03u, cpu->x);
            LoadA8(cpu, (uint8_t)(Read8(memory, cycle) + 1u));
            Compare8(cpu, A8(cpu),
                Read8(memory, DirectIndexedAddress(cpu, 0x01u, cpu->x)));
            if (cpu->carry)
                LoadA8(cpu, 0x00u);
            Write8(memory, cycle, A8(cpu));                    /* CFE1 */
            StoreADirect8(memory, cpu, 0x38u);
            AslA8(cpu);
            StoreADirect8(memory, cpu, 0x33u);
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x01u, cpu->x)));
            cpu->carry = 1;
            Sbc8(cpu, Read8(memory, cycle));
            StoreADirect8(memory, cpu, 0x37u);
            LoadA8(cpu, 0x00u);
            ExchangeAccumulatorBytes(cpu);
            LoadA8(cpu, Read8(memory, DirectIndexedAddress(cpu, 0x00u, cpu->x)));
            AslA8(cpu);
            Adc8(cpu, DirectByte(memory, cpu, 0x33u));
            TransferAToY(cpu);
            do {
                LoadAAbsolute8(memory, cpu, WRAM_CGRAM_BUFFER, cpu->y); /* CFF8 */
                StoreAAbsolute8(memory, cpu, SNES_CGDATA, 0);
                IncrementY16(cpu);
                LoadAAbsolute8(memory, cpu, WRAM_CGRAM_BUFFER, cpu->y);
                StoreAAbsolute8(memory, cpu, SNES_CGDATA, 0);
                IncrementY16(cpu);
                DecrementDirect8(memory, cpu, 0x37u);
            } while (!cpu->zero);
            LoadA8(cpu, DirectByte(memory, cpu, 0x38u));
            if (!cpu->zero) {
                LoadA8(cpu, Read8(memory,
                    DirectIndexedAddress(cpu, 0x00u, cpu->x)));
                AslA8(cpu);
                TransferAToY(cpu);
                do {
                    LoadAAbsolute8(memory, cpu, WRAM_CGRAM_BUFFER, cpu->y); /* D012 */
                    StoreAAbsolute8(memory, cpu, SNES_CGDATA, 0);
                    IncrementY16(cpu);
                    LoadAAbsolute8(memory, cpu, WRAM_CGRAM_BUFFER, cpu->y);
                    StoreAAbsolute8(memory, cpu, SNES_CGDATA, 0);
                    IncrementY16(cpu);
                    DecrementDirect8(memory, cpu, 0x38u);
                } while (!cpu->zero);
            }
        }
        SetAccumulatorWidth(cpu, 0);                           /* D024 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0005u);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, Pull8(memory, cpu));
        DecrementA8(cpu);
    } while (!cpu->zero);
}

/* $86:CEF6: world map NMI via the $00:0067 vector. */
Lufia2ExecutionResult Lufia2WorldMapNmiUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t mode7_regs[8] = {
        SNES_M7A, SNES_M7A, SNES_M7B, SNES_M7B, SNES_M7C, SNES_M7C, SNES_M7D,
        SNES_M7D};
    static const uint16_t scroll_regs[6] = {
        SNES_BG1HOFS, SNES_BG1VOFS, SNES_BG2HOFS, SNES_BG2VOFS, SNES_BG3HOFS, SNES_BG3VOFS};
    Lufia2ExecutionResult result;
    unsigned i;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x86d1a0u;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* CEF6 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, 0x86u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StoreAImmediate8(memory, cpu, 0x8fu, SNES_INIDISP);
    StoreZeroAbsolute8(memory, cpu, SNES_HDMAEN, 0);
    LoadAAbsolute8(memory, cpu, 0x11d9u, 0);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x1365u, 0);
        if (!cpu->zero)
            WorldMapTileUploads(memory, cpu);
    }
    StoreZeroAbsolute8(memory, cpu, SNES_DMAP(7), 0);          /* CF15 */
    StoreAImmediate8(memory, cpu, 0x18u, SNES_BBAD(7));
    LoadAAbsolute8(memory, cpu, 0x1710u, 0);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, SNES_VMAIN, 0);
        StoreAImmediate8(memory, cpu, 0x7fu, SNES_A1B(7));
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1712u, 0));
        Write16Absolute(memory, cpu, SNES_A1TL(7), cpu->accumulator);
        And16(cpu, 0x3fffu);
        Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0x0100u);
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
        StoreZeroAbsolute8(memory, cpu, 0x1710u, 0);
    }
    LoadAAbsolute8(memory, cpu, 0x1711u, 0);                   /* CF48 */
    if (!cpu->zero) {
        StoreAImmediate8(memory, cpu, 0x03u, SNES_VMAIN);
        StoreAImmediate8(memory, cpu, 0x7fu, SNES_A1B(7));
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1714u, 0));
        And16(cpu, 0x3fffu);
        Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
        IncrementA16(cpu);
        PushAccumulator16(memory, cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0xdf00u);
        Write16Absolute(memory, cpu, SNES_A1TL(7), cpu->x);
        LoadX16(cpu, 0x0080u);
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
        cpu->y = PullIndexValue(memory, cpu);
        Write16Absolute(memory, cpu, SNES_VMADDL, cpu->y);
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
        StoreZeroAbsolute8(memory, cpu, 0x1711u, 0);
    }
    StoreAImmediate8(memory, cpu, 0x80u, SNES_VMAIN);          /* CF86 */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1702u, 0));
    if (!cpu->zero) {
        Write16Absolute(memory, cpu, SNES_DASL(7), cpu->x);
        CopyAbsolute8(memory, cpu, 0x1701u, SNES_CGADD);
        SetAccumulatorWidth(cpu, 0);
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1704u, 0));
        Write16Absolute(memory, cpu, SNES_A1TL(7), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        CopyAbsolute8(memory, cpu, 0x1706u, SNES_A1B(7));
        StoreZeroAbsolute8(memory, cpu, SNES_DMAP(7), 0);
        StoreAImmediate8(memory, cpu, 0x22u, SNES_BBAD(7));
        StoreAImmediate8(memory, cpu, 0x80u, SNES_MDMAEN);
        LoadX16(cpu, 0x0000u);
        Write16Absolute(memory, cpu, 0x1702u, cpu->x);
    }
    LoadAAbsolute8(memory, cpu, 0x16e7u, 0);                   /* CFC0 */
    if (!cpu->zero)
        WorldMapPaletteCycles(memory, cpu);
    Write8(memory, DirectAddress(cpu, 0x33u), 0x00u);          /* D032 */
    LoadAAbsolute8(memory, cpu, 0x11deu, 0);
    if (cpu->zero) {
        for (i = 0; i < 8u; ++i)
            CopyAbsolute8(memory, cpu, (uint16_t)(0x1707u + i), mode7_regs[i]);
    } else {
        LoadAAbsolute8(memory, cpu, 0x11dau, 0);               /* D06B */
        if (!cpu->negative) {
            LoadA8(cpu, 0x03u);
            StoreAAbsolute8(memory, cpu, SNES_DMAP(0), 0);
            StoreAAbsolute8(memory, cpu, SNES_DMAP(1), 0);
            StoreAImmediate8(memory, cpu, 0x1bu, SNES_BBAD(0));
            StoreAImmediate8(memory, cpu, 0x1du, SNES_BBAD(1));
            StoreAImmediate8(memory, cpu, 0x00u, SNES_A1B(0));
            StoreAImmediate8(memory, cpu, 0x00u, SNES_A1B(1));
            LoadX16(cpu, 0x1718u);
            Write16Absolute(memory, cpu, SNES_A1TL(0), cpu->x);
            LoadX16(cpu, 0x1a9bu);
            Write16Absolute(memory, cpu, SNES_A1TL(1), cpu->x);
            LoadA8(cpu, 0x03u);
            TestBitsDirect(memory, cpu, 0x33u, 1);
        }
        LoadAAbsolute8(memory, cpu, 0x11ddu, 0);               /* D09C */
        if (cpu->zero) {
            StoreZeroAbsolute8(memory, cpu, SNES_CGWSEL, 0);
            CopyAbsolute8(memory, cpu, 0x170fu, SNES_CGADSUB);
            StoreZeroAbsolute8(memory, cpu, SNES_DMAP(4), 0);
            StoreAImmediate8(memory, cpu, 0x32u, SNES_BBAD(4));
            StoreZeroAbsolute8(memory, cpu, SNES_A1B(4), 0);
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1716u, 0));
            Write16Absolute(memory, cpu, SNES_A1TL(4), cpu->x);
            LoadA8(cpu, 0x10u);
            TestBitsDirect(memory, cpu, 0x33u, 1);
        }
    }
    LoadAAbsolute8(memory, cpu, 0x11dfu, 0);                   /* D0BF */
    if (!cpu->zero) {
        LoadA8(cpu, 0x43u);
        StoreAAbsolute8(memory, cpu, SNES_DMAP(4), 0);
        StoreAAbsolute8(memory, cpu, SNES_DMAP(5), 0);
        LoadX16(cpu, 0xde00u);
        Write16Absolute(memory, cpu, SNES_A1TL(4), cpu->x);
        LoadX16(cpu, 0xe000u);
        Write16Absolute(memory, cpu, SNES_A1TL(5), cpu->x);
        LoadA8(cpu, 0x30u);
        TestBitsDirect(memory, cpu, 0x33u, 1);
    }
    LoadAAbsolute8(memory, cpu, 0x11e1u, 0);                   /* D0DC */
    if (!cpu->zero) {
        LoadA8(cpu, 0x41u);
        StoreAAbsolute8(memory, cpu, SNES_DMAP(2), 0);
        StoreAAbsolute8(memory, cpu, SNES_DMAP(3), 0);
        StoreAImmediate8(memory, cpu, 0x26u, SNES_BBAD(2));
        StoreAImmediate8(memory, cpu, 0x28u, SNES_BBAD(3));
        LoadA8(cpu, 0x7fu);
        StoreAAbsolute8(memory, cpu, SNES_A1B(2), 0);
        StoreAAbsolute8(memory, cpu, SNES_DASB(2), 0);
        StoreAAbsolute8(memory, cpu, SNES_A1B(3), 0);
        StoreAAbsolute8(memory, cpu, SNES_DASB(3), 0);
        LoadY16(cpu, 0xd400u);
        LoadAAbsolute8(memory, cpu, 0x11dbu, 0);
        LsrA8(cpu);
        if (cpu->carry)
            LoadY16(cpu, 0xd600u);
        Write16Absolute(memory, cpu, SNES_A1TL(2), cpu->y);
        LoadY16(cpu, 0xd800u);
        LoadAAbsolute8(memory, cpu, 0x11dcu, 0);
        LsrA8(cpu);
        if (cpu->carry)
            LoadY16(cpu, 0xda00u);
        Write16Absolute(memory, cpu, SNES_A1TL(3), cpu->y);
        LoadA8(cpu, 0x0cu);
        TestBitsDirect(memory, cpu, 0x33u, 1);
        LoadY16(cpu, 0x00ffu);
        Write16Absolute(memory, cpu, SNES_WH0, cpu->y);
        Write16Absolute(memory, cpu, SNES_WH2, cpu->y);
    }
    LoadAAbsolute8(memory, cpu, 0x11d8u, 0);                   /* D12C */
    if (!cpu->zero) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x33u));
        StoreAAbsolute8(memory, cpu, SNES_HDMAEN, 0);
    }
    for (i = 0; i < 4u; ++i)                                   /* D136 */
        CopyAbsolute8(memory, cpu, (uint16_t)(0x11f8u + i),
            (uint16_t)(SNES_M7X + (i >> 1)));
    LoadAAbsolute8(memory, cpu, 0x11d9u, 0);
    if (cpu->zero) {
        for (i = 0; i < 12u; ++i)
            CopyAbsolute8(memory, cpu, (uint16_t)(0x0594u + i),
                scroll_regs[i >> 1]);
    }
    StoreZeroAbsolute8(memory, cpu, 0x11d9u, 0);               /* D19B */
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return result;
}

/* $86:ADEE: $0B = map cell offset of ($58, $5A). */
static void WorldMapCellOffset(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadADirect16(memory, cpu, 0x5au);                         /* ADEE */
    And16(cpu, 0x003fu);
    ExchangeAccumulatorBytes(cpu);
    LsrA16(cpu);
    StoreADirect16(memory, cpu, 0x0bu);
    LoadADirect16(memory, cpu, 0x58u);
    And16(cpu, 0x003fu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x0bu));
    AslA16(cpu);
    Add16Value(cpu, 0x0000u);
    StoreADirect16(memory, cpu, 0x0bu);
    SimulateRtsFrame(memory, cpu);
}

/* $86:AE05: $08 = map block pointer for ($58, $5A). */
static void WorldMapBlockPointer(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadADirect16(memory, cpu, 0x58u);                         /* AE05 */
    And16(cpu, 0x00ffu);
    LsrA16(cpu);
    StoreADirect16(memory, cpu, 0x08u);
    LoadADirect16(memory, cpu, 0x5au);
    And16(cpu, 0x00ffu);
    LsrA16(cpu);
    ExchangeAccumulatorBytes(cpu);
    LsrA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x08u));
    AslA16(cpu);
    Add16Value(cpu, 0x4040u);
    StoreADirect16(memory, cpu, 0x08u);
    SimulateRtsFrame(memory, cpu);
}

/* Metatile index for the current cell: [$DF] or [$E3] table. */
static void WorldMapMetatile(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA16(cpu, Read16Long(memory, ((uint32_t)cpu->data_bank << 16) +
        Read16Direct(memory, cpu, 0x08u)));                    /* LDA ($08) */
    AslA16(cpu);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x00u));
    TransferAToY(cpu);
    LoadA16(cpu, Read16IndirectLongY(
        memory, cpu, cpu->negative ? 0xe3u : 0xdfu));
    StoreADirect16(memory, cpu, 0x00u);
    AslA16(cpu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x00u));
    TransferAToY(cpu);
    LoadXDirect16(memory, cpu, 0x0bu);
}

/* $86:ACFE: stream one map column into $7F:DF00/$7F:DF80. */
static void WorldMapStreamColumn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x99eeu);
    PushAndSetDataBank(memory, cpu, 0x7fu);                    /* ACFE */
    SetAccumulatorWidth(cpu, 0);
    WorldMapCellOffset(memory, cpu, 0xad07u);
    WorldMapBlockPointer(memory, cpu, 0xad0au);
    LoadADirect16(memory, cpu, 0x0bu);
    And16(cpu, 0xc07fu);
    Write16Long(memory, 0x001714u, cpu->accumulator);
    LoadADirect16(memory, cpu, 0x58u);
    And16(cpu, 0x0001u);
    AslA16(cpu);
    StoreADirect16(memory, cpu, 0x13u);
    LoadADirect16(memory, cpu, 0x5au);
    PushAccumulator16(memory, cpu);
    And16(cpu, 0x003fu);
    AslA16(cpu);
    StoreADirect16(memory, cpu, 0x0bu);
    LoadA16(cpu, 0x0040u);
    StoreADirect16(memory, cpu, 0x26u);
    do {
        LoadADirect16(memory, cpu, 0x5au);                     /* AD2A */
        And16(cpu, 0x0001u);
        AslA16(cpu);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, 0x13u));
        StoreADirect16(memory, cpu, 0x00u);
        WorldMapMetatile(memory, cpu);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xe7u));
        StoreAAbsolute16(memory, cpu, 0xdf00u, cpu->x);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xeau));
        StoreAAbsolute16(memory, cpu, 0xdf80u, cpu->x);
        LoadADirect16(memory, cpu, 0x0bu);
        IncrementA16(cpu);
        IncrementA16(cpu);
        And16(cpu, 0x007fu);
        StoreADirect16(memory, cpu, 0x0bu);
        SetAccumulatorWidth(cpu, 1);
        IncrementDirect8(memory, cpu, 0x5au);
        if (cpu->zero) {
            SetAccumulatorWidth(cpu, 0);                       /* AD70 */
            WorldMapBlockPointer(memory, cpu, 0xad74u);
        } else {
            LoadA8(cpu, DirectByte(memory, cpu, 0x5au));
            LsrA8(cpu);
            if (!cpu->carry)
                IncrementDirect8(memory, cpu, 0x09u);
        }
        SetAccumulatorWidth(cpu, 0);                           /* AD75 */
        {
            const uint16_t left =
                (uint16_t)(Read16Direct(memory, cpu, 0x26u) - 1u);

            Write16Direct(memory, cpu, 0x26u, left);
            SetNz16(cpu, left);
        }
    } while (!cpu->zero);
    PullAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, 0x58u);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $86:AC6C: stream one map row into the $7F buffer at $0B. */
static void WorldMapStreamRow(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9a1fu);
    PushAndSetDataBank(memory, cpu, 0x7fu);                    /* AC6C */
    SetAccumulatorWidth(cpu, 0);
    WorldMapCellOffset(memory, cpu, 0xac75u);
    WorldMapBlockPointer(memory, cpu, 0xac78u);
    LoadADirect16(memory, cpu, 0x0bu);
    And16(cpu, 0xff80u);
    Write16Long(memory, 0x001712u, cpu->accumulator);
    LoadADirect16(memory, cpu, 0x5au);
    And16(cpu, 0x0001u);
    AslA16(cpu);
    AslA16(cpu);
    StoreADirect16(memory, cpu, 0x13u);
    LoadADirect16(memory, cpu, 0x58u);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, 0x0040u);
    StoreADirect16(memory, cpu, 0x26u);
    do {
        LoadADirect16(memory, cpu, 0x58u);                     /* AC93 */
        And16(cpu, 0x0001u);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, 0x13u));
        StoreADirect16(memory, cpu, 0x00u);
        WorldMapMetatile(memory, cpu);
        LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0xe7u));
        SetAccumulatorWidth(cpu, 1);
        StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        ExchangeAccumulatorBytes(cpu);
        StoreAAbsolute8(memory, cpu, 0x0080u, cpu->x);
        LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0xeau));
        StoreAAbsolute8(memory, cpu, 0x0001u, cpu->x);
        LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0xedu));
        StoreAAbsolute8(memory, cpu, 0x0081u, cpu->x);
        SetAccumulatorWidth(cpu, 0);                           /* ACCB */
        LoadADirect16(memory, cpu, 0x0bu);
        IncrementA16(cpu);
        IncrementA16(cpu);
        cpu->zero = (cpu->accumulator & 0x007fu) == 0;
        if (cpu->zero)
            Subtract16(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x0bu);
        SetAccumulatorWidth(cpu, 1);
        IncrementDirect8(memory, cpu, 0x58u);
        SetAccumulatorWidth(cpu, 0);
        if (cpu->zero) {
            WorldMapBlockPointer(memory, cpu, 0xace6u);
        } else {
            LoadADirect16(memory, cpu, 0x58u);                 /* ACEA */
            LsrA16(cpu);
            if (!cpu->carry) {
                uint16_t pointer = Read16Direct(memory, cpu, 0x08u);

                pointer = (uint16_t)(pointer + 2u);
                Write16Direct(memory, cpu, 0x08u, pointer);
                SetNz16(cpu, pointer);
            }
        }
        {
            const uint16_t left =
                (uint16_t)(Read16Direct(memory, cpu, 0x26u) - 1u);

            Write16Direct(memory, cpu, 0x26u, left);
            SetNz16(cpu, left);
        }
    } while (!cpu->zero);
    PullAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, 0x58u);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* Edge ahead of the move: position +$1F or -$1F. */
static void WorldMapEdge(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t position) {
    Compare8(cpu, A8(cpu), 0x80u);
    LoadAAbsolute8(memory, cpu, position, 0);
    if (!cpu->carry)
        Adc8(cpu, 0x1fu);
    else
        Sbc8(cpu, 0x1fu);
}

/* $86:99BF: stream the map edges the camera moved across. */
Lufia2ExecutionResult Lufia2WorldMapStreamEdges(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x869a43u;
    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = 0x8699bfu;
        return result;
    }
    Write8(memory, DirectAddress(cpu, 0x59u), 0x00u);          /* 99BF */
    Write8(memory, DirectAddress(cpu, 0x5bu), 0x00u);
    LoadAAbsolute8(memory, cpu, 0x11f4u, 0);
    cpu->carry = 1;
    Sbc8(cpu, 0x20u);
    StoreADirect8(memory, cpu, 0x5au);
    LoadAAbsolute8(memory, cpu, 0x11f2u, 0);
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 0x11f6u, 0));
    if (!cpu->zero) {
        WorldMapEdge(memory, cpu, 0x11f2u);
        StoreADirect8(memory, cpu, 0x58u);                     /* 99EA */
        WorldMapStreamColumn(memory, cpu);
        StoreAImmediate8(memory, cpu, 0xffu, 0x1711u);
    }
    LoadAAbsolute8(memory, cpu, 0x11f2u, 0);                   /* 99F4 */
    cpu->carry = 1;
    Sbc8(cpu, 0x20u);
    StoreADirect8(memory, cpu, 0x58u);
    LoadAAbsolute8(memory, cpu, 0x11f4u, 0);
    cpu->carry = 1;
    Sbc8(cpu, AbsoluteByte(memory, cpu, 0x11f7u, 0));
    if (!cpu->zero) {
        WorldMapEdge(memory, cpu, 0x11f4u);
        StoreADirect8(memory, cpu, 0x5au);                     /* 9A1B */
        WorldMapStreamRow(memory, cpu);
        StoreAImmediate8(memory, cpu, 0xffu, 0x1710u);
    }
    LoadAAbsolute8(memory, cpu, 0x11f2u, 0);                   /* 9A25 */
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x11f6u, 0));
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x11f4u, 0);
        Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x11f7u, 0));
    }
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x11e3u, 0);               /* 9A35 */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        Compare8(cpu, A8(cpu), 0xffu);
        if (!cpu->carry)
            StoreAAbsolute8(memory, cpu, 0x11e3u, 0);
    }
    SimulateJsrFrame(memory, cpu, 0x9a42u);                    /* 9A44 */
    CopyAbsolute8(memory, cpu, 0x11f2u, 0x11f6u);
    CopyAbsolute8(memory, cpu, 0x11f4u, 0x11f7u);
    SimulateRtsFrame(memory, cpu);
    return result;
}

/* $86:9EDD: world map region holding ($58, $5A); carry clear = hit. */
Lufia2ExecutionResult Lufia2WorldMapRegionSearch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint8_t edges[4] = {0x01u, 0x03u, 0x02u, 0x04u};
    uint32_t entries;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x869eddu);
    PushDataBank(memory, cpu);                                 /* 9EDD */
    SimulateJsrFrame(memory, cpu, 0x9ee0u);
    SetAccumulatorWidth(cpu, 0);                               /* 9F35 */
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09ebu, 0));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xce36u, cpu->y));
    AslA16(cpu);
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0xce36u, cpu->y));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0xcffcbeu, cpu->x)));
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StoreADirect8(memory, cpu, 0x10u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0xcffcbcu, cpu->x)));
    TransferAToX(cpu);
    cpu->carry = 0;
    SimulateRtsFrame(memory, cpu);
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0006u, cpu->x));
    TransferAToX(cpu);                                         /* 9EE4 */
    for (entries = 0;; ++entries) {
        unsigned i;
        uint8_t inside = 1;

        /* A list without an end marker spins the ROM. */
        if (entries == 0x10000u) {
            SetAccumulatorWidth(cpu, 1);
            return ExecutionHandoff(cpu, 0x869ee7u);
        }
        SetAccumulatorWidth(cpu, 1);                           /* 9EE5 */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);
        if (cpu->negative) {
            cpu->carry = 1;                                    /* 9F10 */
            break;
        }
        for (i = 0; i < 4u && inside; ++i) {
            if (i == 0)
                LoadA8(cpu, DirectByte(memory, cpu, 0x58u));
            else if (i == 2)
                LoadA8(cpu, DirectByte(memory, cpu, 0x5au));
            Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, edges[i], cpu->x));
            inside = (i & 1u) ? !cpu->carry : cpu->carry;
        }
        if (inside)
            break;
        SetAccumulatorWidth(cpu, 0);                           /* 9F04 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x0009u);
        TransferAToX(cpu);
    }
    PullDataBank(memory, cpu);                                 /* 9F11 */
    return ExecutionReturned(0x869f12u);
}
