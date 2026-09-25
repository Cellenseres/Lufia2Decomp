/* Field NMI uploads ($83:9FA9). */

#include "core/cpu_internal.h"
#include "lufia2/field.h"
#include "field/field_internal.h"
#include "system/wram.h"

/* $83:A052: CGRAM upload on DMA channel 6. */
static void NmiCgramUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StoreYIndex(memory, cpu, SNES_DASL(6));                    /* A052 */
    StoreAAbsolute8(memory, cpu, SNES_CGADD, 0);
    Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->x);
    StoreA8Absolute(memory, cpu, SNES_DMAP(6), 0x00u);
    StoreA8Absolute(memory, cpu, SNES_A1B(6), 0x00u);
    StoreA8Absolute(memory, cpu, SNES_BBAD(6), 0x22u);
    StoreA8Absolute(memory, cpu, SNES_MDMAEN, 0x40u);
}

/* $83:A033: queued palette uploads, $73 bits 0-1. */
static void NmiPaletteUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fb3u);
    LoadA8(cpu, 0x01u);                                        /* A033 */
    TrbDirect8(memory, cpu, DP_NMI_UPLOAD_FLAGS);
    if (!cpu->zero) {
        LoadA8(cpu, 0x10u);
        LoadX16(cpu, 0x0340u);
        LoadY16(cpu, 0x00e0u);
        SimulateJsrFrame(memory, cpu, 0xa043u);
        NmiCgramUpload(memory, cpu);
        SimulateRtsFrame(memory, cpu);
    }
    LoadA8(cpu, 0x02u);                                        /* A044 */
    TrbDirect8(memory, cpu, DP_NMI_UPLOAD_FLAGS);
    if (!cpu->zero) {
        LoadA8(cpu, 0xf0u);
        LoadX16(cpu, 0x0500u);
        LoadY16(cpu, 0x0020u);
        NmiCgramUpload(memory, cpu);
    }
    SimulateRtsFrame(memory, cpu);
}

/* One two-part VRAM tile upload of $A0E1. */
static void NmiTileBlock(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t list,
    uint16_t source,
    uint8_t vmain,
    uint8_t bank,
    uint8_t setup,
    uint16_t second_step) {
    const uint16_t entry = Read16AbsoluteIndexed(memory, cpu, list, cpu->x);

    LoadA16(cpu, entry);
    if (cpu->zero)
        return;
    Write16Absolute(memory, cpu, (uint16_t)(list + cpu->x), 0x0000u);
    LoadY8(cpu, vmain);
    StoreYIndex(memory, cpu, SNES_VMAIN);
    And16(cpu, 0x1fffu);
    LsrA16(cpu);
    Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
    Write16Direct(memory, cpu, 0x33u, cpu->accumulator);
    LoadY8(cpu, bank);
    StoreYIndex(memory, cpu, SNES_A1B(6));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, source, cpu->x));
    Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
    Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
    if (setup) {
        LoadY8(cpu, 0x01u);
        StoreYIndex(memory, cpu, SNES_DMAP(6));
        LoadY8(cpu, 0x18u);
        StoreYIndex(memory, cpu, SNES_BBAD(6));
    }
    LoadA16(cpu, 0x0040u);
    Write16Absolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
    LoadY8(cpu, 0x40u);
    StoreYIndex(memory, cpu, SNES_MDMAEN);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));
    cpu->carry = 0;
    Add16Immediate(cpu, 0x0040u);
    Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
    LoadA16(cpu, Read16Direct(memory, cpu, 0x33u));
    if (second_step == 1u) {
        IncrementA16(cpu);
    } else {
        cpu->carry = 0;
        Add16Immediate(cpu, second_step);
    }
    Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
    LoadA16(cpu, 0x0040u);
    Write16Absolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
    LoadY8(cpu, 0x40u);
    StoreYIndex(memory, cpu, SNES_MDMAEN);
}

