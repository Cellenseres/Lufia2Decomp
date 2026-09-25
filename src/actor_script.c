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

/* $85:8E98: sixteen queued VRAM DMA uploads on channel 6. */
static void BattleVramQueue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8dccu);
    LoadX16(cpu, 0x005au);                                     /* 8E98 */
    do {
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a8fu, cpu->x));
        if (!cpu->zero) {
            Write16Absolute(memory, cpu, 0x4365u, cpu->y);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a91u, cpu->x));
            Write16Absolute(memory, cpu, 0x4362u, cpu->y);
            LoadA8(cpu, 0x01u);
            StoreAAbsolute8(memory, cpu, 0x4360u, 0);
            LoadA8(cpu, 0x7eu);
            StoreAAbsolute8(memory, cpu, 0x4364u, 0);
            LoadA8(cpu, 0x18u);
            StoreAAbsolute8(memory, cpu, 0x4361u, 0);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a93u, cpu->x));
            Write16Absolute(memory, cpu, 0x2116u, cpu->y);
            LoadA8(cpu, 0x40u);
            StoreAAbsolute8(memory, cpu, 0x420bu, 0);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x1a8fu, cpu->x), 0x00u);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x1a90u, cpu->x), 0x00u);
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 6u));                 /* 8EC9 */
    } while (!cpu->negative);
    SimulateRtsFrame(memory, cpu);
}

/* $85:8ED2: latch HDMA channels from $1AEF, enable $420C. */
static void BattleHdmaChannels(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8e24u);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xdau)));    /* 8ED2 */
    Write8(memory, DirectAddress(cpu, 0xdau), 0x00u);
    {
        const uint32_t d9 = DirectAddress(cpu, 0xd9u);
        const uint8_t value = Read8(memory, d9);

        cpu->zero = (value & A8(cpu)) == 0;                    /* TSB $D9 */
        Write8(memory, d9, (uint8_t)(value | A8(cpu)));
    }
    Or8(cpu, Read8(memory, DirectAddress(cpu, 0xd8u)));
    Or8(cpu, 0x40u);
    And8(cpu, Read8(memory, DirectAddress(cpu, 0xd9u)));
    Write8(memory, DirectAddress(cpu, 0xd8u), A8(cpu));
    if (!cpu->zero) {
        LoadY16(cpu, 0x4300u);
        LoadX16(cpu, 0x1aefu);
        for (;;) {
            const uint32_t d8 = DirectAddress(cpu, 0xd8u);     /* 8EE8 */
            const uint8_t mask = Read8(memory, d8);

            cpu->carry = mask & 1u;
            Write8(memory, d8, (uint8_t)(mask >> 1));
            SetNz8(cpu, (uint8_t)(mask >> 1));
            if (cpu->carry) {
                LoadAAbsolute8(memory, cpu, 0x0000u, cpu->x);  /* 8EF2 */
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->y);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0001u, cpu->x));
                Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0001u, cpu->y),
                    cpu->accumulator);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->x));
                Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0003u, cpu->y),
                    cpu->accumulator);
            } else if (cpu->zero) {
                break;
            } else {
                SetAccumulatorWidth(cpu, 0);
            }
            LoadA16(cpu, cpu->x);                              /* 8F06 */
            cpu->carry = 0;
            Add16Value(cpu, 0x0005u);
            TransferAToX(cpu);
            LoadA16(cpu, cpu->y);
            Add16Value(cpu, 0x0010u);
            TransferAToY(cpu);
            SetAccumulatorWidth(cpu, 1);
        }
    }
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xd9u)));    /* 8F15 */
    StoreAAbsolute8(memory, cpu, 0x420cu, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $85:A8E7: timer 1, battle HDMA wave table at $7E:40CC. */
static void BattleWaveTable(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadAAbsolute8(memory, cpu, 0x1b23u, 0);                   /* A8E7 */
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x1b22u, 0);
        PushIndex(memory, cpu);
        SetIndexWidth(cpu, 1);
        TransferAToX(cpu);
        And8(cpu, 0x03u);
        Write8(memory, DirectAddress(cpu, 0x33u), A8(cpu));
        LoadA8(cpu, 0x04u);
        cpu->carry = 1;
        Sbc8(cpu, Read8(memory, DirectAddress(cpu, 0x33u)));
        Write8(memory, DirectAddress(cpu, 0x33u), A8(cpu));
        AslA8(cpu);
        Adc8(cpu, Read8(memory, DirectAddress(cpu, 0x33u)));
        TransferAToY(cpu);
        TransferXToA(cpu);
        AslA8(cpu);
        cpu->carry = 0;
        Adc8(cpu, 0x08u);
        TransferAToX(cpu);
        PushDataBank(memory, cpu);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859ffau, cpu->x)));
        LoadX8(cpu, 0x00u);
        do {
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x40ccu, cpu->x),
                cpu->accumulator);                             /* A915 */
            LoadY8(cpu, (uint8_t)(cpu->y - 1u));
            if (cpu->zero) {
                LoadY8(cpu, 0x0cu);
                cpu->carry = 0;
                Add16Value(cpu, 0x0004u);
            }
            LoadX8(cpu, (uint8_t)(cpu->x + 1u));               /* A921 */
            LoadX8(cpu, (uint8_t)(cpu->x + 1u));
            Compare8(cpu, (uint8_t)cpu->x, 0x90u);
        } while (!cpu->zero);
        PullDataBank(memory, cpu);                             /* A927 */
        SetAccumulatorWidth(cpu, 1);
        SetIndexWidth(cpu, 0);
        cpu->x = PullIndexValue(memory, cpu);
    } else {
        static const uint16_t fills[3][2] = {
            {0x015fu, 0x0008u}, {0x015bu, 0x0038u}, {0x011fu, 0x0008u}};
        unsigned band;

        PushIndex(memory, cpu);                                /* A933 */
        PushDataBank(memory, cpu);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadX16(cpu, 0x0000u);
        SetAccumulatorWidth(cpu, 0);
        for (band = 0; band < 3u; ++band) {
            LoadA16(cpu, fills[band][0]);
            LoadY16(cpu, fills[band][1]);
            do {
                Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x40ccu, cpu->x),
                    cpu->accumulator);
                IncrementX16(cpu);
                IncrementX16(cpu);
                LoadY16(cpu, (uint16_t)(cpu->y - 1u));
            } while (!cpu->zero);
        }
        SetAccumulatorWidth(cpu, 1);                           /* A968 */
        PullDataBank(memory, cpu);
        cpu->x = PullIndexValue(memory, cpu);
    }
    LoadA8(cpu, 0x01u);                                        /* A92D */
    StoreAAbsolute8(memory, cpu, 0x1b20u, 0);
}

/* $85:8F1B: eight battle timers; 0 = handler left to LLE. */
static uint8_t BattleTimers(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8e78u);
    SetIndexWidth(cpu, 1);                                     /* 8F1B */
    LoadX8(cpu, 0x00u);
    do {
        LoadAAbsolute8(memory, cpu, 0x1b17u, cpu->x);
        if (!cpu->zero) {
            const uint32_t counter =
                AbsoluteIndexedAddress(cpu, 0x1b18u, cpu->x);
            const uint8_t left = (uint8_t)(Read8(memory, counter) - 1u);

            Write8(memory, counter, left);
            SetNz8(cpu, left);
            if (cpu->zero) {
                uint16_t handler;

                LoadAAbsolute8(memory, cpu, 0x1b19u, cpu->x);  /* 8F29 */
                AslA8(cpu);
                TransferAToY(cpu);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x9e05u, cpu->y));
                handler = cpu->accumulator;
                Push8(memory, cpu, 0x8fu);                     /* PEA #$8F3B */
                Push8(memory, cpu, 0x3bu);
                PushAccumulator16(memory, cpu);
                SetAccumulatorWidth(cpu, 1);
                SetIndexWidth(cpu, 0);
                (void)Pull8(memory, cpu);                      /* RTS */
                (void)Pull8(memory, cpu);
                if (handler != 0xa8e6u) {
                    /* Other handlers run on LLE after RTS. */
                    cpu->resume_pc = 0x850000u | (uint16_t)(handler + 1u);
                    return 0;
                }
                BattleWaveTable(memory, cpu);
                SimulateRtsFrame(memory, cpu);                 /* to 8F3C */
                SetIndexWidth(cpu, 1);
            }
        }
        LoadA8(cpu, (uint8_t)cpu->x);                          /* 8F3E */
        cpu->carry = 0;
        Adc8(cpu, 0x08u);
        TransferAToX(cpu);
        Compare8(cpu, (uint8_t)cpu->x, 0x40u);
    } while (!cpu->zero);
    SetIndexWidth(cpu, 0);                                     /* 8F47 */
    SimulateRtsFrame(memory, cpu);
    return 1;
}

/* $85:8DC5: battle NMI work via the $00:0067 vector. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleNmiUploads(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    static const uint16_t scroll_regs[6] = {
        0x210du, 0x210eu, 0x210fu, 0x2110u, 0x2111u, 0x2112u};
    static const uint16_t window_regs[8] = {
        0x2123u, 0x2125u, 0x2127u, 0x2129u, 0x212bu, 0x212du, 0x212fu,
        0x2131u};
    Lufia2ActorPrimaryUpdateResult result;
    unsigned i;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x858e97u;
    result.dispatches = 0;
    /* The NMI handler always enters with M=1. */
    if (!cpu->accumulator_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x858dc5u;
        return result;
    }
    PushDataBank(memory, cpu);                                 /* 8DC5 */
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    SetIndexWidth(cpu, 0);
    BattleVramQueue(memory, cpu);
    LoadA8(cpu, 0xffu);                                        /* 8DCD */
    TestBitsAbsolute8(memory, cpu, 0x15b3u, 0);
    if (!cpu->zero) {
        for (i = 0; i < 12u; ++i) {
            LoadAAbsolute8(memory, cpu, (uint16_t)(0x0594u + i), 0);
            StoreAAbsolute8(memory, cpu, scroll_regs[i >> 1], 0);
        }
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x059au, 0));
        Write16Absolute(memory, cpu, 0x1256u, cpu->x);
    }
    BattleHdmaChannels(memory, cpu);                           /* 8E22 */
    for (i = 0; i < 8u; ++i) {
        LoadX16(cpu, Read16AbsoluteIndexed(
            memory, cpu, (uint16_t)(0x123cu + 2u * i), 0));
        Write16Absolute(memory, cpu, window_regs[i], cpu->x);
    }
    LoadAAbsolute8(memory, cpu, 0x1268u, 0);                   /* 8E55 */
    if (cpu->zero) {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1249u, 0));
        Write16Absolute(memory, cpu, 0x1252u, cpu->x);
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1245u, 0));
        Write16Absolute(memory, cpu, 0x1250u, cpu->x);
    }
    LoadAAbsolute8(memory, cpu, 0x1254u, 0);                   /* 8E66 */
    StoreAAbsolute8(memory, cpu, 0x1255u, 0);
    LoadAAbsolute8(memory, cpu, 0x0583u, 0);
    StoreAAbsolute8(memory, cpu, 0x2100u, 0);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xdbu)));
    if (!cpu->zero && !BattleTimers(memory, cpu)) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc;
        return result;
    }
    LoadY16(cpu, 0x0000u);                                     /* 8E79 */
    LoadAAbsolute8(memory, cpu, 0x12e3u, cpu->y);
    if (!cpu->negative) {
        /* Queued tasks run through JSR ($9E37,x) on LLE. */
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x858e7fu;
        return result;
    }
    PullDataBank(memory, cpu);                                 /* 8E96 */
    return result;
}

/* Y += step with M=0, back to M=1. */
static void BattleNextRecord(Lufia2ActorFrontendCpu *cpu, uint16_t step) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, step);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $81:BD4B, $81:BDCC: OAM strips, five bytes per sprite. */
static void BattleOamStrips(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t mirrored) {
    BattleSetDataBank(memory, cpu, 0x7eu);
    if (mirrored) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BDD1 */
        DecrementA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        AslA8(cpu);
        Adc8(cpu, DirectByte(memory, cpu, 0x06u));
        StoreADirect8(memory, cpu, 0x06u);
    }
    Write8(memory, DirectAddress(cpu, 0x15u), 0x00u);
    TransferDirectToA(cpu);
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x07u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x06u));
    TransferAToY(cpu);
    StoreYDirect16(memory, cpu, 0x17u);
    StoreYDirect16(memory, cpu, 0x1cu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x00u));
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x11u);
    And8(cpu, 0xf0u);
    Adc8(cpu, DirectByte(memory, cpu, 0x11u));
    StoreADirect8(memory, cpu, 0x11u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x05u));
    DecrementA8(cpu);
    StoreADirect8(memory, cpu, 0x16u);
    LoadXDirect16(memory, cpu, 0x08u);
    do {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BD70 */
        StoreADirect8(memory, cpu, 0x13u);
        do {
            static const uint8_t fields[4] = {0x17u, 0x16u, 0x11u, 0x04u};
            unsigned i;

            for (i = 0; i < 4u; ++i) {
                LoadA8(cpu, DirectByte(memory, cpu, fields[i]));
                StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
                IncrementX16(cpu);
            }
            LoadA8(cpu, DirectByte(memory, cpu, 0x18u));
            And8(cpu, 0x03u);
            StoreAAbsolute8(memory, cpu, 0x0000u, cpu->x);
            IncrementX16(cpu);
            IncrementDirect8(memory, cpu, 0x15u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x17u));
            if (mirrored) {
                Subtract16(cpu, 0x0010u);
            } else {
                cpu->carry = 0;
                Add16Value(cpu, 0x0010u);
            }
            Write16Direct(memory, cpu, 0x17u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
            cpu->carry = 0;
            Adc8(cpu, 0x02u);
            BitImmediate8(cpu, 0x0fu);
            if (cpu->zero)
                Adc8(cpu, 0x10u);
            StoreADirect8(memory, cpu, 0x11u);
            DecrementDirect8(memory, cpu, 0x13u);
        } while (!cpu->zero);
        LoadA8(cpu, DirectByte(memory, cpu, 0x16u));           /* BDB3 */
        cpu->carry = 0;
        Adc8(cpu, 0x10u);
        StoreADirect8(memory, cpu, 0x16u);
        LoadYDirect16(memory, cpu, 0x1cu);
        StoreYDirect16(memory, cpu, 0x17u);
        DecrementDirect8(memory, cpu, 0x03u);
    } while (!cpu->zero);
    StoreXDirect16(memory, cpu, 0x08u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x15u));
    PullDataBank(memory, cpu);
}

