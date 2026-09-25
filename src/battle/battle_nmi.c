/* Battle NMI uploads ($85:8DC5). */

#include "core/cpu_internal.h"
#include "lufia2/battle.h"
#include "system/wram.h"

/* $85:8E98: sixteen queued VRAM DMA uploads on channel 6. */
static void BattleVramQueue(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x8dccu);
    LoadX16(cpu, 0x005au);                                     /* 8E98 */
    do {
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a8fu, cpu->x));
        if (!cpu->zero) {
            Write16Absolute(memory, cpu, SNES_DASL(6), cpu->y);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a91u, cpu->x));
            Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->y);
            LoadA8(cpu, 0x01u);
            StoreAAbsolute8(memory, cpu, SNES_DMAP(6), 0);
            LoadA8(cpu, 0x7eu);
            StoreAAbsolute8(memory, cpu, SNES_A1B(6), 0);
            LoadA8(cpu, 0x18u);
            StoreAAbsolute8(memory, cpu, SNES_BBAD(6), 0);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1a93u, cpu->x));
            Write16Absolute(memory, cpu, SNES_VMADDL, cpu->y);
            LoadA8(cpu, 0x40u);
            StoreAAbsolute8(memory, cpu, SNES_MDMAEN, 0);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x1a8fu, cpu->x), 0x00u);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x1a90u, cpu->x), 0x00u);
        }
        LoadX16(cpu, (uint16_t)(cpu->x - 6u));                 /* 8EC9 */
    } while (!cpu->negative);
    SimulateRtsFrame(memory, cpu);
}

/* $85:8ED2: latch HDMA channels from $1AEF, enable $420C. */
static void BattleHdmaChannels(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
        LoadY16(cpu, SNES_DMAP(0));
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
    StoreAAbsolute8(memory, cpu, SNES_HDMAEN, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $85:A8E7: timer 1, battle HDMA wave table at $7E:40CC. */
static void BattleWaveTable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
Lufia2ExecutionResult Lufia2BattleNmiUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t scroll_regs[6] = {
        SNES_BG1HOFS, SNES_BG1VOFS, SNES_BG2HOFS, SNES_BG2VOFS, SNES_BG3HOFS, SNES_BG3VOFS};
    static const uint16_t window_regs[8] = {
        SNES_W12SEL, SNES_WOBJSEL, SNES_WH1, SNES_WH3, SNES_WOBJLOG, SNES_TS, SNES_TSW,
        SNES_CGADSUB};
    Lufia2ExecutionResult result;
    unsigned i;

    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x858e97u;
    result.dispatches = 0;
    /* The NMI handler always enters with M=1. */
    if (!cpu->accumulator_is_8_bit) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
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
    LoadAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
    StoreAAbsolute8(memory, cpu, SNES_INIDISP, 0);
    LoadA8(cpu, Read8(memory, DirectAddress(cpu, 0xdbu)));
    if (!cpu->zero && !BattleTimers(memory, cpu)) {
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc;
        return result;
    }
    LoadY16(cpu, 0x0000u);                                     /* 8E79 */
    LoadAAbsolute8(memory, cpu, 0x12e3u, cpu->y);
    if (!cpu->negative) {
        /* Queued tasks run through JSR ($9E37,x) on LLE. */
        result.flow = LUFIA2_EXECUTION_BOUNDARY;
        result.pc = cpu->resume_pc = 0x858e7fu;
        return result;
    }
    PullDataBank(memory, cpu);                                 /* 8E96 */
    return result;
}