/* $83:A0E1: queued VRAM tile uploads, four slots each list. */
static void NmiTileUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fb6u);
    SetAccumulatorWidth(cpu, 0);                               /* A0E1 */
    SetIndexWidth(cpu, 1);
    LoadY8(cpu, 0x80u);
    StoreYIndex(memory, cpu, SNES_VMAIN);
    LoadY8(cpu, 0x01u);
    StoreYIndex(memory, cpu, SNES_DMAP(6));
    LoadY8(cpu, 0x18u);
    StoreYIndex(memory, cpu, SNES_BBAD(6));
    LoadX8(cpu, 0x06u);
    for (;;) {
        NmiTileBlock(memory, cpu, 0x1246u, 0x1236u, 0x81u, 0x7fu, 0, 1u);
        NmiTileBlock(memory, cpu, 0x123eu, 0x122eu, 0x80u, 0x7eu, 1,
            0x0020u);                                          /* A13E */
        LoadX8(cpu, (uint8_t)(cpu->x - 2u));                   /* A193 */
        if (cpu->negative)
            break;
    }
    LoadY8(cpu, 0x80u);                                        /* A19A */
    StoreYIndex(memory, cpu, SNES_VMAIN);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A1A2: column uploads listed at $7F:D4F8. */
static void NmiColumnUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fbfu);
    TransferDirectToA(cpu);                                    /* A1A2 */
    LoadA8(cpu, Read8(memory, 0x7fd4f8u));
    TransferAToX(cpu);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    do {
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd4f8u, cpu->x)));
        Write16Direct(memory, cpu, 0x33u, cpu->accumulator);   /* A1A8 */
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd518u, cpu->x)));
        Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fd538u, cpu->x)));
        Write16Direct(memory, cpu, 0x37u, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        StoreA8Absolute(memory, cpu, SNES_A1B(6), 0x7eu);
        StoreA8Absolute(memory, cpu, SNES_DMAP(6), 0x01u);
        StoreA8Absolute(memory, cpu, SNES_BBAD(6), 0x18u);
        do {
            SetAccumulatorWidth(cpu, 0);                       /* A1CD */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x33u));
            Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
            cpu->carry = 0;
            Add16Immediate(cpu, 0x0080u);
            And16(cpu, 0x07ffu);
            Write16Direct(memory, cpu, 0x39u, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x33u));
            And16(cpu, 0xf800u);
            LoadA16(cpu, (uint16_t)(cpu->accumulator |
                Read16Direct(memory, cpu, 0x39u)));
            Write16Direct(memory, cpu, 0x33u, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));    /* A1E6 */
            Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
            cpu->carry = 0;
            Add16Immediate(cpu, 0x0040u);
            And16(cpu, 0x03ffu);
            Write16Direct(memory, cpu, 0x39u, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));
            And16(cpu, 0xfc00u);
            LoadA16(cpu, (uint16_t)(cpu->accumulator |
                Read16Direct(memory, cpu, 0x39u)));
            Write16Direct(memory, cpu, 0x35u, cpu->accumulator);
            LoadA16(cpu, 0x0080u);
            Write16Absolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            StoreA8Absolute(memory, cpu, SNES_MDMAEN, 0x40u);
            DecrementDirect8(memory, cpu, 0x37u);              /* A20A */
        } while (!cpu->zero);
        LoadX16(cpu, (uint16_t)(cpu->x - 2u));                 /* A20E */
    } while (!cpu->zero);
    TransferDirectToA(cpu);                                    /* A212 */
    Write8(memory, 0x7fd4f8u, A8(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 1);
    SimulateRtsFrame(memory, cpu);
}

/* $83:A070: eight queued VRAM block uploads at $05C2. */
static void NmiBlockUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fc2u);
    SetAccumulatorWidth(cpu, 1);                               /* A070 */
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, 0x0000u);
    do {
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05c2u, cpu->x));
        if (!cpu->zero) {
            Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->y); /* A07C */
            Write16Direct(memory, cpu, 0x33u, cpu->y);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x05c2u, cpu->x), 0x00u);
            Write8(memory, AbsoluteIndexedAddress(cpu, 0x05c3u, cpu->x), 0x00u);
            LoadAAbsolute8(memory, cpu, 0x11d9u, cpu->x);
            StoreAAbsolute8(memory, cpu, SNES_A1B(6), 0);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11e9u, cpu->x));
            Write16Direct(memory, cpu, 0x35u, cpu->y);
            Write16Absolute(memory, cpu, SNES_VMADDL, cpu->y);
            LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x11f9u, cpu->x));
            Write16Direct(memory, cpu, 0x37u, cpu->y);
            Write16Absolute(memory, cpu, SNES_DASL(6), cpu->y);
            StoreA8Absolute(memory, cpu, SNES_DMAP(6), 0x01u);
            StoreA8Absolute(memory, cpu, SNES_BBAD(6), 0x18u);
            StoreA8Absolute(memory, cpu, SNES_MDMAEN, 0x40u);
            SetAccumulatorWidth(cpu, 0);                       /* A0AC */
            LoadA16(cpu, Read16Direct(memory, cpu, 0x35u));
            cpu->carry = 0;
            Add16Immediate(cpu, 0x0100u);
            Write16Absolute(memory, cpu, SNES_VMADDL, cpu->accumulator);
            LoadA16(cpu, Read16Direct(memory, cpu, 0x37u));
            Write16Absolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x33u));
            Write16Absolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
            SetAccumulatorWidth(cpu, 1);
            StoreA8Absolute(memory, cpu, SNES_DMAP(6), 0x01u);
            StoreA8Absolute(memory, cpu, SNES_BBAD(6), 0x18u);
            StoreA8Absolute(memory, cpu, SNES_MDMAEN, 0x40u);
        }
        IncrementX16(cpu);                                     /* A0D3 */
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0010u);
    } while (!cpu->carry);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0732u, 0), 0x00u);
    Write8(memory, AbsoluteIndexedAddress(cpu, 0x0733u, 0), 0x00u);
    SimulateRtsFrame(memory, cpu);
}