/* JSL $81:BD47 or $81:BDC8 from bank 85. */
static void BattleCallOamStrips(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t mirrored,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    SimulateJsrFrame(memory, cpu, mirrored ? 0xbdcau : 0xbd49u);
    BattleOamStrips(memory, cpu, mirrored);
    SimulateRtsFrame(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $81:BE58: 16x16 tilemap block, rows of $02 cells. */
static void BattleTileBlock(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x7eu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x00u));               /* BE5D */
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x11u);
    And8(cpu, 0xf0u);
    Adc8(cpu, DirectByte(memory, cpu, 0x11u));
    StoreADirect8(memory, cpu, 0x11u);
    LoadXDirect16(memory, cpu, 0x08u);
    StoreXDirect16(memory, cpu, 0x19u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x03u));
    StoreADirect8(memory, cpu, 0x14u);
    for (;;) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x02u));           /* BE70 */
        StoreADirect8(memory, cpu, 0x13u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, DirectByte(memory, cpu, 0x11u));
        do {
            SetAccumulatorWidth(cpu, 0);                       /* BE79 */
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->x),
                cpu->accumulator);
            TransferAToY(cpu);
            IncrementA16(cpu);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0002u, cpu->x),
                cpu->accumulator);
            LoadA16(cpu, cpu->y);
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0040u, cpu->x),
                cpu->accumulator);
            IncrementA16(cpu);
            Write16Long(memory, AbsoluteIndexedAddress(cpu, 0x0042u, cpu->x),
                cpu->accumulator);
            LoadA16(cpu, cpu->y);
            Add16Value(cpu, 0x0002u);
            cpu->zero = (cpu->accumulator & 0x000fu) == 0;
            if (cpu->zero)
                Add16Value(cpu, 0x0010u);
            SetAccumulatorWidth(cpu, 1);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementX16(cpu);
            IncrementX16(cpu);
            DecrementDirect8(memory, cpu, 0x13u);
        } while (!cpu->zero);
        StoreADirect8(memory, cpu, 0x11u);                     /* BEA5 */
        DecrementDirect8(memory, cpu, 0x14u);
        if (cpu->zero)
            break;
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Direct(memory, cpu, 0x19u));
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        Write16Direct(memory, cpu, 0x19u, cpu->accumulator);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
    }
    PullDataBank(memory, cpu);
}

/* $85:8B4B: $153C sprites from 13-byte records at $139A. */
static void BattleSpriteRecords(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadA8(cpu, 0xffu);                                        /* 8B50 */
    StoreAAbsolute8(memory, cpu, 0x15c7u, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15c8u, 0));
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15cau, 0);
    LoadAAbsolute8(memory, cpu, 0x153cu, 0);
    if (!cpu->zero) {
        StoreADirect8(memory, cpu, 0x0du);
        LoadY16(cpu, 0x0000u);
        do {
            LoadAAbsolute8(memory, cpu, 0x139au, cpu->y);      /* 8B67 */
            if (cpu->negative) {
                PushY(memory, cpu);
                TransferDirectToA(cpu);
                LoadAAbsolute8(memory, cpu, 0x13a4u, cpu->y);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x13a0u, cpu->y));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreADirect8(memory, cpu, 0x05u);
                LoadX16(cpu, 0x0202u);
                StoreXDirect16(memory, cpu, 0x02u);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13a2u, cpu->y));
                cpu->carry = 0;
                Add16Value(cpu,
                    Read16AbsoluteIndexed(memory, cpu, 0x139eu, cpu->y));
                And16(cpu, 0x01ffu);
                Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x15b5u, 0);       /* 8B8D */
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x139cu, cpu->y));
                And8(cpu, 0x07u);
                AslA8(cpu);
                Or8(cpu, AbsoluteByte(memory, cpu, 0x1475u, 0));
                Or8(cpu, AbsoluteByte(memory, cpu, 0x139bu, cpu->y));
                StoreADirect8(memory, cpu, 0x04u);
                LoadAAbsolute8(memory, cpu, 0x139du, cpu->y);
                StoreADirect8(memory, cpu, 0x00u);
                BattleCallOamStrips(memory, cpu, 0, 0x8ba7u);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x15cau, 0));
                StoreAAbsolute8(memory, cpu, 0x15cau, 0);
                cpu->y = PullIndexValue(memory, cpu);
            }
            BattleNextRecord(cpu, 0x000du);                    /* 8BB0 */
            DecrementDirect8(memory, cpu, 0x0du);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);
}

/* $85:8BC0: the single 3x3 sprite at $13CE. */
static void BattleSpriteSingle(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15ccu, 0));
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15cbu, 0);
    LoadAAbsolute8(memory, cpu, 0x13ceu, 0);
    if (cpu->negative) {
        LoadA8(cpu, 0xffu);                                    /* 8BD2 */
        StoreAAbsolute8(memory, cpu, 0x15cbu, 0);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x13d8u, 0);
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x13d4u, 0));
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreADirect8(memory, cpu, 0x05u);
        LoadX16(cpu, 0x0303u);
        StoreXDirect16(memory, cpu, 0x02u);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13d6u, 0));
        cpu->carry = 0;
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13d2u, 0));
        And16(cpu, 0x01ffu);
        Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x15b6u, 0);               /* 8BF7 */
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x13d0u, 0));
        And8(cpu, 0x07u);
        Or8(cpu, AbsoluteByte(memory, cpu, 0x13ceu, 0));
        cpu->carry = 1;
        RolA8(cpu);
        Or8(cpu, AbsoluteByte(memory, cpu, 0x1476u, 0));
        Or8(cpu, AbsoluteByte(memory, cpu, 0x13cfu, 0));
        StoreADirect8(memory, cpu, 0x04u);
        LoadAAbsolute8(memory, cpu, 0x13d1u, 0);
        StoreADirect8(memory, cpu, 0x00u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
        BitImmediate8(cpu, 0x40u);
        if (cpu->zero)
            BattleCallOamStrips(memory, cpu, 0, 0x8c1bu);
        else
            BattleCallOamStrips(memory, cpu, 1, 0x8c21u);
        StoreAAbsolute8(memory, cpu, 0x15ceu, 0);
    }
    PullDataBank(memory, cpu);                                 /* 8C25 */
}

/* $85:8C27: five 1x1 sprites from $1435, same records as $139A. */
static void BattleSpriteMarkers(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadX16(cpu, 0x493du);                                     /* 8C2C */
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15d2u, 0);
    LoadA8(cpu, 0x05u);
    StoreADirect8(memory, cpu, 0x0du);
    LoadY16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0x1435u, cpu->y);          /* 8C3B */
        if (cpu->negative) {
            PushY(memory, cpu);
            TransferDirectToA(cpu);
            LoadAAbsolute8(memory, cpu, 0x13a4u, cpu->y);
            cpu->carry = 0;
            Adc8(cpu, AbsoluteByte(memory, cpu, 0x13a0u, cpu->y));
            StoreADirect8(memory, cpu, 0x05u);
            LoadX16(cpu, 0x0101u);
            StoreXDirect16(memory, cpu, 0x02u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13a2u, cpu->y));
            cpu->carry = 0;
            Add16Value(cpu,
                Read16AbsoluteIndexed(memory, cpu, 0x139eu, cpu->y));
            cpu->carry = 0;
            Add16Value(cpu, 0x0010u);
            And16(cpu, 0x01ffu);
            Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            LoadAAbsolute8(memory, cpu, 0x1478u, 0);           /* 8C64 */
            cpu->carry = 0;
            Adc8(cpu, 0x08u);
            Or8(cpu, AbsoluteByte(memory, cpu, 0x1436u, cpu->y));
            Or8(cpu, 0x01u);
            StoreADirect8(memory, cpu, 0x04u);
            LoadAAbsolute8(memory, cpu, 0x1438u, cpu->y);
            StoreADirect8(memory, cpu, 0x00u);
            BattleCallOamStrips(memory, cpu, 0, 0x8c79u);
            cpu->carry = 0;
            Adc8(cpu, AbsoluteByte(memory, cpu, 0x15d2u, 0));
            StoreAAbsolute8(memory, cpu, 0x15d2u, 0);
            cpu->y = PullIndexValue(memory, cpu);
        }
        BattleNextRecord(cpu, 0x000du);                        /* 8C82 */
        DecrementDirect8(memory, cpu, 0x0du);
    } while (!cpu->zero);
    LoadAAbsolute8(memory, cpu, 0x15d2u, 0);                   /* 8C90 */
    StoreAAbsolute8(memory, cpu, 0x15cfu, 0);
    PullDataBank(memory, cpu);
}

/* $85:8C98: six sprites from 15-byte records at $13DB. */
static void BattleSpriteParty(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    BattleSetDataBank(memory, cpu, 0x00u);
    LoadA8(cpu, 0xffu);                                        /* 8C9D */
    StoreAAbsolute8(memory, cpu, 0x15d3u, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15d4u, 0));
    StoreXDirect16(memory, cpu, 0x08u);
    StoreZeroAbsolute8(memory, cpu, 0x15d6u, 0);
    LoadX16(cpu, 0x0000u);
    StoreXDirect16(memory, cpu, 0x0bu);
    StoreZeroAbsolute8(memory, cpu, 0x1577u, 0);
    LoadAAbsolute8(memory, cpu, 0x154eu, 0);
    if (!cpu->zero) {
        LoadA8(cpu, 0x06u);
        StoreADirect8(memory, cpu, 0x0du);
        LoadY16(cpu, 0x0000u);
        do {
            LoadAAbsolute8(memory, cpu, 0x13dbu, cpu->y);      /* 8CBE */
            if (cpu->negative) {
                uint8_t mirrored;

                PushY(memory, cpu);
                TransferDirectToA(cpu);
                LoadAAbsolute8(memory, cpu, 0x13e5u, cpu->y);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x13e1u, cpu->y));
                LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                StoreADirect8(memory, cpu, 0x05u);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13e7u, cpu->y));
                Write16Direct(memory, cpu, 0x02u, cpu->accumulator);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13e3u, cpu->y));
                cpu->carry = 0;
                Add16Value(cpu,
                    Read16AbsoluteIndexed(memory, cpu, 0x13dfu, cpu->y));
                And16(cpu, 0x01ffu);
                Write16Direct(memory, cpu, 0x06u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x15b7u, 0);       /* 8CE4 */
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x13ddu, cpu->y));
                And8(cpu, 0x07u);
                Or8(cpu, AbsoluteByte(memory, cpu, 0x13dbu, cpu->y));
                cpu->carry = 1;
                RolA8(cpu);
                Or8(cpu, AbsoluteByte(memory, cpu, 0x1477u, 0));
                StoreADirect8(memory, cpu, 0x04u);
                LoadAAbsolute8(memory, cpu, 0x13deu, cpu->y);
                StoreADirect8(memory, cpu, 0x00u);
                LoadA8(cpu, DirectByte(memory, cpu, 0x04u));
                BitImmediate8(cpu, 0x40u);
                mirrored = cpu->zero ? 0u : 1u;
                BattleCallOamStrips(
                    memory, cpu, mirrored, mirrored ? 0x8d0bu : 0x8d05u);
                LoadYDirect16(memory, cpu, 0x0bu);             /* 8D0C */
                StoreAAbsolute8(memory, cpu, 0x157fu, cpu->y);
                IncrementDirect8(memory, cpu, 0x0bu);
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x15d6u, 0));
                StoreAAbsolute8(memory, cpu, 0x15d6u, 0);
                StoreAAbsolute8(memory, cpu, 0x1578u, cpu->y);
                cpu->y = PullIndexValue(memory, cpu);
            }
            BattleNextRecord(cpu, 0x000fu);                    /* 8D1E */
            DecrementDirect8(memory, cpu, 0x0du);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);                                 /* 8D2C */
}

/* $85:8D2E: party tilemap at $7E:2800 instead of sprites. */
static void BattlePartyTilemap(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);                                 /* PHK */
    PullDataBank(memory, cpu);
    StoreZeroAbsolute8(memory, cpu, 0x15d3u, 0);               /* 8D31 */
    LoadX16(cpu, 0x2800u);
    LoadY16(cpu, 0x0200u);
    Write16Absolute(memory, cpu, 0x2181u, cpu->x);
    StoreZeroAbsolute8(memory, cpu, 0x2183u, 0);
    LoadA8(cpu, 0x01u);
    do {
        StoreZeroAbsolute8(memory, cpu, 0x2180u, 0);           /* 8D42 */
        StoreAAbsolute8(memory, cpu, 0x2180u, 0);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    LoadAAbsolute8(memory, cpu, 0x154eu, 0);                   /* 8D4B */
    if (!cpu->zero) {
        LoadA8(cpu, 0x06u);
        StoreADirect8(memory, cpu, 0x05u);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadY16(cpu, 0x0000u);
        do {
            LoadAAbsolute8(memory, cpu, 0x13dbu, cpu->y);      /* 8D5B */
            if (cpu->negative) {
                PushY(memory, cpu);
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x13e7u, cpu->y));
                Write16Direct(memory, cpu, 0x02u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x13e1u, cpu->y);  /* 8D6A */
                ExchangeAccumulatorBytes(cpu);
                LoadAAbsolute8(memory, cpu, 0x13dfu, cpu->y);
                SetAccumulatorWidth(cpu, 0);
                cpu->carry = 0;
                Add16Value(cpu, 0x0100u);
                LsrA16(cpu);
                LsrA16(cpu);
                LsrA16(cpu);
                And16(cpu, 0x1f1fu);
                SetAccumulatorWidth(cpu, 1);
                ExchangeAccumulatorBytes(cpu);                 /* 8D7F */
                Write8(memory, 0x004202u, A8(cpu));
                LoadA8(cpu, 0x40u);
                Write8(memory, 0x004203u, A8(cpu));
                LoadA8(cpu, 0x00u);
                ExchangeAccumulatorBytes(cpu);
                AslA8(cpu);
                SetAccumulatorWidth(cpu, 0);
                cpu->carry = 0;
                Add16Value(cpu, Read16Long(memory, 0x004216u));
                Add16Value(cpu, 0x2800u);
                Write16Direct(memory, cpu, 0x08u, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                LoadAAbsolute8(memory, cpu, 0x13ddu, cpu->y);  /* 8D9C */
                cpu->carry = 0;
                Adc8(cpu, AbsoluteByte(memory, cpu, 0x15b7u, 0));
                And8(cpu, 0x07u);
                AslA8(cpu);
                AslA8(cpu);
                Or8(cpu, 0x23u);
                StoreADirect8(memory, cpu, 0x04u);
                LoadAAbsolute8(memory, cpu, 0x13deu, cpu->y);
                StoreADirect8(memory, cpu, 0x00u);
                SimulateJslFrame(memory, cpu, 0x85u, 0x8db3u); /* $81:BE54 */
                SimulateJsrFrame(memory, cpu, 0xbe56u);
                BattleTileBlock(memory, cpu);
                SimulateRtsFrame(memory, cpu);
                SimulateRtlFrame(memory, cpu);
                cpu->y = PullIndexValue(memory, cpu);
            }
            BattleNextRecord(cpu, 0x000fu);                    /* 8DB5 */
            DecrementDirect8(memory, cpu, 0x05u);
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);                                 /* 8DC3 */
}

