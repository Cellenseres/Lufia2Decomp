/* Menu NMI, input and window animation. */

#include "core/cpu_internal.h"

/* $82:8DA6: HDMA table [$F4] to [$F7], channel $F3 setup. */
static void MenuHdmaUpdate(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t return_bank,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, return_bank, return_address);
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);                   /* 8DA6 */
    And8(cpu, 0x01u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1565u, 0);
        LoadY16(cpu, 0x0000u);
        for (;;) {
            LoadA8(cpu, Read8(memory,                          /* 8DB3 */
                DirectLongIndirectY(memory, cpu, 0xf4u)));
            Write8(memory, DirectLongIndirectY(memory, cpu, 0xf7u), A8(cpu));
            if (cpu->zero)
                break;
            IncrementY16(cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory,
                DirectLongIndirectY(memory, cpu, 0xf4u)));
            Write16Long(memory, DirectLongIndirectY(memory, cpu, 0xf7u),
                cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            IncrementY16(cpu);
            IncrementY16(cpu);
        }
        SetAccumulatorWidth(cpu, 0);                           /* 8DC6 */
        LoadA16(cpu, Read16Direct(memory, cpu, 0xf3u));
        And16(cpu, 0x00ffu);
        LoadY16(cpu, cpu->accumulator);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        LsrA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Direct(memory, cpu, 0xf7u));
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x4302u, cpu->y),
            (uint8_t)cpu->accumulator);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x4303u, cpu->y),
            (uint8_t)(cpu->accumulator >> 8));
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0xf9u));
        StoreAAbsolute8(memory, cpu, 0x4304u, cpu->y);
        LoadA8(cpu, DirectByte(memory, cpu, 0xfau));
        StoreAAbsolute8(memory, cpu, 0x4301u, cpu->y);
        LoadA8(cpu, DirectByte(memory, cpu, 0xfbu));
        StoreAAbsolute8(memory, cpu, 0x4300u, cpu->y);
    }
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);                   /* 8DE9 */
    And8(cpu, 0x02u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1565u, 0);
        LoadA8(cpu, DirectByte(memory, cpu, 0xf2u));
        StoreAAbsolute8(memory, cpu, 0x420cu, 0);
    }
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);                   /* 8DF8 */
    And8(cpu, 0x04u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1565u, 0);
        StoreAImmediate8(memory, cpu, 0x04u, 0x212du);
        StoreAImmediate8(memory, cpu, 0x02u, 0x2130u);
        StoreAImmediate8(memory, cpu, 0x10u, 0x2131u);
    }
    SimulateRtlFrame(memory, cpu);
}

/* A += delta on the $7E:80C0 table entry at X. */
static void MenuTableStep8(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    int delta) {
    const uint32_t address = LongIndexedAddress(0x7e80c0u, cpu->x);

    LoadA8(cpu, (uint8_t)(Read8(memory, address) + delta));
    Write8(memory, address, A8(cpu));
}

static void MenuTableStep16(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    int delta) {
    const uint32_t address = LongIndexedAddress(0x7e80c0u, cpu->x);

    LoadA16(cpu, (uint16_t)(Read16Long(memory, address) + delta));
    Write16Long(memory, address, cpu->accumulator);
}

/* X += 3, 16-bit index. */
static void MenuTableNext(Lufia2ActorFrontendCpu *cpu) {
    IncrementX16(cpu);
    IncrementX16(cpu);
    IncrementX16(cpu);
}

/* $82:8F14: grow the window rows by three lines. */
static void MenuWindowGrow(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));  /* 8F14 */
    MenuTableStep8(memory, cpu, -3);
    SetAccumulatorWidth(cpu, 0);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1530u, 0));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
    IncrementX16(cpu);
    do {
        MenuTableStep16(memory, cpu, 3);                       /* 8F2B */
        MenuTableNext(cpu);
        cpu->y = (uint16_t)(cpu->y - 1u);
        SetNz16(cpu, cpu->y);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x153au, 0));
    MenuTableStep8(memory, cpu, 3);
    IncrementX16(cpu);
    SetAccumulatorWidth(cpu, 0);
    MenuTableStep16(memory, cpu, 3);
    SetAccumulatorWidth(cpu, 1);
}

