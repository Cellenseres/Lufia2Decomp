#include "core/cpu_internal.h"
#include "actor/actor_internal.h"
#include "field/field_internal.h"
#include "system/system_internal.h"
#include "text/text_internal.h"

void Lufia2QueueDeferredSound(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    ExchangeAccumulatorBytes(cpu);                             /* 8766 */
    LoadA8(cpu, Read8(memory, 0x0005b6u));
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        ExchangeAccumulatorBytes(cpu);                         /* 876F */
        Write8(memory, 0x0017acu, A8(cpu));
    }
}

/* $80:832D: lagged XOR refill, lags 24 and 31. */
static void RandomRefill(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadX8(cpu, 0x00u);                                        /* 832D */
    do {
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x)));
        LoadA8(
            cpu, (uint8_t)(A8(cpu) ^ Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0540u, cpu->x))));
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x),
            A8(cpu));
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        Compare8(cpu, (uint8_t)cpu->x, 0x18u);                 /* 8339 */
    } while (!cpu->zero);
    do {
        LoadA8(
            cpu, Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x)));
        LoadA8(
            cpu, (uint8_t)(A8(cpu) ^ Read8(
                memory, AbsoluteIndexedAddress(cpu, 0x0509u, cpu->x))));
        Write8(
            memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x),
            A8(cpu));
        LoadX8(cpu, (uint8_t)(cpu->x + 1u));
        Compare8(cpu, (uint8_t)cpu->x, 0x37u);                 /* 8347 */
    } while (!cpu->zero);
}

/* PHB/PHK/PLB/PHX/PHY/PHP/SEP #$30 */
static void RandomEnter(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x80u);
    PullDataBank(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
}

/* Next table index in X, refilling past $36. */
static void RandomAdvance(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t refill_return) {
    LoadX8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x0559u, 0)));
    LoadX8(cpu, (uint8_t)(cpu->x + 1u));
    Compare8(cpu, (uint8_t)cpu->x, 0x37u);
    if (cpu->carry) {
        SimulateJsrFrame(memory, cpu, refill_return);
        RandomRefill(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadX8(cpu, 0x00u);
    }
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x0559u, 0),
        (uint8_t)cpu->x);
}

/* PLP/PLY/PLX/PLB */
static void RandomLeave(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    UnpackStatus(cpu, Pull8(memory, cpu));
    cpu->y = PullIndexValue(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    PullDataBank(memory, cpu);
}

void Lufia2RandomByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    RandomEnter(memory, cpu);                                  /* 82C7 */
    RandomAdvance(memory, cpu, 0x82d9u);                       /* 82CF */
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x)));
    RandomLeave(memory, cpu);                                  /* 82E2 */
}

void Lufia2RandomScale(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    RandomEnter(memory, cpu);                                  /* 8299 */
    ExchangeAccumulatorBytes(cpu);                             /* 82A1 */
    RandomAdvance(memory, cpu, 0x82acu);                       /* 82A2 */
    ExchangeAccumulatorBytes(cpu);                             /* 82B2 */
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x4202u, 0), A8(cpu));
    LoadA8(
        cpu, Read8(
            memory, AbsoluteIndexedAddress(cpu, 0x0521u, cpu->x)));
    Write8(
        memory, AbsoluteIndexedAddress(cpu, 0x4203u, 0), A8(cpu));
    LoadA8(cpu, 0x00u);                                        /* 82BC */
    ExchangeAccumulatorBytes(cpu);                             /* 82BE */
    LoadA8(
        cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x4217u, 0)));
    RandomLeave(memory, cpu);                                  /* 82C2 */
}