/* $85:972E: 15 rows of 16 tile ids from $3710. */
static void BattleTileGrid(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    unsigned row;

    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, 0x3710u);
    for (row = 0; row < 15u; ++row) {
        unsigned column;

        LoadX16(cpu, (uint16_t)(0x2816u + 0x40u * row));
        SimulateJsrFrame(memory, cpu, (uint16_t)(0x9738u + 6u * row));
        for (column = 0; column < 16u; ++column) {
            Write16Long(memory, AbsoluteIndexedAddress(
                cpu, (uint16_t)(2u * column), cpu->x), cpu->accumulator);
            IncrementA16(cpu);
        }
        SimulateRtsFrame(memory, cpu);
    }
    SetAccumulatorWidth(cpu, 1);
}

/* $85:8A2F body; returns the RTL taken. */
static uint16_t BattleSprites(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadAAbsolute8(memory, cpu, 0x15abu, 0);
    DecrementA8(cpu);
    if (cpu->zero) {
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a6fu);
        BattleSpriteRecords(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a73u);
        BattleSpriteSingle(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a77u);
        BattleSpriteMarkers(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        LoadAAbsolute8(memory, cpu, 0x11deu, 0);               /* 8A78 */
        if (cpu->zero) {
            SimulateJslFrame(memory, cpu, 0x85u, 0x8a80u);
            BattlePartyTilemap(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            return 0x8a95u;
        }
        LoadAAbsolute8(memory, cpu, 0x125fu, 0);               /* 8A83 */
        if (!cpu->zero)
            return 0x8a95u;
        StoreZeroAbsolute8(memory, cpu, 0x15d3u, 0);
        BattleSetDataBank(memory, cpu, 0x7eu);
        SimulateJslFrame(memory, cpu, 0x85u, 0x8a93u);
        BattleTileGrid(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        PullDataBank(memory, cpu);
        return 0x8a95u;
    }
    DecrementA8(cpu);
    if (!cpu->zero)
        return 0x8a38u;
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aa1u);             /* 8A9E */
    BattleSpriteRecords(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aa5u);
    BattleSpriteSingle(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aa9u);
    BattleSpriteMarkers(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0x8aadu);
    BattleSpriteParty(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return 0x8aaeu;
}

/* $85:91A1: refresh the five $147A state bytes. */
static void BattleSlotStates(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);                                 /* PHK */
    PullDataBank(memory, cpu);
    LoadY16(cpu, 0x0000u);
    LoadX16(cpu, cpu->y);                                      /* TYX */
    do {
        SetAccumulatorWidth(cpu, 0);                           /* 91A8 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a64u, cpu->x));
        if (!cpu->zero) {
            PushIndex(memory, cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadAAbsolute8(memory, cpu, 0x147au, cpu->y);
            ExchangeAccumulatorBytes(cpu);
            LoadAAbsolute8(memory, cpu, 0x000fu, cpu->x);
            StoreAAbsolute8(memory, cpu, 0x147au, cpu->y);
            if (cpu->zero) {
                TransferDirectToA(cpu);                        /* 91CC */
                StoreAAbsolute8(memory, cpu, 0x1479u, cpu->y);
            } else {
                ExchangeAccumulatorBytes(cpu);
                if (cpu->zero) {
                    LoadA8(cpu, 0xffu);
                    StoreAAbsolute8(memory, cpu, 0x147bu, cpu->y);
                    StoreAAbsolute8(memory, cpu, 0x1479u, cpu->y);
                }
            }
            cpu->x = PullIndexValue(memory, cpu);
        }
        IncrementX16(cpu);                                     /* 91D1 */
        IncrementX16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x0014u);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
}

static Lufia2ActorPrimaryUpdateResult BattleFrameEntry(
    Lufia2ActorFrontendCpu *cpu, uint32_t entry, uint32_t exit) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = exit;
    result.dispatches = 0;
    /* Only the M=1 X=0 entry is native. */
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = entry;
    }
    return result;
}

/* $85:8A2F: battle sprites, JSL entry. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleSprites(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result =
        BattleFrameEntry(cpu, 0x858a2fu, 0x858a38u);

    if (result.flow == LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED)
        result.pc = 0x850000u | BattleSprites(memory, cpu);
    return result;
}

/* $85:ECF0: per-frame battle upkeep from the $81:8877 loop. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleFrameUpkeep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const Lufia2ActorPrimaryUpdateResult result =
        BattleFrameEntry(cpu, 0x85ecf0u, 0x85ed50u);
    static const uint16_t lists[2][2] = {{0x0a64u, 8u}, {0x0a6eu, 10u}};
    unsigned list;

    if (result.flow != LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED)
        return result;
    SimulateJslFrame(memory, cpu, 0x85u, 0xecf3u);
    BattleSlotStates(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x85u, 0xecf7u);
    (void)BattleSprites(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0xffu);                                        /* ECF8 */
    Write8(memory, 0x0012f3u, A8(cpu));
    SimulateJslFrame(memory, cpu, 0x85u, 0xed01u);             /* $85:9265 */
    BattleSetDataBank(memory, cpu, 0x7fu);
    LoadX16(cpu, 0x02fbu);
    do {
        StoreZeroAbsolute8(memory, cpu, 0xf44eu, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    StoreZeroAbsolute8(memory, cpu, 0x1b8bu, 0);               /* ED02 */
    LoadX16(cpu, 0x0023u);
    do {
        StoreZeroAbsolute8(memory, cpu, 0x1b8cu, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    LoadAAbsolute8(memory, cpu, 0x11e8u, 0);                   /* ED0E */
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    if (!cpu->zero)
        StoreAAbsolute8(memory, cpu, 0x11e8u, 0);
    StoreZeroAbsolute8(memory, cpu, 0x11eau, 0);
    for (list = 0; list < 2u; ++list) {
        LoadY16(cpu, lists[list][1]);                          /* ED1A */
        do {
            LoadX16(cpu, Read16AbsoluteIndexed(
                memory, cpu, lists[list][0], cpu->y));
            if (!cpu->zero) {
                LoadAAbsolute8(memory, cpu, 0x0010u, cpu->x);
                And8(cpu, 0xfeu);
                StoreAAbsolute8(memory, cpu, 0x0010u, cpu->x);
            }
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->negative);
    }
    LoadX16(cpu, 0x0041u);                                     /* ED42 */
    do {
        LoadAAbsolute8(memory, cpu, 0x14e6u, cpu->x);
        if (!cpu->zero) {
            const uint32_t timer =
                AbsoluteIndexedAddress(cpu, 0x14e6u, cpu->x);
            const uint8_t left = (uint8_t)(Read8(memory, timer) - 1u);

            Write8(memory, timer, left);
            SetNz8(cpu, left);
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    return result;
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

/* $85:ECDB: Y = first free slot in the $1A8F VRAM queue. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleVramQueueSlot(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;

    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_RETURNED;
    result.pc = 0x85ecefu;
    result.dispatches = 0;
    /* X=1 decodes LDY #imm as two bytes. */
    if (cpu->index_is_8_bit) {
        result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
        result.pc = cpu->resume_pc = 0x85ecdbu;
        return result;
    }
    LoadY16(cpu, 0x0000u);                                     /* ECDB */
    for (;;) {
        LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a8fu, cpu->y));
        if (cpu->zero)
            return result;
        LoadY16(cpu, (uint16_t)(cpu->y + 6u));
        Compare16(cpu, cpu->y, 0x0060u);
        if (cpu->zero)
            break;
    }
    /* Queue full: BRK #$6B on LLE. */
    result.flow = LUFIA2_ACTOR_PRIMARY_UPDATE_BOUNDARY;
    result.pc = cpu->resume_pc = 0x85eceeu;
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

/* $85:C168: battler byte A to its variable base in X. */
static void BattleScriptSlot(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreAAbsolute8(memory, cpu, 0x09fbu, 0);                  /* C168 */
    BitImmediate8(cpu, 0x3fu);
    if (!cpu->zero) {
        And8(cpu, 0x80u);
        if (!cpu->zero)
            LoadA8(cpu, 0x05u);
        StoreAAbsolute8(memory, cpu, 0x09fau, 0);
        LoadA8(cpu, 0xffu);
        do {
            const uint32_t bits = AbsoluteIndexedAddress(cpu, 0x09fbu, 0);
            const uint8_t value = Read8(memory, bits);

            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));              /* C17A */
            cpu->carry = value & 1u;
            Write8(memory, bits, (uint8_t)(value >> 1));
            SetNz8(cpu, (uint8_t)(value >> 1));
        } while (!cpu->carry);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x09fau, 0)));
    }
    SetAccumulatorWidth(cpu, 0);                               /* C184 */
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a13u, 0));
    And16(cpu, 0x00ffu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
        cpu->zero ? 0x0a80u : 0x0a64u, cpu->x));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $85:C4DE: variable bases $C1 and $BE. */
static void BattleScriptBases(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0xb462u);
    LoadA8(cpu, Read8(memory, 0x7ff450u));                     /* C4DE */
    BattleScriptSlot(memory, cpu, 0xc4e4u);
    Write16Direct(memory, cpu, 0xc1u, cpu->x);
    LoadA8(cpu, Read8(memory, 0x7ff44eu));
    BattleScriptSlot(memory, cpu, 0xc4edu);
    Write16Direct(memory, cpu, 0xbeu, cpu->x);
    SimulateRtsFrame(memory, cpu);
}

/* INC $BB, 16-bit. */
static void BattleScriptAdvance(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint16_t pointer = (uint16_t)(Read16Direct(memory, cpu, 0xbbu) + 1u);

    Write16Direct(memory, cpu, 0xbbu, pointer);
    SetNz16(cpu, pointer);
}

/* $85:BFBF: next script byte into A; flags kept. */
static void BattleScriptByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFBF */
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0xbbu)));
    SetAccumulatorWidth(cpu, 0);
    BattleScriptAdvance(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:BFCA: next script word into X. */
static void BattleScriptWord(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFCA */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0xbbu)));
    BattleScriptAdvance(memory, cpu);
    BattleScriptAdvance(memory, cpu);
    TransferAToX(cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* Selector bit 7: local slot at base $C1 or $BE. */
static uint16_t BattleScriptLocal(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a52u, 0));
    And16(cpu, 0x00ffu);
    {
        const uint8_t base = cpu->zero ? 0xbeu : 0xc1u;

        LoadA16(cpu, (uint16_t)(
            Read8(memory, (uint16_t)(cpu->stack + 1u)) |
            (Read8(memory, (uint16_t)(cpu->stack + 2u)) << 8)));
        And16(cpu, 0x007fu);
        AslA16(cpu);
        Add16Value(cpu, Read16Direct(memory, cpu, base));
    }
    return cpu->accumulator;
}

/* $85:BFED: read variable A (bit 7 local) into X. */
static void BattleScriptRead(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFED */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    cpu->zero = (cpu->accumulator & 0x0080u) == 0;
    if (cpu->zero) {
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7ff40eu, cpu->x)));
    } else {
        BattleScriptLocal(memory, cpu);                        /* C001 */
        TransferAToX(cpu);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0015u, cpu->x));
    }
    TransferAToX(cpu);                                         /* C01F */
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:C023: write X to variable A. */
static void BattleScriptWrite(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* C023 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushY(memory, cpu);
    PushAccumulator16(memory, cpu);
    cpu->zero = (cpu->accumulator & 0x0080u) == 0;
    if (cpu->zero) {
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        LoadY16(cpu, cpu->accumulator);
        LoadA16(cpu, cpu->x);
        LoadX16(cpu, cpu->y);
        Write16Long(memory, LongIndexedAddress(0x7ff40eu, cpu->x),
            cpu->accumulator);
    } else {
        BattleScriptLocal(memory, cpu);                        /* C03A */
        LoadY16(cpu, cpu->accumulator);
        LoadA16(cpu, cpu->x);
        LoadX16(cpu, cpu->y);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x0015u, cpu->x),
            (uint8_t)cpu->accumulator);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x0016u, cpu->x),
            (uint8_t)(cpu->accumulator >> 8));
    }
    TransferAToX(cpu);                                         /* C05A */
    PullAccumulator16(memory, cpu);
    cpu->y = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:BFD8: next word, bit 15 = variable, into X. */