/* BIT #mask; true when set. */
static uint8_t MenuBit(Lufia2ActorFrontendCpu *cpu, uint8_t mask) {
    BitImmediate8(cpu, mask);
    return !cpu->zero;
}

/* $82:8E12: window HDMA line table in $7E:80C0 by $1566 bit. */
static void MenuWindowLines(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x93b4u);
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);                   /* 8E12 */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        LoadY16(cpu, 0x0000u);
        for (;;) {
            LoadA8(cpu, Read8(memory,                          /* 8E1F */
                DirectLongIndirectY(memory, cpu, 0xf4u)));
            if (cpu->zero)
                break;
            Write8(memory, DirectLongIndirectY(memory, cpu, 0xf7u), A8(cpu));
            IncrementY16(cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory,
                DirectLongIndirectY(memory, cpu, 0xf4u)));
            Write16Long(memory, DirectLongIndirectY(memory, cpu, 0xf7u),
                cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            IncrementY16(cpu);
            IncrementY16(cpu);
        }
    } else if (MenuBit(cpu, 0x02u)) {                      /* 8E33 */
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1532u, 0));
        Write16Direct(memory, cpu, 0x33u, cpu->accumulator);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1530u, 0));
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        cpu->carry = 0;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1534u, 0));
        Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1530u, 0));
        IncrementY16(cpu);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
        IncrementX16(cpu);
        do {
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));    /* 8E56 */
            Subtract16(cpu, Read16Direct(memory, cpu, 0x33u));
            cpu->carry = 0;
            Add16Value(cpu, 0x0009u);
            Write16Long(memory, LongIndexedAddress(0x7e80c0u, cpu->x),
                cpu->accumulator);
            MenuTableNext(cpu);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x33u));
            cpu->carry = 0;
            Add16Value(cpu, 0x000cu);
            Write16Direct(memory, cpu, 0x33u, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
            Compare16(cpu, cpu->accumulator,
                Read16AbsoluteIndexed(memory, cpu, 0x1536u, 0));
            if (cpu->zero) {
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1534u, 0));
                Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
            }
            cpu->y = (uint16_t)(cpu->y - 1u);                  /* 8E80 */
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
        LoadA8(cpu, 0x03u);
        Write8(memory, LongIndexedAddress(0x7e80c0u, cpu->x), A8(cpu));
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x153au, 0));
        LoadA8(cpu, 0x09u);
        Write8(memory, LongIndexedAddress(0x7e80c0u, cpu->x), A8(cpu));
    } else if (MenuBit(cpu, 0x04u)) {                      /* 8E98 */
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
        MenuTableStep8(memory, cpu, 3);
        SetAccumulatorWidth(cpu, 0);
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1530u, 0));
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1538u, 0));
        IncrementX16(cpu);
        do {
            MenuTableStep16(memory, cpu, -3);                  /* 8EB8 */
            MenuTableNext(cpu);
            cpu->y = (uint16_t)(cpu->y - 1u);
            SetNz16(cpu, cpu->y);
        } while (!cpu->zero);
        SetAccumulatorWidth(cpu, 1);
        cpu->x = (uint16_t)(cpu->x - 1u);
        SetNz16(cpu, cpu->x);
        MenuTableStep8(memory, cpu, -3);
        IncrementX16(cpu);
        SetAccumulatorWidth(cpu, 0);
        MenuTableStep16(memory, cpu, -3);
        SetAccumulatorWidth(cpu, 1);
    } else if (MenuBit(cpu, 0x08u)) {                      /* 8EE8 */
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x153au, 0));
        MenuTableStep8(memory, cpu, -1);
        IncrementX16(cpu);
        SetAccumulatorWidth(cpu, 0);
        MenuTableStep16(memory, cpu, 1);
        SetAccumulatorWidth(cpu, 1);
        MenuWindowGrow(memory, cpu);
    } else if (MenuBit(cpu, 0x10u)) {                      /* 8F0D */
        StoreZeroAbsolute8(memory, cpu, 0x1566u, 0);
        MenuWindowGrow(memory, cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $82:97A6: step menu sprites along the $8E:E4D8 offset lists. */
static void MenuSpriteSteps(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x93bcu);
    SetIndexWidth(cpu, 1);                                     /* 97A6 */
    BattleSetDataBank(memory, cpu, 0x8eu);
    Write8(memory, DirectAddress(cpu, 0x33u), 0x00u);
    LoadX16(cpu, 0x0010u);
    do {
        LoadAAbsolute8(memory, cpu, 0x11d8u, cpu->x);          /* 97B1 */
        if (!cpu->zero) {
            cpu->y = Read8(memory,
                AbsoluteIndexedAddress(cpu, 0x1208u, cpu->x));
            SetNz8(cpu, (uint8_t)cpu->y);
            LoadAAbsolute8(memory, cpu, 0xe4d8u, cpu->y);
            Compare8(cpu, A8(cpu), 0xaau);
            if (cpu->zero) {
                StoreZeroAbsolute8(memory, cpu, 0x11d8u, cpu->x);
            } else {
                const uint32_t step =
                    AbsoluteIndexedAddress(cpu, 0x1208u, cpu->x);
                uint8_t next;

                cpu->carry = 0;                                /* 97C5 */
                Adc8(cpu, Read8(memory,
                    AbsoluteIndexedAddress(cpu, 0x13e8u, cpu->x)));
                StoreAAbsolute8(memory, cpu, 0x13e8u, cpu->x);
                next = (uint8_t)(Read8(memory, step) + 1u);
                Write8(memory, step, next);
                SetNz8(cpu, next);
                LoadA8(cpu, 0x01u);
                StoreADirect8(memory, cpu, 0x33u);
            }
        }
        cpu->x = (uint8_t)(cpu->x + 1u);                       /* 97D3 */
        Compare8(cpu, (uint8_t)cpu->x, 0x30u);
    } while (!cpu->zero);
    LoadA8(cpu, DirectByte(memory, cpu, 0x33u));
    if (cpu->zero)
        StoreZeroAbsolute8(memory, cpu, 0x1567u, 0);
    SetIndexWidth(cpu, 0);                                     /* 97DF */
    PullDataBank(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* $86:8809: patch count/value pairs into a $7E:[$3B] HDMA table. */
static void SelectHdmaPatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t table,
    uint16_t return_address) {
    LoadY16(cpu, table);
    SimulateJsrFrame(memory, cpu, return_address);
    Write16Direct(memory, cpu, 0x3bu, cpu->y);                 /* 8809 */
    LoadY16(cpu, 0x0000u);
    for (;;) {
        LoadA8(cpu, Read8(memory,                              /* 880E */
            DirectLongIndirectY(memory, cpu, 0x3bu)));
        if (cpu->zero)
            break;
        IncrementY16(cpu);
        LoadAAbsolute8(memory, cpu, 0x1597u, cpu->x);
        Write8(memory, DirectLongIndirectY(memory, cpu, 0x3bu), A8(cpu));
        IncrementY16(cpu);
        LoadAAbsolute8(memory, cpu, 0x15a7u, cpu->x);
        Write8(memory, DirectLongIndirectY(memory, cpu, 0x3bu), A8(cpu));
        IncrementY16(cpu);
        IncrementX16(cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* DMA channel 6: $7E:X to VRAM Y, count bytes. */
static void SelectVramDma(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t count) {
    StoreWordAbsolute(memory, cpu, 0x2116u, cpu->y);
    StoreWordAbsolute(memory, cpu, 0x4362u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x01u, 0x4360u);
    StoreAImmediate8(memory, cpu, 0x7eu, 0x4364u);
    StoreAImmediate8(memory, cpu, 0x18u, 0x4361u);
    LoadX16(cpu, count);
    StoreWordAbsolute(memory, cpu, 0x4365u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x40u, 0x420bu);
}

/* $86:87E0: HDMA table patches and the $15B8 row upload. */
static void SelectNmiRedraw(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x81c1u);
    SetAccumulatorWidth(cpu, 1);                               /* 87E0 */
    SetIndexWidth(cpu, 0);
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);
    And8(cpu, 0x01u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1566u, 0);
        LoadX16(cpu, 0x0000u);
        LoadA8(cpu, 0x7eu);
        StoreADirect8(memory, cpu, 0x3du);
        SelectHdmaPatch(memory, cpu, 0x80c0u, 0x87fau);
        SelectHdmaPatch(memory, cpu, 0x81c0u, 0x8800u);
        SelectHdmaPatch(memory, cpu, 0x82c0u, 0x8806u);
    }
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);                   /* 8823 */
    And8(cpu, 0x02u);
    if (!cpu->zero) {
        TestBitsAbsolute8(memory, cpu, 0x1566u, 0);
        StoreAImmediate8(memory, cpu, 0x81u, 0x2115u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15b8u, 0));
        And16(cpu, 0x00ffu);
        cpu->carry = 0;
        Add16Value(cpu, 0x1800u);
        LoadY16(cpu, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0xc3c0u);
        SelectVramDma(memory, cpu, 0x003cu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $82:8D44: tilemap uploads by $1568 bits. */
static void SelectTilemapUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const struct {
        uint8_t mask;
        uint16_t source;
        uint16_t vram;
        uint16_t return_address;
    } uploads[3] = {
        {0xc0u, 0x2000u, 0x1000u, 0x8d5cu},
        {0x30u, 0x2800u, 0x1400u, 0x8d6cu},
        {0x0cu, 0x3000u, 0x1800u, 0x8d7cu}};
    unsigned i;

    SimulateJslFrame(memory, cpu, 0x86u, 0x81cau);
    SetAccumulatorWidth(cpu, 1);                               /* 8D44 */
    SetIndexWidth(cpu, 0);
    StoreAImmediate8(memory, cpu, 0x80u, 0x2115u);
    for (i = 0; i < 3u; ++i) {
        LoadAAbsolute8(memory, cpu, 0x1568u, 0);
        And8(cpu, uploads[i].mask);
        if (cpu->zero)
            continue;
        LoadX16(cpu, uploads[i].source);
        LoadY16(cpu, uploads[i].vram);
        SimulateJsrFrame(memory, cpu, uploads[i].return_address);
        PushAccumulator8(memory, cpu);                         /* 8D83 */
        SelectVramDma(memory, cpu, 0x0800u);
        LoadA8(cpu, Pull8(memory, cpu));
        SimulateRtsFrame(memory, cpu);
    }
    LoadA8(cpu, 0xaau);                                        /* 8D7D */
    TestBitsAbsolute8(memory, cpu, 0x1568u, 0);
    SimulateRtlFrame(memory, cpu);
}

/* $82:939C: menu NMI with its three redraw requests. */
Lufia2ActorPrimaryUpdateResult Lufia2MenuNmi(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (cpu->accumulator_is_8_bit)                             /* 939C */
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);
    if (!cpu->zero)
        MenuHdmaUpdate(memory, cpu, 0x82u, 0x93acu);
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);                   /* 93AD */
    if (!cpu->zero)
        MenuWindowLines(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x1567u, 0);                   /* 93B5 */
    if (!cpu->zero)
        MenuSpriteSteps(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 93BD */
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return FieldLoopResult(0x8293c1u);
}

/* $86:81A9: NMI installed by $86:8000. */
Lufia2ActorPrimaryUpdateResult Lufia2SelectScreenNmi(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (cpu->accumulator_is_8_bit)                             /* 81A9 */
        PushAccumulator8(memory, cpu);
    else
        PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadAAbsolute8(memory, cpu, 0x1565u, 0);
    if (!cpu->zero)
        MenuHdmaUpdate(memory, cpu, 0x86u, 0x81b9u);
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);                   /* 81BA */
    if (!cpu->zero)
        SelectNmiRedraw(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x1568u, 0);                   /* 81C2 */
    if (!cpu->zero)
        SelectTilemapUploads(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 81CB */
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        LoadA8(cpu, Pull8(memory, cpu));
    else
        PullAccumulator16(memory, cpu);
    return FieldLoopResult(0x8681cfu);
}

/* $82:8B4B: menu buttons into $14AB/$14AC; carry = none. */
Lufia2ActorPrimaryUpdateResult Lufia2MenuButtons(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint8_t bits[12] = {
        0x80u, 0x40u, 0x20u, 0x10u,
        0x80u, 0x40u, 0x20u, 0x10u, 0x08u, 0x04u, 0x02u, 0x01u};
    unsigned i;

    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x828b4bu);
    StoreZeroAbsolute8(memory, cpu, 0x14abu, 0);               /* 8B4B */
    StoreZeroAbsolute8(memory, cpu, 0x14acu, 0);
    for (i = 0; i < 12u; ++i) {
        const uint8_t latch = i < 4u ? 0x4au : 0x4bu;

        LoadA8(cpu, bits[i]);
        And8(cpu, DirectByte(memory, cpu, latch));
        if (cpu->zero)
            continue;
        LoadA8(cpu, bits[i]);
        And8(cpu, DirectByte(memory, cpu, (uint8_t)(latch - 4u)));
        if (cpu->zero)
            continue;
        TestBitsDirect(memory, cpu, latch, 0);
        LoadA8(cpu, bits[i]);
        TestBitsAbsolute8(memory, cpu, i < 4u ? 0x14abu : 0x14acu, 1);
    }
    LoadAAbsolute8(memory, cpu, 0x14abu, 0);                   /* 8C35 */
    if (cpu->zero)
        LoadAAbsolute8(memory, cpu, 0x14acu, 0);
    cpu->carry = cpu->zero;
    return FieldLoopResult(cpu->zero ? 0x828c40u : 0x828c42u);
}