void Lufia2CallRandomScale(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Lufia2RandomScale(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

void Lufia2CallRandomByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x83u, return_address);
    Lufia2RandomByte(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $86:D1E8: tile column upload, rows of $0100 words. */
static void WorldMapColumnUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xd1d8u);
    SetAccumulatorWidth(cpu, 0);                               /* D1E8 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->y));
    Write16Absolute(memory, cpu, 0x4362u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    StoreAAbsolute8(memory, cpu, 0x4364u, 0);
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
        Write16Absolute(memory, cpu, 0x4365u, cpu->accumulator);
        LoadADirect16(memory, cpu, 0x35u);
        Write16Absolute(memory, cpu, 0x4375u, cpu->accumulator);
        LoadADirect16(memory, cpu, 0x39u);
        Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
        cpu->carry = 0;
        Add16Value(cpu, 0x0100u);
        StoreADirect16(memory, cpu, 0x39u);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, DirectByte(memory, cpu, 0x05u));
        StoreAAbsolute8(memory, cpu, 0x420bu, 0);
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
            Write16Absolute(memory, cpu, 0x4375u, cpu->accumulator);
            LoadADirect16(memory, cpu, 0x39u);
            Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, 0x0100u);
            StoreADirect16(memory, cpu, 0x39u);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
            DecrementDirect8(memory, cpu, 0x37u);
        } while (!cpu->zero);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $86:D271: block upload, two passes of rows $0200 apart. */
static void WorldMapBlockUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    unsigned pass;

    SimulateJsrFrame(memory, cpu, 0xd1ddu);
    SetAccumulatorWidth(cpu, 0);                               /* D271 */
    And16(cpu, 0x007fu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->y));
    Write16Absolute(memory, cpu, 0x4362u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    StoreAAbsolute8(memory, cpu, 0x4364u, 0);
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
            Write16Absolute(memory, cpu, 0x4365u, cpu->accumulator);
            TransferXToA(cpu);
            Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, 0x0200u);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0x40u, 0x420bu);
            DecrementDirect8(memory, cpu, pass ? 0x38u : 0x37u);
        } while (!cpu->zero);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $86:D1A1: pending map tile uploads, $1365 entries. */