static void BattleScriptValue(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* BFD8 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, DirectLongPointer(memory, cpu, 0xbbu)));
    if (cpu->negative)
        BattleScriptRead(memory, cpu, 0xbfe2u);
    else
        TransferAToX(cpu);
    BattleScriptAdvance(memory, cpu);                          /* BFE6 */
    BattleScriptAdvance(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:B551: $BB = $0A42 + next word. */
static void BattleScriptJump(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* B551 */
    BattleScriptWord(memory, cpu, 0xb555u);
    LoadA16(cpu, cpu->x);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a42u, 0));
    Write16Direct(memory, cpu, 0xbbu, cpu->accumulator);
}

/* Operand fetch shared by the compare and math opcodes. */
static void BattleScriptOperands(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t opcode) {
    BattleScriptByte(memory, cpu, (uint16_t)(opcode + 2u));
    BattleScriptRead(memory, cpu, (uint16_t)(opcode + 5u));
    Write16Direct(memory, cpu, 0x54u, cpu->x);
    BattleScriptValue(memory, cpu, (uint16_t)(opcode + 10u));
}

/* Signed $54 - X, 16-bit, overflow kept. */
static void BattleScriptCompare(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
    Write16Direct(memory, cpu, 0x54u, cpu->x);
    cpu->carry = 1;
    Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
}

/* Store binary result X into the destination variable. */
static void BattleScriptStoreBinary(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t value,
    uint16_t return_address) {
    LoadA16(cpu, value);
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    BattleScriptWrite(memory, cpu, return_address);
}

/* Unary opcodes: destination byte, source variable, result X. */
static void BattleScriptUnary(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t opcode,
    uint8_t kind) {
    BattleScriptByte(memory, cpu, (uint16_t)(opcode + 2u));
    PushAccumulator8(memory, cpu);
    BattleScriptRead(memory, cpu, (uint16_t)(opcode + 6u));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->x);
    if (kind == 0) {                                           /* abs */
        if (cpu->negative) {
            LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
            LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
        }
        TransferAToX(cpu);
    } else if (kind == 1) {                                    /* negate */
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffffu));
        LoadA16(cpu, (uint16_t)(cpu->accumulator + 1u));
        TransferAToX(cpu);
    } else if (!cpu->zero) {                                   /* sign */
        LoadX16(cpu, cpu->negative ? 0xffffu : 0x0001u);
    }
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, Pull8(memory, cpu));
    BattleScriptWrite(memory, cpu,
        (uint16_t)(opcode + (kind == 0 ? 22u : kind == 1 ? 20u : 27u)));
}

/* INC $66, 16-bit. */
static void BattleIncrement66(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    const uint16_t value = (uint16_t)(Read16Direct(memory, cpu, 0x66u) + 1u);

    Write16Direct(memory, cpu, 0x66u, value);
    SetNz16(cpu, value);
}

/* $85:DCA3: $63-$66 = $54 * $56, 16x16 via $4202. */
static void BattleMultiply(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DCA3 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x56u));
    StoreAAbsolute8(memory, cpu, 0x4202u, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, 0x4203u, 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    SetAccumulatorWidth(cpu, 0);
    StoreWordAbsolute(memory, cpu, 0x4202u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x63u, cpu->x);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreWordAbsolute(memory, cpu, 0x4202u, cpu->accumulator);
    Write16Direct(memory, cpu, 0x65u, cpu->x);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x55u));
    cpu->carry = 0;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    StoreWordAbsolute(memory, cpu, 0x4202u, cpu->x);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x64u));
    if (cpu->carry) {
        BattleIncrement66(memory, cpu);
        cpu->carry = 0;
    }
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4216u, 0));
    if (cpu->carry)
        BattleIncrement66(memory, cpu);
    Write16Direct(memory, cpu, 0x64u, cpu->accumulator);       /* DCE6 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $85:DC6F: $5D-$5F /= $54, 24/8 via $4204. */
static void BattleDivide(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    PushDataBank(memory, cpu);                                 /* DC6F */
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x5eu));
    StoreWordAbsolute(memory, cpu, 0x4204u, cpu->x);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, 0x4206u, 0);
    PushIndex(memory, cpu);
    cpu->x = PullIndexValue(memory, cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
    ExchangeAccumulatorBytes(cpu);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4214u, 0));
    LoadAAbsolute8(memory, cpu, 0x4216u, 0);
    ExchangeAccumulatorBytes(cpu);
    LoadY16(cpu, cpu->accumulator);
    StoreWordAbsolute(memory, cpu, 0x4204u, cpu->y);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    StoreAAbsolute8(memory, cpu, 0x4206u, 0);
    LoadA8(cpu, (uint8_t)cpu->x);
    ExchangeAccumulatorBytes(cpu);
    StoreADirect8(memory, cpu, 0x5fu);
    LoadA8(cpu, 0x00u);
    SetAccumulatorWidth(cpu, 0);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x4214u, 0));
    Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    PullDataBank(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $85:DCEA: A times a random 16-bit fraction. */
static void BattleRandomScale(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    static const struct {
        uint8_t limit;
        uint16_t call;
    } rolls[3] = {{0x80u, 0xdcf7u}, {0x80u, 0xdcffu}, {0x04u, 0xdd07u}};
    unsigned i;

    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DCEA */
    PushDataBank(memory, cpu);
    Push8(memory, cpu, 0x85u);
    PullDataBank(memory, cpu);
    if (cpu->accumulator_is_8_bit)
        StoreADirect8(memory, cpu, 0x54u);
    else
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    for (i = 0; i < 3u; ++i) {
        LoadA8(cpu, rolls[i].limit);
        SimulateJslFrame(memory, cpu, 0x85u, rolls[i].call);
        Lufia2RandomScale(memory, cpu);
        SimulateRtlFrame(memory, cpu);
        if (i < 2u)
            StoreADirect8(memory, cpu, i == 0 ? 0x56u : 0x57u);
    }
    for (i = 0; i < 2u; ++i) {
        const uint32_t address = DirectAddress(cpu, (uint8_t)(0x56u + i));
        const uint8_t value = Read8(memory, address);
        const uint8_t in = (uint8_t)(A8(cpu) & 1u);

        LsrA8(cpu);                                            /* DD08 */
        cpu->carry = value >> 7;
        Write8(memory, address, (uint8_t)((value << 1) | in));
        SetNz8(cpu, (uint8_t)((value << 1) | in));
    }
    BattleMultiply(memory, cpu, 0xdd11u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x65u));
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $85:DB6D: 32/16 shift-subtract divide of $63-$66 by $58. */
static void BattleLongDivide(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    unsigned i;

    SimulateJslFrame(memory, cpu, 0x85u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* DB6D */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x66u));
    And16(cpu, 0x00ffu);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x64u));
    Write16Direct(memory, cpu, 0x65u, cpu->x);
    LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
    Write16Direct(memory, cpu, 0x64u, cpu->x);
    for (i = 0; i < 16u; ++i) {
        const uint16_t low = Read16Direct(memory, cpu, 0x63u);
        const uint16_t high = Read16Direct(memory, cpu, 0x65u);
        const uint16_t a = cpu->accumulator;

        Write16Direct(memory, cpu, 0x63u, (uint16_t)(low << 1));
        Write16Direct(memory, cpu, 0x65u,
            (uint16_t)((high << 1) | (low >> 15)));
        LoadA16(cpu, (uint16_t)((a << 1) | (high >> 15)));
        cpu->carry = a >> 15;
        if (!cpu->carry) {
            Compare16(cpu, cpu->accumulator,
                Read16Direct(memory, cpu, 0x58u));
            if (!cpu->carry)
                continue;
        }
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x58u));
        Write16Direct(memory, cpu, 0x63u,
            (uint16_t)(Read16Direct(memory, cpu, 0x63u) + 1u));
    }
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* DC6D */
    SimulateRtlFrame(memory, cpu);
}

/* $85:C099: X = $85:9E47 word for index A. */
static void BattleStatOffset(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* C099 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadX16(cpu, Read16Long(memory, LongIndexedAddress(0x859e47u, cpu->x)));
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:C05F: X = stat A of battler ($BE); bytes at $0E/$BC. */
static void BattleStat(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* C05F */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushY(memory, cpu);
    BattleStatOffset(memory, cpu, 0xc066u);
    LoadY16(cpu, cpu->x);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
        Read16Direct(memory, cpu, 0xbeu), cpu->y));
    if (cpu->y == 0x00bcu || cpu->y == 0x000eu)
        And16(cpu, 0x00ffu);
    else
        Compare16(cpu, cpu->y, 0x000eu);
    TransferAToX(cpu);
    cpu->y = PullIndexValue(memory, cpu);
    PullAccumulator16(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $85:C117: mask of battlers without status bit 2. */
static void BattleActiveMask(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    const uint8_t enemies = (uint8_t)(A8(cpu) & 0x80u);

    SimulateJsrFrame(memory, cpu, return_address);
    StoreZeroAbsolute8(memory, cpu, 0x09fau, 0);               /* C117 */
    cpu->zero = enemies == 0;
    SetAccumulatorWidth(cpu, 0);
    LoadX16(cpu, enemies ? 0x000au : 0x0008u);
    do {
        const uint32_t mask = AbsoluteIndexedAddress(cpu, 0x09fau, 0);
        uint16_t bits;

        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu,
            enemies ? 0x0a6eu : 0x0a64u, cpu->x));
        cpu->carry = 0;
        if (!cpu->zero) {
            LoadY16(cpu, cpu->accumulator);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x000fu, cpu->y));
            cpu->zero = (cpu->accumulator & 0x0004u) == 0;
            cpu->carry = cpu->zero;
        }
        bits = (uint16_t)(Read8(memory, mask) |
            (Read8(memory, AbsoluteIndexedAddress(cpu, 0x09fbu, 0)) << 8));
        {
            const uint8_t out = (uint8_t)(bits >> 15);

            bits = (uint16_t)((bits << 1) | cpu->carry);
            cpu->carry = out;
        }
        Write8(memory, mask, (uint8_t)bits);
        Write8(memory, AbsoluteIndexedAddress(cpu, 0x09fbu, 0),
            (uint8_t)(bits >> 8));
        SetNz16(cpu, bits);
        cpu->x = (uint16_t)(cpu->x - 2u);
        SetNz16(cpu, cpu->x);
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, 0x09fau, 0);
    if (enemies)
        Or8(cpu, 0x80u);
    SimulateRtsFrame(memory, cpu);
}