/* $82:9313: menu window refresh request; the upload runs on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2MenuWindowRequest(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return FieldLoopHandoff(cpu, 0x829313u);
    LoadAAbsolute8(memory, cpu, 0x156au, 0);                   /* 9313 */
    if (cpu->zero)
        return FieldLoopResult(0x82932fu);
    StoreAImmediate8(memory, cpu, 0x20u, 0x0564u);
    LoadA8(cpu, 0x8eu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadY16(cpu, 0xd4deu);
    LoadX16(cpu, 0x35e0u);
    return FieldLoopHandoff(cpu, 0x829327u);
}

/* $82:C627: menu cursor blink, toggles $1552 every $20 frames. */
Lufia2ActorPrimaryUpdateResult Lufia2MenuCursorBlink(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x82c627u);
    LoadAAbsolute8(memory, cpu, 0x09c8u, 0);                   /* C627 */
    if (!cpu->zero)
        LoadAAbsolute8(memory, cpu, 0x1553u, 0);
    if (!cpu->zero) {
        const uint32_t timer = AbsoluteIndexedAddress(cpu, 0x1554u, 0);
        const uint8_t count = (uint8_t)(Read8(memory, timer) + 1u);

        Write8(memory, timer, count);
        LoadAAbsolute8(memory, cpu, 0x1554u, 0);
        Compare8(cpu, A8(cpu), 0x20u);
        if (cpu->zero) {
            StoreZeroAbsolute8(memory, cpu, 0x1554u, 0);
            LoadAAbsolute8(memory, cpu, 0x1552u, 0);
            LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0x01u));
            StoreAAbsolute8(memory, cpu, 0x1552u, 0);
        }
    }
    return FieldLoopResult(0x82c646u);
}