static void WorldMapTileUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xcf14u);
    LoadA8(cpu, 0x18u);                                        /* D1A1 */
    StoreAAbsolute8(memory, cpu, 0x4361u, 0);
    StoreAAbsolute8(memory, cpu, 0x4371u, 0);
    StoreAImmediate8(memory, cpu, 0x01u, 0x4360u);
    StoreAImmediate8(memory, cpu, 0x09u, 0x4370u);
    LoadX16(cpu, 0xd1e7u);
    Write16Absolute(memory, cpu, 0x4372u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x86u, 0x4374u);
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
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
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
            StoreAAbsolute8(memory, cpu, 0x2121u, 0);
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
                LoadAAbsolute8(memory, cpu, 0x0320u, cpu->y);  /* CFF8 */
                StoreAAbsolute8(memory, cpu, 0x2122u, 0);
                IncrementY16(cpu);
                LoadAAbsolute8(memory, cpu, 0x0320u, cpu->y);
                StoreAAbsolute8(memory, cpu, 0x2122u, 0);
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
                    LoadAAbsolute8(memory, cpu, 0x0320u, cpu->y);  /* D012 */
                    StoreAAbsolute8(memory, cpu, 0x2122u, 0);
                    IncrementY16(cpu);
                    LoadAAbsolute8(memory, cpu, 0x0320u, cpu->y);
                    StoreAAbsolute8(memory, cpu, 0x2122u, 0);
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
Lufia2ActorPrimaryUpdateResult Lufia2WorldMapNmiUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint16_t mode7_regs[8] = {
        0x211bu, 0x211bu, 0x211cu, 0x211cu, 0x211du, 0x211du, 0x211eu,
        0x211eu};
    static const uint16_t scroll_regs[6] = {
        0x210du, 0x210eu, 0x210fu, 0x2110u, 0x2111u, 0x2112u};
    Lufia2ActorPrimaryUpdateResult result;
    unsigned i;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x86d1a0u;
    result.dispatches = 0;
    Push8(memory, cpu, PackStatus(cpu));                       /* CEF6 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, 0x86u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    StoreAImmediate8(memory, cpu, 0x8fu, 0x2100u);
    StoreZeroAbsolute8(memory, cpu, 0x420cu, 0);
    LoadAAbsolute8(memory, cpu, 0x11d9u, 0);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x1365u, 0);
        if (!cpu->zero)
            WorldMapTileUploads(memory, cpu);
    }
    StoreZeroAbsolute8(memory, cpu, 0x4370u, 0);               /* CF15 */
    StoreAImmediate8(memory, cpu, 0x18u, 0x4371u);
    LoadAAbsolute8(memory, cpu, 0x1710u, 0);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, 0x2115u, 0);
        StoreAImmediate8(memory, cpu, 0x7fu, 0x4374u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1712u, 0));
        Write16Absolute(memory, cpu, 0x4372u, cpu->accumulator);
        And16(cpu, 0x3fffu);
        Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0x0100u);
        Write16Absolute(memory, cpu, 0x4375u, cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
        StoreZeroAbsolute8(memory, cpu, 0x1710u, 0);
    }
    LoadAAbsolute8(memory, cpu, 0x1711u, 0);                   /* CF48 */
    if (!cpu->zero) {
        StoreAImmediate8(memory, cpu, 0x03u, 0x2115u);
        StoreAImmediate8(memory, cpu, 0x7fu, 0x4374u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1714u, 0));
        And16(cpu, 0x3fffu);
        Write16Absolute(memory, cpu, 0x2116u, cpu->accumulator);
        IncrementA16(cpu);
        PushAccumulator16(memory, cpu);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0xdf00u);
        Write16Absolute(memory, cpu, 0x4372u, cpu->x);
        LoadX16(cpu, 0x0080u);
        Write16Absolute(memory, cpu, 0x4375u, cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
        cpu->y = PullIndexValue(memory, cpu);
        Write16Absolute(memory, cpu, 0x2116u, cpu->y);
        Write16Absolute(memory, cpu, 0x4375u, cpu->x);
        StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
        StoreZeroAbsolute8(memory, cpu, 0x1711u, 0);
    }
    StoreAImmediate8(memory, cpu, 0x80u, 0x2115u);             /* CF86 */
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1702u, 0));
    if (!cpu->zero) {
        Write16Absolute(memory, cpu, 0x4375u, cpu->x);
        CopyAbsolute8(memory, cpu, 0x1701u, 0x2121u);
        SetAccumulatorWidth(cpu, 0);
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1704u, 0));
        Write16Absolute(memory, cpu, 0x4372u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        CopyAbsolute8(memory, cpu, 0x1706u, 0x4374u);
        StoreZeroAbsolute8(memory, cpu, 0x4370u, 0);
        StoreAImmediate8(memory, cpu, 0x22u, 0x4371u);
        StoreAImmediate8(memory, cpu, 0x80u, 0x420bu);
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
            StoreAAbsolute8(memory, cpu, 0x4300u, 0);
            StoreAAbsolute8(memory, cpu, 0x4310u, 0);
            StoreAImmediate8(memory, cpu, 0x1bu, 0x4301u);
            StoreAImmediate8(memory, cpu, 0x1du, 0x4311u);
            StoreAImmediate8(memory, cpu, 0x00u, 0x4304u);
            StoreAImmediate8(memory, cpu, 0x00u, 0x4314u);
            LoadX16(cpu, 0x1718u);
            Write16Absolute(memory, cpu, 0x4302u, cpu->x);
            LoadX16(cpu, 0x1a9bu);
            Write16Absolute(memory, cpu, 0x4312u, cpu->x);
            LoadA8(cpu, 0x03u);
            TestBitsDirect(memory, cpu, 0x33u, 1);
        }
        LoadAAbsolute8(memory, cpu, 0x11ddu, 0);               /* D09C */
        if (cpu->zero) {
            StoreZeroAbsolute8(memory, cpu, 0x2130u, 0);
            CopyAbsolute8(memory, cpu, 0x170fu, 0x2131u);
            StoreZeroAbsolute8(memory, cpu, 0x4340u, 0);
            StoreAImmediate8(memory, cpu, 0x32u, 0x4341u);
            StoreZeroAbsolute8(memory, cpu, 0x4344u, 0);
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1716u, 0));
            Write16Absolute(memory, cpu, 0x4342u, cpu->x);
            LoadA8(cpu, 0x10u);
            TestBitsDirect(memory, cpu, 0x33u, 1);
        }
    }
    LoadAAbsolute8(memory, cpu, 0x11dfu, 0);                   /* D0BF */
    if (!cpu->zero) {
        LoadA8(cpu, 0x43u);
        StoreAAbsolute8(memory, cpu, 0x4340u, 0);
        StoreAAbsolute8(memory, cpu, 0x4350u, 0);
        LoadX16(cpu, 0xde00u);
        Write16Absolute(memory, cpu, 0x4342u, cpu->x);
        LoadX16(cpu, 0xe000u);
        Write16Absolute(memory, cpu, 0x4352u, cpu->x);
        LoadA8(cpu, 0x30u);
        TestBitsDirect(memory, cpu, 0x33u, 1);
    }
    LoadAAbsolute8(memory, cpu, 0x11e1u, 0);                   /* D0DC */
    if (!cpu->zero) {
        LoadA8(cpu, 0x41u);
        StoreAAbsolute8(memory, cpu, 0x4320u, 0);
        StoreAAbsolute8(memory, cpu, 0x4330u, 0);
        StoreAImmediate8(memory, cpu, 0x26u, 0x4321u);
        StoreAImmediate8(memory, cpu, 0x28u, 0x4331u);
        LoadA8(cpu, 0x7fu);
        StoreAAbsolute8(memory, cpu, 0x4324u, 0);
        StoreAAbsolute8(memory, cpu, 0x4327u, 0);
        StoreAAbsolute8(memory, cpu, 0x4334u, 0);
        StoreAAbsolute8(memory, cpu, 0x4337u, 0);
        LoadY16(cpu, 0xd400u);
        LoadAAbsolute8(memory, cpu, 0x11dbu, 0);
        LsrA8(cpu);
        if (cpu->carry)
            LoadY16(cpu, 0xd600u);
        Write16Absolute(memory, cpu, 0x4322u, cpu->y);
        LoadY16(cpu, 0xd800u);
        LoadAAbsolute8(memory, cpu, 0x11dcu, 0);
        LsrA8(cpu);
        if (cpu->carry)
            LoadY16(cpu, 0xda00u);
        Write16Absolute(memory, cpu, 0x4332u, cpu->y);
        LoadA8(cpu, 0x0cu);
        TestBitsDirect(memory, cpu, 0x33u, 1);
        LoadY16(cpu, 0x00ffu);
        Write16Absolute(memory, cpu, 0x2126u, cpu->y);
        Write16Absolute(memory, cpu, 0x2128u, cpu->y);
    }
    LoadAAbsolute8(memory, cpu, 0x11d8u, 0);                   /* D12C */
    if (!cpu->zero) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x33u));
        StoreAAbsolute8(memory, cpu, 0x420cu, 0);
    }
    for (i = 0; i < 4u; ++i)                                   /* D136 */
        CopyAbsolute8(memory, cpu, (uint16_t)(0x11f8u + i),
            (uint16_t)(0x211fu + (i >> 1)));
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
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
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
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
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
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
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
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x99eeu);
    BattleSetDataBank(memory, cpu, 0x7fu);                     /* ACFE */
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
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9a1fu);
    BattleSetDataBank(memory, cpu, 0x7fu);                     /* AC6C */
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
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t position) {
    Compare8(cpu, A8(cpu), 0x80u);
    LoadAAbsolute8(memory, cpu, position, 0);
    if (!cpu->carry)
        Adc8(cpu, 0x1fu);
    else
        Sbc8(cpu, 0x1fu);
}