/* $85:B452: battle script VM; other opcodes run on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2BattleScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    Lufia2ActorPrimaryUpdateResult result;
    unsigned opcodes;

    PushDataBank(memory, cpu);                                 /* B452 */
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(memory, cpu, 0x0a60u, 0);
    for (opcodes = 0;; ++opcodes) {
        uint16_t handler;

        SetAccumulatorWidth(cpu, 1);                           /* B45E */
        BattleScriptBases(memory, cpu);
        TransferDirectToA(cpu);
        LoadA8(cpu, Read8(memory, DirectLongPointer(memory, cpu, 0xbbu)));
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        TransferAToX(cpu);
        BattleScriptAdvance(memory, cpu);
        SetAccumulatorWidth(cpu, 1);
        Push8(memory, cpu, 0x85u);
        PullDataBank(memory, cpu);
        handler = (uint16_t)(
            Read8(memory, 0x850000u | (uint16_t)(0xb483u + cpu->x)) |
            (Read8(memory, 0x850000u | (uint16_t)(0xb484u + cpu->x)) << 8));
        switch (opcodes < 4096u ? handler : 0u) {
        case 0xb473u:                                          /* end */
        case 0xb476u:                                          /* end, $FF */
            if (handler == 0xb473u)
                TransferDirectToA(cpu);
            else
                LoadA8(cpu, 0xffu);
            StoreAAbsolute8(memory, cpu, 0x0a5bu, 0);          /* B478 */
            SetAccumulatorWidth(cpu, 0);
            SetIndexWidth(cpu, 0);
            cpu->y = PullIndexValue(memory, cpu);
            cpu->x = PullIndexValue(memory, cpu);
            PullAccumulator16(memory, cpu);
            UnpackStatus(cpu, Pull8(memory, cpu));
            PullDataBank(memory, cpu);
            return FieldLoopResult(0x85b482u);
        case 0xb551u:                                          /* jump */
            BattleScriptJump(memory, cpu);
            break;
        case 0xb582u:                                          /* random */
            BattleScriptByte(memory, cpu, 0xb584u);
            StoreADirect8(memory, cpu, 0x54u);
            LoadA8(cpu, 0xffu);
            SimulateJslFrame(memory, cpu, 0x85u, 0xb58cu);
            Lufia2RandomScale(memory, cpu);
            SimulateRtlFrame(memory, cpu);
            cpu->carry = 1;
            Sbc8(cpu, DirectByte(memory, cpu, 0x54u));
            if (!cpu->carry)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb594u);
            break;
        case 0xb598u:                                          /* equal */
        case 0xb5adu:                                          /* not equal */
            BattleScriptOperands(memory, cpu, handler);
            Compare16(cpu, cpu->x, Read16Direct(memory, cpu, 0x54u));
            if (cpu->zero == (handler == 0xb598u))
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, (uint16_t)(handler + 17u));
            break;
        case 0xb60au:                                          /* >= */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (cpu->negative == cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb629u);
            break;
        case 0xb62du:                                          /* <= */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (cpu->zero || cpu->negative != cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb64eu);
            break;
        case 0xb670u:                                          /* set */
            BattleScriptByte(memory, cpu, 0xb672u);
            BattleScriptValue(memory, cpu, 0xb675u);
            BattleScriptWrite(memory, cpu, 0xb678u);
            break;
        case 0xb67cu:                                          /* add */
        case 0xb698u:                                          /* subtract */
            BattleScriptOperands(memory, cpu, handler);
            PushAccumulator8(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            if (handler == 0xb67cu) {
                LoadA16(cpu, cpu->x);
                cpu->carry = 0;
                Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
            } else {
                LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
                Write16Direct(memory, cpu, 0x54u, cpu->x);
                cpu->carry = 1;
                Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
            }
            BattleScriptStoreBinary(memory, cpu, cpu->accumulator,
                handler == 0xb67cu ? 0xb694u : 0xb6b3u);
            break;
        case 0xb849u:                                          /* and */
        case 0xb864u:                                          /* or */
        case 0xb87fu: {                                        /* xor */
            uint16_t value;

            BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
            PushAccumulator8(memory, cpu);
            BattleScriptRead(memory, cpu, (uint16_t)(handler + 6u));
            Write16Direct(memory, cpu, 0x54u, cpu->x);
            BattleScriptValue(memory, cpu, (uint16_t)(handler + 11u));
            SetAccumulatorWidth(cpu, 0);
            value = Read16Direct(memory, cpu, 0x54u);
            value = handler == 0xb849u ? (uint16_t)(cpu->x & value)
                : handler == 0xb864u ? (uint16_t)(cpu->x | value)
                : (uint16_t)(cpu->x ^ value);
            BattleScriptStoreBinary(memory, cpu, value,
                (uint16_t)(handler + 0x17u));
            break;
        }
        case 0xb89au:
            BattleScriptUnary(memory, cpu, handler, 0);
            break;
        case 0xb8b4u:
            BattleScriptUnary(memory, cpu, handler, 1);
            break;
        case 0xb8ccu:
            BattleScriptUnary(memory, cpu, handler, 2);
            break;
        case 0xb8ebu:                                          /* leader id */
            LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11e8u, 0));
            BattleScriptByte(memory, cpu, 0xb8f0u);
            BattleScriptWrite(memory, cpu, 0xb8f3u);
            break;
        case 0xb91fu:                                          /* $7F:F45C */
            BattleScriptWord(memory, cpu, 0xb921u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff45cu, cpu->accumulator);
            break;
        case 0xb92cu:                                          /* $7F:F45E */
            BattleScriptByte(memory, cpu, 0xb92eu);
            Write8(memory, 0x7ff45eu, A8(cpu));
            BattleScriptByte(memory, cpu, 0xb935u);
            Write8(memory, 0x7ff460u, A8(cpu));
            break;
        case 0xb560u:                                          /* jump if F42E */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory, 0x7ff42eu));
            if (!cpu->zero)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb56au);
            break;
        case 0xb5c2u:                                          /* > */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (!cpu->zero && cpu->negative == cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb5e3u);
            break;
        case 0xb5e7u:                                          /* < */
            BattleScriptOperands(memory, cpu, handler);
            BattleScriptCompare(memory, cpu);
            if (cpu->negative != cpu->overflow)
                BattleScriptJump(memory, cpu);
            else
                BattleScriptWord(memory, cpu, 0xb606u);
            break;
        case 0xb93du:                                          /* move setup */
            BattleScriptWord(memory, cpu, 0xb93fu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff45cu, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff45au, cpu->accumulator);
            LoadA16(cpu, 0x0005u);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            BattleScriptValue(memory, cpu, 0xb957u);
            LoadA16(cpu, (uint16_t)(0u - cpu->x));
            Write16Long(memory, 0x7ff462u, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            BattleScriptByte(memory, cpu, 0xb96au);
            break;
        case 0xb9b8u:                                          /* step setup */
            BattleScriptWord(memory, cpu, 0xb9bau);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, (uint16_t)(0u - cpu->x));
            Write16Long(memory, 0x7ff462u, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            Write16Long(memory, 0x7ff45au, cpu->accumulator);
            LoadA16(cpu, 0x0005u);
            break;
        case 0xb9dau:                                          /* F44E word+byte */
            BattleScriptByte(memory, cpu, 0xb9dcu);
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f04u, cpu->x)));
            And16(cpu, 0x00ffu);
            PushAccumulator16(memory, cpu);
            BattleScriptValue(memory, cpu, 0xb9edu);
            LoadA16(cpu, cpu->x);
            cpu->x = PullIndexValue(memory, cpu);
            Write16Long(memory, LongIndexedAddress(0x7ff44eu, cpu->x),
                cpu->accumulator);
            IncrementX16(cpu);
            IncrementX16(cpu);
            SetAccumulatorWidth(cpu, 1);
            BattleScriptByte(memory, cpu, 0xb9fau);
            Write8(memory, LongIndexedAddress(0x7ff44eu, cpu->x), A8(cpu));
            break;
        case 0xba02u:                                          /* F44E byte, clear */
            BattleScriptByte(memory, cpu, 0xba04u);
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f04u, cpu->x)));
            And16(cpu, 0x00ffu);
            cpu->carry = 0;
            Add16Value(cpu, 0x0004u);
            TransferAToX(cpu);
            BattleScriptByte(memory, cpu, 0xba19u);
            Write16Long(memory, LongIndexedAddress(0x7ff44eu, cpu->x),
                cpu->accumulator);
            cpu->x = (uint16_t)(cpu->x - 2u);
            SetNz16(cpu, cpu->x);
            TransferDirectToA(cpu);
            Write16Long(memory, LongIndexedAddress(0x7ff44eu, cpu->x),
                cpu->accumulator);
            break;
        case 0xba28u:                                          /* F44E byte */
        case 0xba47u:
            BattleScriptByte(memory, cpu, (uint16_t)(handler + 2u));
            SetAccumulatorWidth(cpu, 0);
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x859f0fu, cpu->x)));
            And16(cpu, 0x00ffu);
            if (handler == 0xba28u)
                LoadA16(cpu, (uint16_t)(cpu->accumulator + 2u));
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            BattleScriptByte(memory, cpu,
                handler == 0xba28u ? 0xba3fu : 0xba5cu);
            Write8(memory, LongIndexedAddress(0x7ff44eu, cpu->x), A8(cpu));
            break;
        case 0xba64u:                                          /* action 1 */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, 0x0001u);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            LoadA16(cpu, 0x0000u);
            Write16Long(memory, 0x7ff456u, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            StoreAImmediate8(memory, cpu, 0xffu, 0x1262u);
            StoreZeroAbsolute8(memory, cpu, 0x1269u, 0);
            break;
        case 0xba81u:                                          /* action 4 */
        case 0xba8au:                                          /* action 6 */
        case 0xbb46u:                                          /* action 13 */
        case 0xbd6eu:                                          /* action 12 */
            LoadA8(cpu, handler == 0xba81u ? 0x04u : handler == 0xba8au
                ? 0x06u : handler == 0xbb46u ? 0x0du : 0x0cu);
            Write8(memory, 0x7ff454u, A8(cpu));
            break;
        case 0xba93u:                                          /* action 3 */
            LoadA8(cpu, 0x03u);
            Write8(memory, 0x7ff454u, A8(cpu));
            BattleScriptWord(memory, cpu, 0xba9bu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff456u, cpu->accumulator);
            break;
        case 0xbd5eu:                                          /* action 11 */
            LoadA8(cpu, 0x0bu);
            Write8(memory, 0x7ff454u, A8(cpu));
            BattleScriptByte(memory, cpu, 0xbd66u);
            Write8(memory, 0x7ff456u, A8(cpu));
            break;
        case 0xbb4fu:                                          /* party flag count */
            LoadA8(cpu, Read8(memory, 0x7ff450u));
            if (cpu->negative) {
                LoadX16(cpu, 0x0000u);
            } else {
                Write8(memory, DirectAddress(cpu, 0x54u), 0x00u);
                LoadY16(cpu, 0x0008u);
                do {
                    LoadX16(cpu, Read16AbsoluteIndexed(
                        memory, cpu, 0x0a64u, cpu->y));        /* BB5F */
                    if (!cpu->zero) {
                        LoadAAbsolute8(memory, cpu, 0x000fu, cpu->x);
                        BitImmediate8(cpu, 0x04u);
                        if (!cpu->zero)
                            IncrementDirect8(memory, cpu, 0x54u);
                    }
                    cpu->y = (uint16_t)(cpu->y - 2u);
                    SetNz16(cpu, cpu->y);
                } while (!cpu->negative);
                TransferDirectToA(cpu);
                LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
                TransferAToX(cpu);
            }
            BattleScriptByte(memory, cpu, 0xbb77u);
            BattleScriptWrite(memory, cpu, 0xbb7au);
            break;
        case 0xbb7eu:                                          /* side leader */
            TransferDirectToA(cpu);
            LoadA8(cpu, Read8(memory, 0x7ff44eu));
            if (cpu->negative) {
                LoadAAbsolute8(memory, cpu, 0x15feu, 0);
            } else {
                LoadAAbsolute8(memory, cpu, 0x0a13u, 0);
                LoadAAbsolute8(memory, cpu,
                    cpu->zero ? 0x0a7au : 0x153cu, 0);
            }
            TransferAToX(cpu);
            BattleScriptByte(memory, cpu, 0xbb9au);
            BattleScriptWrite(memory, cpu, 0xbb9du);
            break;
        case 0xbccfu:                                          /* $0A62 byte */
            BattleScriptByte(memory, cpu, 0xbcd1u);
            StoreAAbsolute8(memory, cpu, 0x0a62u, 0);
            break;
        case 0xbcd8u:                                          /* $0A62 if F42E */
            BattleScriptByte(memory, cpu, 0xbcdau);
            StoreADirect8(memory, cpu, 0x54u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory, 0x7ff42eu));
            if (!cpu->zero) {
                LoadA16(cpu, Read16Direct(memory, cpu, 0x54u));
                Write16Absolute(memory, cpu, 0x0a62u, cpu->accumulator);
            }
            break;
        case 0xbe22u:                                          /* timer $1264 */
            BattleScriptValue(memory, cpu, 0xbe24u);
            Write16Absolute(memory, cpu, 0x1264u, cpu->x);
            break;
        case 0xbe68u:                                          /* F450 mask */
            LoadAAbsolute8(memory, cpu, 0x0a5du, 0);
            if (!cpu->negative) {
                LoadA8(cpu, (uint8_t)(Read8(memory, 0x7ff450u) | 0xefu));
                LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory,
                    AbsoluteIndexedAddress(cpu, 0x0a5du, 0))));
                Write8(memory, 0x7ff450u, A8(cpu));
            }
            break;
        case 0xb6b7u:                                          /* multiply */
            BattleScriptOperands(memory, cpu, handler);
            Write16Direct(memory, cpu, 0x56u, cpu->x);
            PushAccumulator8(memory, cpu);
            BattleMultiply(memory, cpu, 0xb6c8u);
            LoadA8(cpu, Pull8(memory, cpu));
            LoadX16(cpu, Read16Direct(memory, cpu, 0x63u));
            BattleScriptWrite(memory, cpu, 0xb6ceu);
            break;
        case 0xb6d2u: {                                        /* divide */
            uint8_t negative;

            BattleScriptByte(memory, cpu, 0xb6d4u);
            BattleScriptRead(memory, cpu, 0xb6d7u);
            PushAccumulator8(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            negative = cpu->negative;
            if (negative)
                LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
            Write16Direct(memory, cpu, 0x5du, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            BattleScriptValue(memory, cpu, negative ? 0xb701u : 0xb6e4u);
            LoadA8(cpu, (uint8_t)cpu->x);
            StoreADirect8(memory, cpu, 0x54u);
            Write8(memory, DirectAddress(cpu, 0x5fu), 0x00u);
            BattleDivide(memory, cpu, negative ? 0xb70au : 0xb6edu);
            if (negative) {
                SetAccumulatorWidth(cpu, 0);
                LoadA16(cpu, (uint16_t)(0u - Read16Direct(memory, cpu, 0x5du)));
                TransferAToX(cpu);
                SetAccumulatorWidth(cpu, 1);
            } else {
                LoadX16(cpu, Read16Direct(memory, cpu, 0x5du));
            }
            LoadA8(cpu, Pull8(memory, cpu));
            BattleScriptWrite(memory, cpu, negative ? 0xb719u : 0xb6f3u);
            break;
        }
        case 0xb73bu:                                          /* random scale */
            BattleScriptByte(memory, cpu, 0xb73du);
            BattleScriptRead(memory, cpu, 0xb740u);
            PushAccumulator8(memory, cpu);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            BattleRandomScale(memory, cpu, 0xb748u);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Pull8(memory, cpu));
            BattleScriptWrite(memory, cpu, 0xb74fu);
            break;
        case 0xb753u:                                          /* long divide */
            BattleScriptByte(memory, cpu, 0xb755u);
            BattleScriptRead(memory, cpu, 0xb758u);
            PushAccumulator8(memory, cpu);
            Write16Direct(memory, cpu, 0x58u, cpu->x);
            BattleScriptByte(memory, cpu, 0xb75eu);
            BattleScriptRead(memory, cpu, 0xb761u);
            Write16Direct(memory, cpu, 0x65u, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            Write16Direct(memory, cpu, 0x63u, 0x0000u);
            BattleLongDivide(memory, cpu, 0xb76bu);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x64u));
            And16(cpu, 0x00ffu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadA8(cpu, Pull8(memory, cpu));
            BattleScriptWrite(memory, cpu, 0xb777u);
            break;
        case 0xb96eu:                                          /* move by stats */
            BattleScriptWord(memory, cpu, 0xb970u);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            Write16Long(memory, 0x7ff45cu, cpu->accumulator);
            LoadA16(cpu, 0x01dcu);
            Write16Long(memory, 0x7ff45au, cpu->accumulator);
            LoadA16(cpu, 0x000au);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            BattleScriptValue(memory, cpu, 0xb988u);
            Write16Direct(memory, cpu, 0xcau, cpu->x);
            LoadA16(cpu, 0x0008u);
            BattleStat(memory, cpu, 0xb990u);
            LoadA16(cpu, cpu->x);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
            Write16Direct(memory, cpu, 0xcau, cpu->accumulator);
            LoadA16(cpu, 0x0011u);
            BattleStat(memory, cpu, 0xb99cu);
            LoadA16(cpu, cpu->x);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
            Write16Direct(memory, cpu, 0xcau, cpu->accumulator);
            LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
            Write16Long(memory, 0x7ff462u, cpu->accumulator);
            LoadA16(cpu, 0x0020u);
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            BattleScriptByte(memory, cpu, 0xb9b4u);
            break;
        case 0xbcedu:                                          /* jump back */
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, 0x0001u);
            Write16Long(memory, 0x7ff454u, cpu->accumulator);
            TransferDirectToA(cpu);
            Write16Long(memory, 0x7ff456u, cpu->accumulator);
            LoadA16(cpu, Read16Long(memory, 0x7ff462u));
            if (cpu->zero) {
                LoadA16(cpu, 0x0004u);
                BattleStat(memory, cpu, 0xbd06u);
                LoadA16(cpu, cpu->x);
                cpu->carry = 0;
                Add16Value(cpu, Read16Long(memory, 0x7ff4cau));
                PushAccumulator16(memory, cpu);
                LoadA16(cpu, 0x000du);
                BattleStat(memory, cpu, 0xbd13u);
                LoadA16(cpu, cpu->x);
                cpu->carry = 0;
                Add16Value(cpu, (uint16_t)(
                    Read8(memory, (uint16_t)(cpu->stack + 1u)) |
                    (Read8(memory, (uint16_t)(cpu->stack + 2u)) << 8)));
                LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
                Write16Long(memory, 0x7ff462u, cpu->accumulator);
                PullAccumulator16(memory, cpu);
            }
            LoadA16(cpu, 0x0040u);                             /* BD21 */
            Write16Long(memory, 0x7ff464u, cpu->accumulator);
            break;
        case 0xbd2cu:                                          /* extend jump */
            LoadA8(cpu, 0x08u);
            BattleStat(memory, cpu, 0xbd30u);
            Write16Direct(memory, cpu, 0xcau, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory, 0x7ff462u));
            if (!cpu->zero) {
                if (cpu->negative) {
                    LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
                    cpu->carry = 0;
                    Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
                    LoadA16(cpu, (uint16_t)(0u - cpu->accumulator));
                } else {
                    cpu->carry = 0;
                    Add16Value(cpu, Read16Direct(memory, cpu, 0xcau));
                }
                Write16Long(memory, 0x7ff462u, cpu->accumulator);
            }
            break;
        case 0xbe56u:                                          /* living mask */
            LoadA8(cpu, Read8(memory, 0x7ff450u));
            BattleActiveMask(memory, cpu, 0xbe5cu);
            LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory, 0x7ff450u)));
            Write8(memory, 0x7ff450u, A8(cpu));
            break;
        case 0xbd77u:                                          /* call */
            BattleScriptWord(memory, cpu, 0xbd79u);
            Write16Direct(memory, cpu, 0xcau, cpu->x);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, cpu->x);
            AslA16(cpu);
            TransferAToX(cpu);
            SetAccumulatorWidth(cpu, 1);
            LoadY16(cpu, Read16Direct(memory, cpu, 0xbbu));
            Write16Absolute(memory, cpu, 0x0a48u, cpu->y);
            LoadA8(cpu, DirectByte(memory, cpu, 0xbdu));
            StoreAAbsolute8(memory, cpu, 0x0a4au, 0);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a42u, 0));
            Write16Absolute(memory, cpu, 0x0a45u, cpu->y);
            LoadAAbsolute8(memory, cpu, 0x0a44u, 0);
            StoreAAbsolute8(memory, cpu, 0x0a47u, 0);
            LoadA8(cpu, 0x96u);
            StoreADirect8(memory, cpu, 0xbdu);
            StoreAAbsolute8(memory, cpu, 0x0a44u, 0);
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16Long(memory,
                LongIndexedAddress(0x96faddu, cpu->x)));
            cpu->carry = 0;
            Add16Value(cpu, 0xfaddu);
            Write16Absolute(memory, cpu, 0x0a42u, cpu->accumulator);
            Write16Direct(memory, cpu, 0xbbu, cpu->accumulator);
            break;
        case 0xbdb2u:                                          /* return */
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a48u, 0));
            Write16Direct(memory, cpu, 0xbbu, cpu->y);
            LoadAAbsolute8(memory, cpu, 0x0a4au, 0);
            StoreADirect8(memory, cpu, 0xbdu);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a45u, 0));
            Write16Absolute(memory, cpu, 0x0a42u, cpu->y);
            LoadAAbsolute8(memory, cpu, 0x0a47u, 0);
            StoreAAbsolute8(memory, cpu, 0x0a44u, 0);
            break;
        default:                                               /* B470 */
            result = FieldLoopHandoff(cpu, 0x85b470u);
            result.dispatches = opcodes;
            return result;
        }
    }
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