/* $83:9FE1: colour window fade via $7F:D0F2/D0F3. */
static void NmiFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9fc5u);
    LoadAAbsolute8(memory, cpu, 0x09aau, 0);                   /* 9FE1 */
    BitImmediate8(cpu, 0x01u);
    if (!cpu->zero) {
        LoadA8(cpu, Read8(memory, 0x7fd0f2u));
        And8(cpu, 0xf0u);
        Or8(cpu, 0x0fu);
        StoreAAbsolute8(memory, cpu, SNES_MOSAIC, 0);
        LoadA8(cpu, Read8(memory, 0x7fd0f2u));
        And8(cpu, 0x0fu);
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, 0x7fd0f3u));
        Write8(memory, 0x7fd0f3u, A8(cpu));
        BitImmediate8(cpu, 0x08u);                             /* A002 */
        if (!cpu->zero) {
            uint8_t finished;

            And8(cpu, 0x80u);
            Write8(memory, 0x7fd0f3u, A8(cpu));
            if (!cpu->negative) {
                LoadA8(cpu, Read8(memory, 0x7fd0f2u));         /* A00E */
                cpu->carry = 1;
                Sbc8(cpu, 0x10u);
                Write8(memory, 0x7fd0f2u, A8(cpu));
                finished = !cpu->carry;
            } else {
                LoadA8(cpu, Read8(memory, 0x7fd0f2u));         /* A01D */
                cpu->carry = 0;
                Adc8(cpu, 0x10u);
                Write8(memory, 0x7fd0f2u, A8(cpu));
                finished = cpu->carry;
            }
            if (finished) {
                LoadA8(cpu, 0x01u);                            /* A02A */
                TestBitsAbsolute8(memory, cpu, 0x09aau, 0);
            }
        }
        LoadAAbsolute8(memory, cpu, 0x09aau, 0);               /* A02F */
    }
    SimulateRtsFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2FieldNmiUploads(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Lufia2ExecutionResult result;

    Push8(memory, cpu, PackStatus(cpu));                       /* 9FA9 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    Push8(memory, cpu, 0x83u);
    PullDataBank(memory, cpu);
    NmiPaletteUploads(memory, cpu);
    NmiTileUploads(memory, cpu);
    LoadA8(cpu, Read8(memory, 0x7fd4f8u));                     /* 9FB7 */
    if (!cpu->zero)
        NmiColumnUploads(memory, cpu);
    NmiBlockUploads(memory, cpu);
    NmiFade(memory, cpu);
    LoadA8(cpu, Read8(memory, 0x7fd081u));                     /* 9FC6 */
    LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
    cpu->carry = 1;
    Adc8(cpu, 0x08u);
    if (cpu->negative)
        TransferDirectToA(cpu);                                /* 9FD1 */
    StoreAAbsolute8(memory, cpu, SNES_WH0, 0);
    Adc8(cpu, 0xeeu);
    if (!cpu->negative)
        LoadA8(cpu, 0xffu);                                    /* 9FD9 */
    StoreAAbsolute8(memory, cpu, SNES_WH1, 0);
    PullDataBank(memory, cpu);                                 /* 9FDE */
    UnpackStatus(cpu, Pull8(memory, cpu));
    result.flow = LUFIA2_EXECUTION_RETURNED;
    result.pc = 0x839fe0u;
    result.dispatches = 0;
    return result;
}