/* $86:99BF: stream the map edges the camera moved across. */
Lufia2ActorPrimaryUpdateResult Lufia2WorldMapStreamEdges(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x869a43u;
    result.dispatches = 0;
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
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

/* $82:E746: JSR $8028 inline table on $30; handlers on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2TitleStateDispatch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint32_t table;
    uint16_t target;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return FieldLoopHandoff(cpu, 0x82e746u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x30u));               /* E746 */
    SimulateJsrFrame(memory, cpu, 0xe74au);
    SetAccumulatorWidth(cpu, 0);                               /* 8028 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    IncrementA16(cpu);
    TransferAToY(cpu);
    PullAccumulator16(memory, cpu);
    StoreADirect16(memory, cpu, 0x5du);
    SetAccumulatorWidth(cpu, 1);
    Push8(memory, cpu, 0x82u);                                 /* PHK */
    LoadA8(cpu, Pull8(memory, cpu));
    StoreADirect8(memory, cpu, 0x5fu);
    SetAccumulatorWidth(cpu, 0);
    table = Read16Direct(memory, cpu, 0x5du) |
        ((uint32_t)DirectByte(memory, cpu, 0x5fu) << 16);
    LoadA16(cpu, Read16Long(memory, (table + cpu->y) & 0x00ffffffu));
    StoreADirect16(memory, cpu, 0x60u);
    SetAccumulatorWidth(cpu, 1);
    target = (uint16_t)(Read8(memory, 0x000060u) |
        ((uint16_t)Read8(memory, 0x000061u) << 8));            /* JMP ($0060) */
    {
        Lufia2ActorPrimaryUpdateResult result =
            FieldLoopHandoff(cpu, 0x820000u | target);

        /* The ROM passed these at this S already. */
        result.dispatches = target == 0xe746u || target == 0xe748u ||
            (target >= 0x8031u && target <= 0x8041u && (target & 1u));
        return result;
    }
}

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