/* $80:C0B7: next text byte from DB:Y; Y past $FFFF moves the bank. */
static void TextNextByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);              /* C0B7 */
    IncrementY16(cpu);
    if (!cpu->negative) {
        PushAccumulator8(memory, cpu);                         /* C0BD */
        Push8(memory, cpu, PackStatus(cpu));
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x09b9u, 0);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, 0x09b9u, 0);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        UnpackStatus(cpu, Pull8(memory, cpu));
        LoadA8(cpu, Pull8(memory, cpu));
        LoadY16(cpu, 0x8000u);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $80:C7C2: glyph $09AF into $7E:[$09B1], attribute from $09AD. */
static void TextDrawGlyph(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJslFrame(memory, cpu, 0x80u, 0xbd3bu);
    Push8(memory, cpu, PackStatus(cpu));                       /* C7C2 */
    PushDataBank(memory, cpu);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, 0x09adu, 0);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80c815u, cpu->x)));
    StoreADirect8(memory, cpu, 0x57u);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09afu, 0));
    Compare16(cpu, cpu->accumulator, 0x0010u);
    if (cpu->zero)
        LoadA16(cpu, 0x0020u);
    Subtract16(cpu, 0x0020u);                                  /* C7DC */
    if (cpu->carry) {
        unsigned row;

        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        AslA16(cpu);
        cpu->carry = 0;
        TransferAToX(cpu);
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09b1u, 0));
        Write16Absolute(memory, cpu, 0x09b5u, cpu->y);
        SetAccumulatorWidth(cpu, 1);
        LoadA8(cpu, 0x7eu);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadA8(cpu, 0x10u);
        StoreADirect8(memory, cpu, 0x58u);
        for (row = 0; row < 16u; ++row) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x9af970u, cpu->x)));
            StoreAAbsolute8(memory, cpu, 0x0000u, cpu->y);     /* C7FC */
            LoadA8(cpu, DirectByte(memory, cpu, 0x57u));
            StoreAAbsolute8(memory, cpu, 0x0001u, cpu->y);
            IncrementX16(cpu);
            IncrementY16(cpu);
            IncrementY16(cpu);
            DecrementDirect8(memory, cpu, 0x58u);
        }
        SetAccumulatorWidth(cpu, 0);                           /* C80B */
        LoadA16(cpu, cpu->y);
        Write16Long(memory, 0x0009b1u, cpu->accumulator);
    }
    PullDataBank(memory, cpu);                                 /* C812 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $80:BD38: draw the glyph and queue its 32-byte VRAM upload. */
static void TextGlyphUpload(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TextDrawGlyph(memory, cpu);                                /* BD38 */
    StoreAImmediate8(memory, cpu, 0x01u, 0x4300u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09b5u, 0));
    Write16Absolute(memory, cpu, 0x4302u, cpu->accumulator);
    Subtract16(cpu, 0xd000u);
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x1800u);
    Write16Absolute(memory, cpu, 0x0079u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    StoreAImmediate8(memory, cpu, 0x7eu, 0x4304u);
    LoadX16(cpu, 0x0020u);
    Write16Absolute(memory, cpu, 0x4305u, cpu->x);
    StoreAImmediate8(memory, cpu, 0x18u, 0x4301u);
    LoadA8(cpu, 0x41u);
    StoreADirect8(memory, cpu, 0x75u);
    SimulateRtsFrame(memory, cpu);
}

/* $80:9DB0: PLP, PLB, RTL. */
static Lufia2ActorPrimaryUpdateResult TextEngineExit(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result) {
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 9DB0 */
    PullDataBank(memory, cpu);
    result.pc = 0x809db2u;
    return result;
}

/* $80:C0EC: previous text byte; Y below $8000 moves the bank back. */
static void TextPrevByte(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    cpu->y = (uint16_t)(cpu->y - 1u);                          /* C0EC */
    SetNz16(cpu, cpu->y);
    if (!cpu->negative) {
        PushAccumulator8(memory, cpu);                         /* C0EF */
        Push8(memory, cpu, PackStatus(cpu));
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x09b9u, 0);
        DecrementA8(cpu);
        StoreAAbsolute8(memory, cpu, 0x09b9u, 0);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        UnpackStatus(cpu, Pull8(memory, cpu));
        LoadA8(cpu, Pull8(memory, cpu));
        LoadY16(cpu, 0xffffu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $84:8328: clear the window buffer $7E:3000-37FF and $099C bit 0. */
void Lufia2TextWindowClear(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJslFrame(memory, cpu, 0x80u, return_address);
    Push8(memory, cpu, PackStatus(cpu));                       /* 8328 */
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, 0x07f8u);
    cpu->carry = 1;
    do {
        unsigned i;

        TransferAToX(cpu);                                     /* 8334 */
        for (i = 0; i < 8u; i += 2u)
            Write16Absolute(
                memory, cpu, (uint16_t)(0x3000u + cpu->x + i), 0);
        Add16Value(cpu, (uint16_t)~0x0008u);
    } while (!cpu->negative);
    SetAccumulatorWidth(cpu, 1);                               /* 8346 */
    LoadAAbsolute8(memory, cpu, 0x099cu, 0);
    And8(cpu, 0xfeu);
    StoreAAbsolute8(memory, cpu, 0x099cu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x059cu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x059du, 0);
    StoreAImmediate8(memory, cpu, 0xfcu, 0x059eu);
    StoreAImmediate8(memory, cpu, 0xffu, 0x059fu);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtlFrame(memory, cpu);
}

/* $80:C1FD: close an open text window. */
static void TextCloseWindow(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x099cu, 0);                   /* C1FD */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x09a7u, 0);
        BitImmediate8(cpu, 0x02u);
        if (cpu->zero) {
            StoreZeroAbsolute8(memory, cpu, 0x125du, 0);
            StoreZeroAbsolute8(memory, cpu, 0x125eu, 0);
            Lufia2TextWindowClear(memory, cpu, 0xc214u);
            LoadA8(cpu, 0x08u);                                /* C215 */
            StoreADirect8(memory, cpu, 0x74u);
        }
    }
    SimulateRtsFrame(memory, cpu);
}

/* $80:9E54: call sub-script $09B0:$09AF from the $8E:EA00 table. */
static void TextSubScript(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    StoreAAbsolute8(memory, cpu, 0x09b0u, 0);                  /* 9E54 */
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, 0x09afu, 0);
    Write16Absolute(memory, cpu, 0x1252u, cpu->y);
    LoadAAbsolute8(memory, cpu, 0x09b9u, 0);
    StoreAAbsolute8(memory, cpu, 0x1254u, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09afu, 0));
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8eea00u, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0xea00u);
    Write16Absolute(memory, cpu, 0x09b7u, cpu->accumulator);
    LoadY16(cpu, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x8eu);
    StoreAAbsolute8(memory, cpu, 0x09b9u, 0);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* $80:9DDB: next text line, $1250 += $400. */
static void TextNewLine(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 0);                               /* 9DDB */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1250u, 0));
    cpu->carry = 0;
    Add16Value(cpu, 0x0400u);
    Write16Absolute(memory, cpu, 0x1250u, cpu->accumulator);
    Write16Absolute(memory, cpu, 0x09b1u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x10u);
    TestBitsAbsolute8(memory, cpu, 0x099cu, 1);
    TransferDirectToA(cpu);
    Write8(memory, 0x7fd0c0u, A8(cpu));
    StoreZeroAbsolute8(memory, cpu, 0x09b3u, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $80:BE30: flag A to byte $56 and mask $57; X 8-bit. */
static void TextFlagBit(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 1);                               /* BE30 */
    SetIndexWidth(cpu, 1);
    StoreADirect8(memory, cpu, 0x54u);
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    StoreADirect8(memory, cpu, 0x56u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x54u));
    And8(cpu, 0x07u);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80be45u, cpu->x)));
    StoreADirect8(memory, cpu, 0x57u);
    SimulateRtsFrame(memory, cpu);
}

/* $80:C0D0: next two text bytes as a word in A. */
static void TextNextWord(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TextNextByte(memory, cpu, 0xc0d2u);                        /* C0D0 */
    PushAccumulator8(memory, cpu);
    TextNextByte(memory, cpu, 0xc0d6u);
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $80:A3C6: goto script base $099E/$09A0 + next word. */
static void TextGoto(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    TextNextWord(memory, cpu, 0xa3c8u);                        /* A3C6 */
    SetAccumulatorWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09a0u, 0));
    Write16Absolute(memory, cpu, 0x09b9u, cpu->accumulator);
    PullAccumulator16(memory, cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x099eu, 0));
    SimulateJsrFrame(memory, cpu, 0xa3d9u);
    LoadA16(cpu, cpu->accumulator);                            /* C102 */
    if (cpu->negative) {
        LoadY16(cpu, cpu->accumulator);
    } else {
        Push8(memory, cpu, PackStatus(cpu));                   /* C109 */
        cpu->carry = 0;
        Add16Value(cpu, 0x8000u);
        LoadY16(cpu, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x09b9u, 0);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, 0x09b9u, 0);
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        UnpackStatus(cpu, Pull8(memory, cpu));
    }
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 1);                               /* A3DA */
}

/* Opcodes that store their argument byte. */
static const struct {
    uint16_t handler;
    uint32_t address;
} kTextByteStores[11] = {
    {0xb7e6u, 0x1255u},                            /* $47 */
    {0xb837u, 0x1265u},                            /* $49 prompt timer */
    {0xb840u, 0x09adu},                            /* $4A glyph attribute */
    {0xb900u, 0x1260u},                            /* $52 typing sound */
    {0xbd6cu, 0x09dfu},                            /* $7C */
    {0xbd75u, 0x09eeu},                            /* $7D */
    {0xbd7eu, 0x09e0u},                            /* $7E */
    {0xbd87u, 0x09e1u},                            /* $7F */
    {0xbd90u, 0x09e2u},                            /* $80 */
    {0xb696u, 0x0b52u},                            /* $CB print delay */
    {0xb2a7u, 0x7ff8a0u}};                         /* $74 */