/* $80:9357: DMA channel 6, $700 bytes from $7E:X to VRAM Y. */
static void IntroVramDma(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreWordAbsolute(memory, cpu, 0x4362u, cpu->x);           /* 9357 */
    StoreWordAbsolute(memory, cpu, 0x2116u, cpu->y);
    StoreAImmediate8(memory, cpu, 0x7eu, 0x4364u);
    LoadX16(cpu, 0x0700u);
    StoreWordAbsolute(memory, cpu, 0x4365u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x01u, 0x4360u);
    StoreAImmediate8(memory, cpu, 0x18u, 0x4361u);
    StoreAImmediate8(memory, cpu, 0x40u, 0x420bu);
    SimulateRtsFrame(memory, cpu);
}

/* $80:92FE/$80:9346: logo tiles from $7E:X, then the next state. */
static void IntroLogoUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t source,
    uint16_t return_address) {
    LoadX16(cpu, source);
    LoadY16(cpu, 0x0000u);
    IntroVramDma(memory, cpu, return_address);
    IncrementDirect8(memory, cpu, 0x50u);
    Write8(memory, DirectAddress(cpu, 0x4eu), 0x00u);
}

/* $80:92A4: intro NMI, state $50 through the table $80:92B7. */
Lufia2ActorPrimaryUpdateResult Lufia2IntroNmi(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    uint16_t handler;

    Push8(memory, cpu, PackStatus(cpu));                       /* 92A4 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x50u));
    AslA8(cpu);
    TransferAToX(cpu);
    handler = (uint16_t)(
        Read8(memory, ProgramAddress(cpu, (uint16_t)(0x92b7u + cpu->x))) |
        (Read8(memory, ProgramAddress(cpu, (uint16_t)(0x92b8u + cpu->x)))
            << 8));
    if (handler != 0x92cbu && handler != 0x92feu && handler != 0x930cu &&
        handler != 0x9320u && handler != 0x9330u && handler != 0x9346u &&
        handler != 0x9354u)
        return FieldLoopHandoff(cpu, 0x8092b1u);
    SimulateJsrFrame(memory, cpu, 0x92b3u);
    switch (handler) {
    case 0x92cbu:                                  /* scroll row upload */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, 0x7e4004u));
        cpu->carry = 0;
        Add16Value(cpu, 0x4000u);
        StoreWordAbsolute(memory, cpu, 0x4362u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7e4006u));
        StoreWordAbsolute(memory, cpu, 0x4365u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0x4000u);
        StoreWordAbsolute(memory, cpu, 0x2116u, cpu->x);
        StoreAImmediate8(memory, cpu, 0x7eu, 0x4364u);
        StoreAImmediate8(memory, cpu, 0x01u, 0x4360u);
        StoreAImmediate8(memory, cpu, 0x18u, 0x4361u);
        StoreAImmediate8(memory, cpu, 0x40u, 0x420bu);
        IncrementDirect8(memory, cpu, 0x50u);
        break;
    case 0x92feu:
        IntroLogoUpload(memory, cpu, 0x2000u, 0x9306u);
        break;
    case 0x930cu:                                  /* fade in over 32 */
        LoadA8(cpu, DirectByte(memory, cpu, 0x4eu));
        LsrA8(cpu);
        StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x4eu) + 1u));
        StoreADirect8(memory, cpu, 0x4eu);
        Compare8(cpu, A8(cpu), 0x20u);
        if (cpu->carry) {
            IncrementDirect8(memory, cpu, 0x50u);
            Write8(memory, DirectAddress(cpu, 0x4eu), 0x00u);
        }
        break;
    case 0x9320u:                                  /* hold 120 frames */
        LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x4eu) + 1u));
        StoreADirect8(memory, cpu, 0x4eu);
        Compare8(cpu, A8(cpu), 0x78u);
        if (cpu->carry) {
            LoadA8(cpu, 0x20u);
            StoreADirect8(memory, cpu, 0x4eu);
            IncrementDirect8(memory, cpu, 0x50u);
        }
        break;
    case 0x9330u:                                  /* fade out */
        LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, 0x4eu) - 1u));
        StoreADirect8(memory, cpu, 0x4eu);
        if (cpu->negative) {
            Write8(memory, DirectAddress(cpu, 0x4eu), 0x00u);
            IncrementDirect8(memory, cpu, 0x50u);
            StoreAImmediate8(memory, cpu, 0x80u, 0x0583u);
        } else {
            LsrA8(cpu);
            StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        }
        break;
    case 0x9346u:
        IntroLogoUpload(memory, cpu, 0x2800u, 0x934eu);
        break;
    default:                                       /* 9354 */
        Write8(memory, DirectAddress(cpu, 0x6au), 0x00u);
        break;
    }
    SimulateRtsFrame(memory, cpu);
    PullDataBank(memory, cpu);                                 /* 92B4 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    return FieldLoopResult(0x8092b6u);
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

/* $80:86C1: screen fade from $0581 into the $0583 brightness. */
Lufia2ActorPrimaryUpdateResult Lufia2ScreenFade(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x8086c1u);
    LoadAAbsolute8(memory, cpu, 0x0581u, 0);                   /* 86C1 */
    if (!cpu->negative)
        return FieldLoopResult(0x808702u);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x0581u, 0);
    And8(cpu, 0x3fu);
    cpu->carry = 0;
    Adc8(cpu, AbsoluteByte(memory, cpu, 0x0582u, 0));
    StoreAAbsolute8(memory, cpu, 0x0582u, 0);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    ExchangeAccumulatorBytes(cpu);
    BitImmediate8(cpu, 0x40u);
    if (!cpu->zero) {
        ExchangeAccumulatorBytes(cpu);                         /* 86DB */
        cpu->carry = 1;
        Sbc8(cpu, 0x0fu);
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        DecrementA8(cpu);
        if (!cpu->negative)
            return FieldLoopResult(0x808702u);
        StoreZeroAbsolute8(memory, cpu, 0x0581u, 0);
        StoreAImmediate8(memory, cpu, 0x80u, 0x0583u);
    } else {
        ExchangeAccumulatorBytes(cpu);                         /* 86F2 */
        StoreAAbsolute8(memory, cpu, 0x0583u, 0);
        Compare8(cpu, A8(cpu), 0x0fu);
        if (!cpu->carry)
            return FieldLoopResult(0x808702u);
        StoreZeroAbsolute8(memory, cpu, 0x0581u, 0);
        StoreAImmediate8(memory, cpu, 0x0fu, 0x0583u);
    }
    return FieldLoopResult(0x808702u);
}

/* $86:9EDD: world map region holding ($58, $5A); carry clear = hit. */
Lufia2ActorPrimaryUpdateResult Lufia2WorldMapRegionSearch(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint8_t edges[4] = {0x01u, 0x03u, 0x02u, 0x04u};
    uint32_t entries;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return FieldLoopHandoff(cpu, 0x869eddu);
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
            return FieldLoopHandoff(cpu, 0x869ee7u);
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
    return FieldLoopResult(0x869f12u);
}