/* $80:BF12/$80:BF43: gold $0A8A-$0A8C +/- A, capped/undone. */
static void TextGold(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint8_t add,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    if (add) {
        cpu->carry = 0;                                        /* BF15 */
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a8au, 0));
        Write16Absolute(memory, cpu, 0x0a8au, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x0a8cu, 0);
        Adc8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, 0x0a8cu, 0);
        Compare8(cpu, A8(cpu), 0x98u);
        if (cpu->carry) {
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a8au, 0));
            Compare16(cpu, cpu->accumulator, 0x967fu);
            if (cpu->carry) {
                LoadA16(cpu, 0x967fu);                         /* 9,999,999 */
                Write16Absolute(memory, cpu, 0x0a8au, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                StoreAImmediate8(memory, cpu, 0x98u, 0x0a8cu);
            }
        }
    } else {
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);   /* BF46 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a8au, 0));
        cpu->carry = 1;
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
        Write16Absolute(memory, cpu, 0x0a8au, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x0a8cu, 0);
        Sbc8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, 0x0a8cu, 0);
        if (!cpu->carry) {
            uint8_t high;

            SetAccumulatorWidth(cpu, 0);                       /* undo */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0a8au, 0));
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
            Write16Absolute(memory, cpu, 0x0a8au, cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            high = (uint8_t)(Read8(memory,
                AbsoluteIndexedAddress(cpu, 0x0a8cu, 0)) + 1u);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x0a8cu, 0), high);
            SetNz8(cpu, high);
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $80:AF77: COLDATA fade setup, rate by $4204 division. */
static void TextColorFade(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreAAbsolute8(memory, cpu, 0x2131u, 0);                  /* AF77 */
    StoreAImmediate8(memory, cpu, 0xe0u, 0x2132u);
    TextNextByte(memory, cpu, 0xaf81u);
    StoreAAbsolute8(memory, cpu, 0x1286u, 0);
    StoreAAbsolute8(memory, cpu, 0x2132u, 0);
    TextNextByte(memory, cpu, 0xaf8au);
    StoreAAbsolute8(memory, cpu, 0x1287u, 0);
    LoadAAbsolute8(memory, cpu, 0x1286u, 0);
    And8(cpu, 0x1fu);
    Compare8(cpu, A8(cpu),
        Read8(memory, AbsoluteIndexedAddress(cpu, 0x1287u, 0)));
    if (!cpu->carry) {
        LoadAAbsolute8(memory, cpu, 0x1287u, 0);
        And8(cpu, 0x1fu);
    }
    AslA8(cpu);                                                /* AF9D */
    AslA8(cpu);
    AslA8(cpu);
    StoreZeroAbsolute8(memory, cpu, 0x4204u, 0);
    StoreAAbsolute8(memory, cpu, 0x4205u, 0);
    TextNextByte(memory, cpu, 0xafa8u);
    StoreAAbsolute8(memory, cpu, 0x4206u, 0);
    StoreAImmediate8(memory, cpu, 0x00u, 0x2130u);
    LoadA8(cpu, 0x80u);
    TestBitsAbsolute8(memory, cpu, 0x1261u, 1);
    StoreZeroAbsolute8(memory, cpu, 0x1288u, 0);
    LoadAAbsolute8(memory, cpu, 0x4215u, 0);
    StoreAAbsolute8(memory, cpu, 0x1289u, 0);
    SimulateRtsFrame(memory, cpu);
}

enum {
    TEXT_OPCODE_NEXT,
    TEXT_OPCODE_RELOAD,
    TEXT_OPCODE_EXIT,
    TEXT_OPCODE_HANDOFF
};

/* Script opcodes behind JMP ($CA14,x); others hand off. */
static unsigned TextScriptOpcode(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    uint16_t handler,
    uint32_t *handoff) {
    unsigned i;

    for (i = 0; i < 11u; ++i) {
        if (handler != kTextByteStores[i].handler)
            continue;
        TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
        if (kTextByteStores[i].address > 0xffffu)
            Write8(memory, kTextByteStores[i].address, A8(cpu));
        else
            StoreAAbsolute8(memory, cpu,
                (uint16_t)kTextByteStores[i].address, 0);
        return TEXT_OPCODE_NEXT;
    }
    switch (handler) {
    case 0xa80fu:                                  /* $33 wait for actor */
        LoadAAbsolute8(memory, cpu, 0x1269u, 0);
        if (cpu->negative) {
            *handoff = 0x80a834u;
            return TEXT_OPCODE_HANDOFF;
        }
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x1269u, 0);
        TransferAToX(cpu);
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        And8(cpu, 0x88u);
        if (!cpu->zero) {
            TextPrevByte(memory, cpu, 0xa822u);
            return TEXT_OPCODE_EXIT;
        }
        LoadA8(cpu, 0xffu);                                    /* A826 */
        StoreAAbsolute8(memory, cpu, 0x1269u, 0);
        TextNextByte(memory, cpu, 0xa82du);
        TextNextByte(memory, cpu, 0xa830u);
        return TEXT_OPCODE_NEXT;
    case 0xb2ebu:                                  /* $37 wait frames */
        LoadA8(cpu, 0x20u);
        TestBitsAbsolute8(memory, cpu, 0x099bu, 1);
        if (cpu->zero)
            Write8(memory, DirectAddress(cpu, 0x42u), 0x00u);
        LoadA8(cpu, DirectByte(memory, cpu, 0x42u));           /* B2F4 */
        Compare8(cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
        if (!cpu->carry) {
            TextPrevByte(memory, cpu, 0xb2fdu);
            return TEXT_OPCODE_EXIT;
        }
        TextNextByte(memory, cpu, 0xb303u);                    /* B301 */
        LoadA8(cpu, 0x20u);
        TestBitsAbsolute8(memory, cpu, 0x099bu, 0);
        return TEXT_OPCODE_NEXT;
    case 0xb397u:                                  /* $3C wait for party */
        LoadAAbsolute8(memory, cpu, 0x1269u, 0);
        if (cpu->negative) {
            *handoff = 0x80b3dfu;
            return TEXT_OPCODE_HANDOFF;
        }
        LoadAAbsolute8(memory, cpu, 0x0622u, 0);
        for (i = 1; i < 5u; ++i)
            Or8(cpu, Read8(memory,
                AbsoluteIndexedAddress(cpu, (uint16_t)(0x0622u + i), 0)));
        BitImmediate8(cpu, 0x08u);
        if (!cpu->zero) {
            TextPrevByte(memory, cpu, 0xb400u);                /* B3FE */
            return TEXT_OPCODE_EXIT;
        }
        LoadAAbsolute8(memory, cpu, 0x09a7u, 0);               /* B3B2 */
        BitImmediate8(cpu, 0x01u);
        if (!cpu->zero) {
            LoadX16(cpu, 0x0004u);
            do {
                LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
                Or8(cpu, 0x04u);
                StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
                cpu->x = (uint16_t)(cpu->x - 1u);
                SetNz16(cpu, cpu->x);
            } while (!cpu->zero);
        } else {
            LoadX16(cpu, 0x0004u);                             /* B3C9 */
            LoadA8(cpu, 0xffu);
            StoreAAbsolute8(memory, cpu, 0x1269u, 0);
            do {
                StoreAAbsolute8(memory, cpu, 0x09a1u, cpu->x);
                cpu->x = (uint16_t)(cpu->x - 1u);
                SetNz16(cpu, cpu->x);
            } while (!cpu->negative);
        }
        LoadA8(cpu, 0xffu);                                    /* B3D7 */
        StoreAAbsolute8(memory, cpu, 0x1269u, 0);
        return TEXT_OPCODE_NEXT;
    case 0x9d4cu:                                  /* $00/$42 end */
        LoadAAbsolute8(memory, cpu, 0x1254u, 0);
        if (cpu->zero) {
            *handoff = 0x809d69u;
            return TEXT_OPCODE_HANDOFF;
        }
        StoreAAbsolute8(memory, cpu, 0x09b9u, 0);      /* back to caller */
        LoadA8(cpu, 0x10u);
        TestBitsAbsolute8(memory, cpu, 0x099bu, 0);
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1252u, 0));
        Write16Absolute(memory, cpu, 0x09b7u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        StoreZeroAbsolute8(memory, cpu, 0x1254u, 0);
        return TEXT_OPCODE_RELOAD;
    case 0xbc3du:                                  /* $68 skip branch */
        LoadAAbsolute8(memory, cpu, 0x05b3u, 0);
        BitImmediate8(cpu, 0x10u);
        if (cpu->zero) {
            *handoff = 0x80bc44u;
            return TEXT_OPCODE_HANDOFF;
        }
        TextNextByte(memory, cpu, 0xbc4eu);
        TextNextByte(memory, cpu, 0xbc51u);
        return TEXT_OPCODE_NEXT;
    case 0x9dd5u:                                  /* $03 new line */
        TextNewLine(memory, cpu, 0x9dd7u);
        return TEXT_OPCODE_NEXT;
    case 0x9e45u:                                  /* $05/$06 sub-script */
    case 0x9e4eu:
        TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, handler == 0x9e45u ? 0x00u : 0x01u);
        TextSubScript(memory, cpu);
        return TEXT_OPCODE_NEXT;
    case 0x9ef0u:                                  /* $0C-$0F ?!,. new line */
    case 0x9ef4u:
    case 0x9ef8u:
    case 0x9efcu:
        LoadA8(cpu, handler == 0x9ef0u ? 0x3fu : handler == 0x9ef4u ? 0x21u
            : handler == 0x9ef8u ? 0x2cu : 0x2eu);
        StoreAAbsolute8(memory, cpu, 0x09afu, 0);
        StoreZeroAbsolute8(memory, cpu, 0x09b0u, 0);
        Write16Absolute(memory, cpu, 0x09b7u, cpu->y);
        TextGlyphUpload(memory, cpu, 0x9f09u);
        TextNewLine(memory, cpu, 0x9f0cu);
        return TEXT_OPCODE_EXIT;
    case 0xa288u:                                  /* $15 goto if flag */
        TextNextByte(memory, cpu, 0xa28au);
        SimulateJsrFrame(memory, cpu, 0xa28du);
        PushY(memory, cpu);                                    /* BE1E */
        Push8(memory, cpu, PackStatus(cpu));
        TextFlagBit(memory, cpu, 0xbe22u);
        cpu->x = DirectByte(memory, cpu, 0x56u);
        SetNz8(cpu, (uint8_t)cpu->x);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x00077eu, cpu->x)));
        And8(cpu, DirectByte(memory, cpu, 0x57u));
        UnpackStatus(cpu, Pull8(memory, cpu));
        cpu->y = PullIndexValue(memory, cpu);
        LoadA8(cpu, A8(cpu));
        SimulateRtsFrame(memory, cpu);
        if (!cpu->zero) {
            TextGoto(memory, cpu);
            return TEXT_OPCODE_NEXT;
        }
        TextNextByte(memory, cpu, 0xa295u);                    /* A293 */
        TextNextByte(memory, cpu, 0xa298u);
        return TEXT_OPCODE_NEXT;
    case 0xa392u:                                  /* $1A set flag */
    case 0xa3acu:                                  /* $1B clear flag */
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        PushY(memory, cpu);
        TextFlagBit(memory, cpu, (uint16_t)(handler + 6u));
        cpu->x = DirectByte(memory, cpu, 0x56u);
        SetNz8(cpu, (uint8_t)cpu->x);
        if (handler == 0xa392u) {
            LoadAAbsolute8(memory, cpu, 0x077eu, cpu->x);
            Or8(cpu, DirectByte(memory, cpu, 0x57u));
        } else {
            LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
            LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory,
                AbsoluteIndexedAddress(cpu, 0x077eu, cpu->x))));
        }
        StoreAAbsolute8(memory, cpu, 0x077eu, cpu->x);
        SetIndexWidth(cpu, 0);
        cpu->y = PullIndexValue(memory, cpu);
        TextNextByte(memory, cpu, (uint16_t)(handler + 0x16u));
        return TEXT_OPCODE_NEXT;
    case 0xa3c6u:                                  /* $1C goto */
        TextGoto(memory, cpu);
        return TEXT_OPCODE_NEXT;
    case 0xa3dfu:                                  /* $1D byte into $079E */
        TransferDirectToA(cpu);
        TextNextByte(memory, cpu, 0xa3e2u);
        TransferAToX(cpu);
        TextNextByte(memory, cpu, 0xa3e6u);
        StoreAAbsolute8(memory, cpu, 0x079eu, cpu->x);
        return TEXT_OPCODE_NEXT;
    case 0xb484u:                                  /* $3E forced blank */
    case 0xb48cu:                                  /* $3F full brightness */
        StoreAImmediate8(memory, cpu,
            handler == 0xb484u ? 0x80u : 0x0fu, 0x0583u);
        return TEXT_OPCODE_NEXT;
    case 0xb8edu:                                  /* $4F/$5F skip a byte */
    case 0xbc05u:
        TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
        return TEXT_OPCODE_NEXT;
    case 0xa4d4u:                                  /* $27 skip two */
    case 0xa5bcu:                                  /* $2A skip three */
        TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
        TextNextByte(memory, cpu, (uint16_t)(handler + 5u));
        if (handler == 0xa5bcu)
            TextNextByte(memory, cpu, (uint16_t)(handler + 8u));
        return TEXT_OPCODE_NEXT;
    case 0xb8f3u:                                  /* $50 typing sound off */
        StoreZeroAbsolute8(memory, cpu, 0x1260u, 0);
        return TEXT_OPCODE_NEXT;
    case 0xb117u:                                  /* $C1 two bytes */
        TextNextByte(memory, cpu, 0xb119u);
        Write8(memory, 0x7fd4f3u, A8(cpu));
        TextNextByte(memory, cpu, 0xb120u);
        Write8(memory, 0x7fd4f4u, A8(cpu));
        return TEXT_OPCODE_NEXT;
    case 0xba1cu:                                  /* $5A start shake */
        LoadA8(cpu, 0x04u);
        TestBitsAbsolute8(memory, cpu, 0x1261u, 1);
        for (i = 0; i < 3u; ++i) {
            TextNextByte(memory, cpu, (uint16_t)(0xba23u + 7u * i));
            Write8(memory, 0x7fd07eu + i, A8(cpu));
        }
        return TEXT_OPCODE_NEXT;
    case 0xb962u:                                  /* $8A stop shake */
        TransferDirectToA(cpu);
        for (i = 0; i < 4u; ++i)
            Write8(memory, 0x7fd081u + i, A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x1261u, 0);
        And8(cpu, 0xfbu);
        StoreAAbsolute8(memory, cpu, 0x1261u, 0);
        return TEXT_OPCODE_NEXT;
    case 0xb955u:                                  /* $C5 stop color fades */
        LoadA8(cpu, 0x80u);
        TestBitsAbsolute8(memory, cpu, 0x1261u, 0);
        LoadA8(cpu, 0x01u);
        TestBitsAbsolute8(memory, cpu, 0x1262u, 0);
        return TEXT_OPCODE_NEXT;
    case 0xadd1u:                                  /* $B5 */
        StoreZeroAbsolute8(memory, cpu, 0x089du, 0);
        StoreAImmediate8(memory, cpu, 0xffu, 0x085du);
        return TEXT_OPCODE_NEXT;
    case 0xb98fu:                                  /* $57 wait for effects */
        LoadAAbsolute8(memory, cpu, 0x1261u, 0);
        if (!cpu->zero) {
            TextPrevByte(memory, cpu, 0xb999u);
            return TEXT_OPCODE_EXIT;
        }
        return TEXT_OPCODE_NEXT;
    case 0xb89du:                                  /* $71 wait $7F:D0FC frames */
        LoadA8(cpu, (uint8_t)(Read8(memory, 0x7fd0fcu) - 1u));
        Write8(memory, 0x7fd0fcu, A8(cpu));
        if (!cpu->zero) {
            TextPrevByte(memory, cpu, 0xb8adu);
            return TEXT_OPCODE_EXIT;
        }
        return TEXT_OPCODE_NEXT;
    case 0xb99du:                                  /* $76 wait for fade */
        LoadAAbsolute8(memory, cpu, 0x0581u, 0);
        if (cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x1261u, 0);
            BitImmediate8(cpu, 0x03u);
            if (cpu->zero)
                return TEXT_OPCODE_NEXT;
        }
        TextPrevByte(memory, cpu, 0xb9aeu);
        return TEXT_OPCODE_EXIT;
    case 0xac18u:                                  /* $AA clear $7F:D100 */
        SetAccumulatorWidth(cpu, 0);
        TransferDirectToA(cpu);
        Write16Long(memory, 0x7fd100u, cpu->accumulator);
        Write16Long(memory, 0x7fd102u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        return TEXT_OPCODE_NEXT;
    case 0xb69fu:                                  /* $CC raise $0B62 */
        LoadAAbsolute8(memory, cpu, 0x0b62u, 0);
        if (cpu->zero) {
            LoadA8(cpu, Read8(memory, 0x7fd4f7u));
            Compare8(cpu, A8(cpu),
                Read8(memory, AbsoluteIndexedAddress(cpu, 0x0b62u, 0)));
            if (cpu->carry)
                StoreAAbsolute8(memory, cpu, 0x0b62u, 0);
        }
        return TEXT_OPCODE_NEXT;
    case 0xbc0bu:                                  /* $60 PPU register */
        TransferDirectToA(cpu);
        TextNextByte(memory, cpu, 0xbc0eu);
        TransferAToX(cpu);
        TextNextByte(memory, cpu, 0xbc12u);
        StoreAAbsolute8(memory, cpu, 0x2100u, cpu->x);
        return TEXT_OPCODE_NEXT;
    case 0xa3edu:                                  /* $1E add into $079E */
        TransferDirectToA(cpu);
        TextNextByte(memory, cpu, 0xa3f0u);
        TransferAToX(cpu);
        TextNextByte(memory, cpu, 0xa3f4u);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x079eu, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x079eu, cpu->x);
        return TEXT_OPCODE_NEXT;
    case 0x9ed6u:                                  /* $09 print name buffer */
        Write16Absolute(memory, cpu, 0x1252u, cpu->y);
        LoadAAbsolute8(memory, cpu, 0x09b9u, 0);
        StoreAAbsolute8(memory, cpu, 0x1254u, 0);
        LoadA8(cpu, 0x10u);
        TestBitsAbsolute8(memory, cpu, 0x099bu, 1);
        LoadY16(cpu, 0x0badu);
        Write16Absolute(memory, cpu, 0x09b7u, cpu->y);
        StoreZeroAbsolute8(memory, cpu, 0x09b9u, 0);
        return TEXT_OPCODE_RELOAD;
    case 0xa480u:                                  /* $22 gold + word */
    case 0xa4cbu:                                  /* $26 gold - word */
        TextNextWord(memory, cpu, (uint16_t)(handler + 2u));
        TextGold(memory, cpu, handler == 0xa480u, (uint16_t)(handler + 5u));
        return TEXT_OPCODE_NEXT;
    case 0xaf53u:                                  /* $95/$94/$96 COLDATA */
    case 0xaf5bu:
    case 0xaf63u:
        LoadA8(cpu, handler == 0xaf53u ? 0x53u
            : handler == 0xaf5bu ? 0x93u : 0xd3u);
        TextColorFade(memory, cpu, (uint16_t)(handler + 4u));
        return TEXT_OPCODE_NEXT;
    default:
        return TEXT_OPCODE_HANDOFF;
    }
}

/* $80:9CB8 text step: plain characters native, the rest on LLE. */
Lufia2ActorPrimaryUpdateResult Lufia2TextEngineStepBody(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result) {
    unsigned opcodes;
    unsigned words = 0;

    LoadA8(cpu, Read8(memory, 0x7fd0ffu));                     /* 9CB8 */
    if (!cpu->zero) {
        Compare8(cpu, A8(cpu), 0x03u);
        if (!cpu->carry) {
            DecrementA8(cpu);
            Write8(memory, 0x7fd0ffu, A8(cpu));
            result.pc = 0x809cc7u;
            return result;
        }
        TransferDirectToA(cpu);                                /* 9CC8 */
        Write8(memory, 0x7fd0ffu, A8(cpu));
    }
    PushDataBank(memory, cpu);                                 /* 9CCD */
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    StoreZeroAbsolute8(memory, cpu, 0x125du, 0);
    StoreZeroAbsolute8(memory, cpu, 0x125eu, 0);
    opcodes = 0;
reload:
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0x09b9u, 0);               /* 9CD9 */
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09b7u, 0));
        LoadAAbsolute8(memory, cpu, 0x1259u, 0);
        if (cpu->zero)
            break;
        {
            const uint32_t count = AbsoluteIndexedAddress(cpu, 0x125au, 0);
            const uint8_t left = (uint8_t)(Read8(memory, count) - 1u);

            Write8(memory, count, left);
            SetNz8(cpu, left);
        }
        if (!cpu->zero)
            break;
        SetAccumulatorWidth(cpu, 0);                           /* 9CEB */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1257u, 0));
        Write16Absolute(memory, cpu, 0x09b7u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x1259u, 0);
        StoreAAbsolute8(memory, cpu, 0x09b9u, 0);
        StoreZeroAbsolute8(memory, cpu, 0x1259u, 0);
    }
    for (;;) {
        uint16_t handler;
        uint32_t handoff = 0x809d3bu;

        Write16Absolute(memory, cpu, 0x09b7u, cpu->y);         /* 9D00 */
        StoreZeroAbsolute8(memory, cpu, 0x0563u, 0);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, 0x099bu, 0);
        And8(cpu, 0x01u);
        if (cpu->zero) {
            TextCloseWindow(memory, cpu, 0x9d30u);             /* 9D2E */
        } else {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);      /* 9D0E */
            Compare8(cpu, A8(cpu), 0x10u);
            if (cpu->carry) {
                StoreAAbsolute8(memory, cpu, 0x09afu, 0);
                StoreZeroAbsolute8(memory, cpu, 0x09b0u, 0);
                Compare8(cpu, A8(cpu), 0x80u);
                if (!cpu->carry)
                    break;                                     /* BCE4 */
                if (words >= 4096u) {
                    result = FieldLoopHandoff(cpu, 0x809d22u);
                    result.dispatches = words;
                    return result;
                }
                ++words;
                TextNextByte(memory, cpu, 0x9d24u);            /* 9D22 */
                cpu->carry = 1;
                Sbc8(cpu, 0x80u);
                ExchangeAccumulatorBytes(cpu);
                LoadA8(cpu, 0x02u);
                TextSubScript(memory, cpu);
                continue;
            }
        }
        TransferDirectToA(cpu);                                /* 9D31 */
        TextNextByte(memory, cpu, 0x9d34u);
        SetAccumulatorWidth(cpu, 0);
        AslA16(cpu);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        handler = (uint16_t)(
            Read8(memory, 0x800000u | (uint16_t)(0xca14u + cpu->x)) |
            (Read8(memory, 0x800000u | (uint16_t)(0xca15u + cpu->x)) << 8));
        switch (TextScriptOpcode(memory, cpu,
                    opcodes < 4096u ? handler : 0u, &handoff)) {
        case TEXT_OPCODE_NEXT:                                 /* 9D00 */
            ++opcodes;
            continue;
        case TEXT_OPCODE_RELOAD:                               /* 9CD9 */
            ++opcodes;
            goto reload;
        case TEXT_OPCODE_EXIT:                                 /* 9DB0 */
            return TextEngineExit(memory, cpu, result);
        default:                                               /* 9D3B */
            result = FieldLoopHandoff(cpu, handoff);
            result.dispatches = handoff == 0x809d3bu ? opcodes : 0;
            return result;
        }
    }
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);                   /* BCE4 */
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        LoadA8(cpu, 0x10u);
        TestBitsAbsolute8(memory, cpu, 0x099cu, 0);
        if (!cpu->zero)
            return FieldLoopHandoff(cpu, 0x80bcf2u);           /* C56E */
    }
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* BCF5 */
    And8(cpu, 0x10u);
    if (!cpu->zero)
        IncrementY16(cpu);
    else
        TextNextByte(memory, cpu, 0xbd01u);
    Write16Absolute(memory, cpu, 0x09b7u, cpu->y);             /* BD02 */
    TextGlyphUpload(memory, cpu, 0xbd07u);
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);                   /* BD08 */
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x0b52u, 0);
        Write8(memory, 0x7fd0ffu, A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x1255u, 0);
        Compare8(cpu, A8(cpu), 0x10u);
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x1260u, 0);
            if (!cpu->zero) {
                LoadA8(cpu, (uint8_t)(Read8(memory, 0x7fd0c0u) ^ 0x01u));
                Write8(memory, 0x7fd0c0u, A8(cpu));
                if (!cpu->zero) {
                    LoadAAbsolute8(memory, cpu, 0x1260u, 0);
                    SimulateJslFrame(memory, cpu, 0x80u, 0xbd34u);
                    ExchangeAccumulatorBytes(cpu);             /* $84:8766 */
                    LoadA8(cpu, Read8(memory, 0x0005b6u));
                    BitImmediate8(cpu, 0x02u);
                    if (cpu->zero) {
                        ExchangeAccumulatorBytes(cpu);
                        Write8(memory, 0x0017acu, A8(cpu));
                    }
                    SimulateRtlFrame(memory, cpu);
                }
            }
        }
    }
    return TextEngineExit(memory, cpu, result);
}

/* $80:C11C: clear bit 0 of the actor flags $0622-$0649. */
static void TextReleaseActors(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9ca5u);
    Push8(memory, cpu, PackStatus(cpu));                       /* C11C */
    SetIndexWidth(cpu, 1);
    cpu->x = 0x27u;
    do {
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        And8(cpu, 0xfeu);
        StoreAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        cpu->x = (uint8_t)(cpu->x - 1u);
        SetNz8(cpu, (uint8_t)cpu->x);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $80:9C80: text box waits for its timer or the A/X buttons. */
Lufia2ActorPrimaryUpdateResult Lufia2TextPromptTick(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu,
    Lufia2ActorPrimaryUpdateResult result) {
    LoadAAbsolute8(memory, cpu, 0x1265u, 0);                   /* 9C80 */
    if (!cpu->zero) {
        const uint32_t timer = AbsoluteIndexedAddress(cpu, 0x1266u, 0);
        const uint8_t count = (uint8_t)(Read8(memory, timer) + 1u);

        Write8(memory, timer, count);
        SetNz8(cpu, count);
        Compare8(cpu, A8(cpu), count);
        if (cpu->carry)
            return result;
    } else {
        LoadA8(cpu, 0xa0u);                                    /* 9C8F */
        SimulateJsrFrame(memory, cpu, 0x9c93u);
        And8(cpu, DirectByte(memory, cpu, 0x46u));             /* C81E */
        if (!cpu->zero)
            TestBitsDirect(memory, cpu, 0x4au, 0);
        SimulateRtsFrame(memory, cpu);
        if (cpu->zero)
            return result;
    }
    LoadA8(cpu, 0x08u);                                        /* 9C96 */
    TestBitsAbsolute8(memory, cpu, 0x099bu, 0);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, 0x099bu, 0);
        TextCloseWindow(memory, cpu, 0x9ca2u);
        TextReleaseActors(memory, cpu);
        return result;
    }
    LoadAAbsolute8(memory, cpu, 0x099bu, 0);                   /* 9CA8 */
    And8(cpu, 0xfdu);
    StoreAAbsolute8(memory, cpu, 0x099bu, 0);
    return Lufia2TextEngineStepBody(memory, cpu, result);
}

/* $80:9CB8: text engine step, JSL entry. */
Lufia2ActorPrimaryUpdateResult Lufia2TextEngineStep(
    const Lufia2ActorFrontendMemory *memory,
    Lufia2ActorFrontendCpu *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return FieldLoopHandoff(cpu, 0x809cb8u);
    return Lufia2TextEngineStepBody(memory, cpu, FieldLoopResult(0x809db2u));
}
