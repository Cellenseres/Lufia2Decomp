/* Intro logos and title state flow. */

#include "core/cpu_internal.h"
#include "lufia2/title.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* $82:E746: JSR $8028 inline table on $30; handlers on LLE. */
Lufia2ExecutionResult Lufia2TitleStateDispatch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    uint32_t table;
    uint16_t target;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x82e746u);
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
        Lufia2ExecutionResult result =
            ExecutionHandoff(cpu, 0x820000u | target);

        /* The ROM passed these at this S already. */
        result.dispatches = target == 0xe746u || target == 0xe748u ||
            (target >= 0x8031u && target <= 0x8041u && (target & 1u));
        return result;
    }
}

/* $80:9357: DMA channel 6, $700 bytes from $7E:X to VRAM Y. */
static void IntroVramDma(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreWordAbsolute(memory, cpu, SNES_A1TL(6), cpu->x);      /* 9357 */
    StoreWordAbsolute(memory, cpu, SNES_VMADDL, cpu->y);
    StoreAImmediate8(memory, cpu, 0x7eu, SNES_A1B(6));
    LoadX16(cpu, 0x0700u);
    StoreWordAbsolute(memory, cpu, SNES_DASL(6), cpu->x);
    StoreAImmediate8(memory, cpu, 0x01u, SNES_DMAP(6));
    StoreAImmediate8(memory, cpu, 0x18u, SNES_BBAD(6));
    StoreAImmediate8(memory, cpu, 0x40u, SNES_MDMAEN);
    SimulateRtsFrame(memory, cpu);
}

/* $80:92FE/$80:9346: logo tiles from $7E:X, then the next state. */
static void IntroLogoUpload(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t source,
    uint16_t return_address) {
    LoadX16(cpu, source);
    LoadY16(cpu, 0x0000u);
    IntroVramDma(memory, cpu, return_address);
    IncrementDirect8(memory, cpu, 0x50u);
    Write8(memory, DirectAddress(cpu, 0x4eu), 0x00u);
}

/* $80:92A4: intro NMI, state $50 through the table $80:92B7. */
Lufia2ExecutionResult Lufia2IntroNmi(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
        return ExecutionHandoff(cpu, 0x8092b1u);
    SimulateJsrFrame(memory, cpu, 0x92b3u);
    switch (handler) {
    case 0x92cbu:                                  /* scroll row upload */
        SetAccumulatorWidth(cpu, 0);
        LoadA16(cpu, Read16Long(memory, 0x7e4004u));
        cpu->carry = 0;
        Add16Value(cpu, 0x4000u);
        StoreWordAbsolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
        LoadA16(cpu, Read16Long(memory, 0x7e4006u));
        StoreWordAbsolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadX16(cpu, 0x4000u);
        StoreWordAbsolute(memory, cpu, SNES_VMADDL, cpu->x);
        StoreAImmediate8(memory, cpu, 0x7eu, SNES_A1B(6));
        StoreAImmediate8(memory, cpu, 0x01u, SNES_DMAP(6));
        StoreAImmediate8(memory, cpu, 0x18u, SNES_BBAD(6));
        StoreAImmediate8(memory, cpu, 0x40u, SNES_MDMAEN);
        IncrementDirect8(memory, cpu, 0x50u);
        break;
    case 0x92feu:
        IntroLogoUpload(memory, cpu, 0x2000u, 0x9306u);
        break;
    case 0x930cu:                                  /* fade in over 32 */
        LoadA8(cpu, DirectByte(memory, cpu, 0x4eu));
        LsrA8(cpu);
        StoreAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
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
            StoreAImmediate8(memory, cpu, BRIGHTNESS_FORCED_BLANK, WRAM_BRIGHTNESS);
        } else {
            LsrA8(cpu);
            StoreAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
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
    return ExecutionReturned(0x8092b6u);
}

/* $86:88E5: one OAM entry for the title particle at X. The sprite
   list [$0003] in bank $86 holds the size flag, the y and x offsets
   and the tile word; $00 counts entries for the OAM high table. */
static void TitleParticleSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* 88E5 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0003u, cpu->x));
    StoreADirect16(memory, cpu, 0x08u);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x86u);
    StoreADirect8(memory, cpu, 0x0au);
    LoadY16(cpu, 0x0000u);
    LoadA8(cpu, Read8IndirectLongY(memory, cpu, 0x08u));       /* size flag */
    AslA8(cpu);
    StoreADirect8(memory, cpu, 0x02u);
    Write8(memory, DirectAddress(cpu, 0x03u), 0x00u);
    IncrementY16(cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16IndirectLongY(memory, cpu, 0x08u));     /* y, x offsets */
    StoreADirect16(memory, cpu, 0x04u);
    IncrementY16(cpu);
    IncrementY16(cpu);
    SetAccumulatorWidth(cpu, 1);
    PushY(memory, cpu);                                        /* 8907 */
    LoadYDirect16(memory, cpu, 0x06u);
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->x);              /* OAM x */
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x05u));
    StoreAAbsolute8(memory, cpu, 0x0100u, cpu->y);
    IncrementY16(cpu);
    LoadAAbsolute8(memory, cpu, 0x0007u, cpu->x);              /* OAM y */
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x04u));
    StoreAAbsolute8(memory, cpu, 0x0100u, cpu->y);
    IncrementY16(cpu);
    StoreYDirect16(memory, cpu, 0x06u);
    cpu->y = PullIndexValue(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* 8921 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x000bu, cpu->x));
    cpu->carry = 0;
    Add16Value(cpu, Read16IndirectLongY(memory, cpu, 0x08u));  /* tile word */
    IncrementY16(cpu);
    IncrementY16(cpu);
    PushY(memory, cpu);
    LoadYDirect16(memory, cpu, 0x06u);
    StoreAAbsolute16(memory, cpu, 0x0100u, cpu->y);
    IncrementY16(cpu);
    IncrementY16(cpu);
    StoreYDirect16(memory, cpu, 0x06u);
    LoadADirect16(memory, cpu, 0x00u);                         /* 8937 */
    LsrA16(cpu);
    LsrA16(cpu);
    LsrA16(cpu);
    And16(cpu, 0x00feu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0200u);                                  /* high table */
    TransferAToY(cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0100u, cpu->y));
    StoreADirect16(memory, cpu, 0x11u);
    LoadADirect16(memory, cpu, 0x00u);
    And16(cpu, 0x000fu);
    IncrementA16(cpu);
    StoreADirect16(memory, cpu, 0x13u);                        /* shift count */
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, DirectByte(memory, cpu, 0x05u));               /* 8953 */
    And8(cpu, 0x80u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, 0x0005u, cpu->x);          /* x - offset */
        cpu->carry = 1;
        Sbc8(cpu, DirectByte(memory, cpu, 0x05u));
        LoadAAbsolute8(memory, cpu, 0x0006u, cpu->x);
        Sbc8(cpu, 0x00u);
    } else {
        LoadA8(cpu, DirectByte(memory, cpu, 0x05u));           /* 8969 */
        LoadA8(cpu, (uint8_t)~A8(cpu));                        /* EOR #$FF */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, 0x0005u, cpu->x));
        LoadAAbsolute8(memory, cpu, 0x0006u, cpu->x);
        Adc8(cpu, 0x00u);
    }
    cpu->carry = 0;
    Adc8(cpu, 0xffu);                                          /* carry: x bit 8 */
    SetAccumulatorWidth(cpu, 0);                               /* 897A */
    RolA16(cpu);
    And16(cpu, 0x0001u);
    Or16(cpu, Read16Direct(memory, cpu, 0x02u));
    cpu->carry = 0;
    RorA16(cpu);
    do {                                                       /* 8984 */
        RolA16(cpu);
        Decrement16Direct(memory, cpu, 0x13u);
    } while (!cpu->zero);
    Or16(cpu, Read16Direct(memory, cpu, 0x11u));
    StoreAAbsolute16(memory, cpu, 0x0100u, cpu->y);
    Increment16Direct(memory, cpu, 0x00u);
    Increment16Direct(memory, cpu, 0x00u);
    SetAccumulatorWidth(cpu, 1);
    cpu->y = PullIndexValue(memory, cpu);                      /* 8994 */
}

/* $86:88BE: OAM entries for the 75 title particles at $C448
   (13 bytes each; byte 1 = active). */
Lufia2ExecutionResult Lufia2TitleParticleSprites(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8688beu);
    LoadX16(cpu, 0x0000u);                                     /* 88BE */
    StoreXDirect16(memory, cpu, 0x00u);
    StoreXDirect16(memory, cpu, 0x06u);                        /* OAM index */
    LoadA8(cpu, 0x4bu);
    StoreADirect8(memory, cpu, 0x15u);
    LoadX16(cpu, 0xc448u);
    do {
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);          /* 88CC */
        if (!cpu->zero) {
            PushIndex(memory, cpu);
            SimulateJsrFrame(memory, cpu, 0x88d4u);
            TitleParticleSprite(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            cpu->x = PullIndexValue(memory, cpu);
        }
        SetAccumulatorWidth(cpu, 0);                           /* 88D6 */
        TransferXToA(cpu);
        cpu->carry = 0;
        Add16Value(cpu, 0x000du);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        DecrementDirect8(memory, cpu, 0x15u);
    } while (!cpu->zero);
    return ExecutionReturned(0x8688e4u);                       /* RTS */
}

/* Title objects: three records at $C400/$C418/$C430 ($15BA), each
   with its sprite record ($15BC) and a pool of 24 particles ($15BE).
   Record: +0/+2 centre, +4/+6 angles (8.8, wrap at $B400), +8 timer,
   +$0A/+$0C radii, +$0E/+$10 radius steps, +$12 flags, +$13/+$15 angle
   steps, +$17 depth. */
enum {
    TITLE_OBJECT = 0x15bau,
    TITLE_OBJECT_SPRITE = 0x15bcu,
    TITLE_OBJECT_PARTICLES = 0x15beu,
    TITLE_SIGNS = 0x1583u,
    TITLE_PRODUCT = 0x1575u,
    TITLE_PARTICLE_SLOTS = 0x18u,
};

static uint16_t TitleWord(
    const Lufia2Memory *memory, const Lufia2CpuState *cpu, uint16_t at) {
    return Read16AbsoluteIndexed(memory, cpu, at, 0);
}

/* word[y + to] += word[y + step]. */
static void TitleAccumulate(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t to,
    uint16_t step) {
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, to, cpu->y));
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, step, cpu->y));
    StoreAAbsolute16(memory, cpu, to, cpu->y);
}

/* $86:8418: angle past $B400 turns by a flag-chosen $B400. */
static void TitleWrapAngle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t angle,
    uint16_t flag) {
    Compare16(cpu, cpu->accumulator, 0xb400u);
    if (!cpu->carry)
        return;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0012u, cpu->y));
    And16(cpu, flag);
    if (!cpu->zero) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, angle, cpu->y));
        cpu->carry = 0;
        Add16Value(cpu, 0xb400u);
    } else {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, angle, cpu->y));
        Subtract16(cpu, 0xb400u);
    }
    StoreAAbsolute16(memory, cpu, angle, cpu->y);
}

/* $86:8444: sign bit into $1583, magnitude * 2 into $1570. */
static void TitleTrigMagnitude(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    AslA8(cpu);
    RolAbsolute8(memory, cpu, TITLE_SIGNS);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    StoreAAbsolute16(memory, cpu, 0x1570u, 0);
}

/* $82:8000 then $86:8503: $1575 = signed magnitude * factor. */
static void TitleSignedProduct(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t multiply_return,
    uint16_t sign_return) {
    StoreAAbsolute16(memory, cpu, 0x1572u, 0);
    Lufia2CallMultiply(memory, cpu, 0x86u, multiply_return);
    SimulateJsrFrame(memory, cpu, sign_return);
    LsrAbsolute16(memory, cpu, TITLE_SIGNS);                   /* 8503 */
    if (cpu->carry) {
        LoadA16(cpu, (uint16_t)(~TitleWord(memory, cpu, TITLE_PRODUCT)));
        IncrementA16(cpu);
        StoreAAbsolute16(memory, cpu, TITLE_PRODUCT, 0);
    }
    SimulateRtsFrame(memory, cpu);
}

/* $86:83F2: advance radii and angles, place the sprite on the
   ellipse; returns the depth step (0-10) from the second angle. */
static void TitleObjectProject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* 83F2 */
    LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));
    TitleAccumulate(memory, cpu, 0x000au, 0x000eu);
    TitleAccumulate(memory, cpu, 0x000cu, 0x0010u);
    TitleAccumulate(memory, cpu, 0x0004u, 0x0013u);
    TitleWrapAngle(memory, cpu, 0x0004u, 0x0001u);
    SetAccumulatorWidth(cpu, 1);                               /* 843B */
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    Lufia2CallCosine(memory, cpu, 0x86u, 0x8443u);
    TitleTrigMagnitude(memory, cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x000bu, cpu->y));
    And16(cpu, 0x00ffu);
    TitleSignedProduct(memory, cpu, 0x845cu, 0x845fu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0000u, cpu->y));
    cpu->carry = 0;
    Add16Value(cpu, TitleWord(memory, cpu, TITLE_PRODUCT));    /* sprite x */
    StoreAAbsolute16(memory, cpu, 0x0005u, cpu->x);
    SetAccumulatorWidth(cpu, 1);                               /* 846A */
    LoadAAbsolute8(memory, cpu, 0x0005u, cpu->y);
    Lufia2CallSine(memory, cpu, 0x86u, 0x8472u);
    TitleTrigMagnitude(memory, cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x000du, cpu->y));
    And16(cpu, 0x00ffu);
    TitleSignedProduct(memory, cpu, 0x848bu, 0x848eu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0002u, cpu->y));
    Subtract16(cpu, TitleWord(memory, cpu, TITLE_PRODUCT));    /* sprite y */
    StoreAAbsolute16(memory, cpu, 0x0007u, cpu->x);
    TitleAccumulate(memory, cpu, 0x0006u, 0x0015u);            /* 8499 */
    TitleWrapAngle(memory, cpu, 0x0006u, 0x0002u);
    SetAccumulatorWidth(cpu, 1);                               /* 84C8 */
    LoadAAbsolute8(memory, cpu, 0x0007u, cpu->y);
    Lufia2CallCosine(memory, cpu, 0x86u, 0x84d0u);
    TitleTrigMagnitude(memory, cpu);
    LoadA16(cpu, 0x0064u);
    TitleSignedProduct(memory, cpu, 0x84e6u, 0x84e9u);
    LoadA16(cpu, TitleWord(memory, cpu, TITLE_PRODUCT));       /* 84EA */
    cpu->carry = 0;
    Add16Value(cpu, 0x0064u);
    LoadX16(cpu, 0x0014u);
    for (;;) {                                                 /* 84F4 */
        Compare16(cpu, cpu->accumulator,
            Read16Long(memory, LongIndexedAddress(0x868a5bu, cpu->x)));
        if (!cpu->negative)
            break;
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
        LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    }
    TransferXToA(cpu);
    LsrA16(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $86:8513: sprite list for the object's depth from $86:8AC0. */
static void TitleObjectSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));        /* 8513 */
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
    LoadA8(cpu, 0x00u);
    ExchangeAccumulatorBytes(cpu);
    LoadAAbsolute8(memory, cpu, 0x0017u, cpu->x);
    AslA8(cpu);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x868ac0u, cpu->x)));
    StoreAAbsolute8(memory, cpu, 0x0003u, cpu->y);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x868ac1u, cpu->x)));
    StoreAAbsolute8(memory, cpu, 0x0004u, cpu->y);
}

/* $86:8530: angle steps for the depth from $86:8A71, signed by
   flag bits 0 and 1. */
static void TitleObjectSpeed(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));                       /* 8530 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0017u, cpu->y));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x868a71u, cpu->x)));
    StoreAAbsolute16(memory, cpu, 0x0013u, cpu->y);
    LsrA16(cpu);
    StoreAAbsolute16(memory, cpu, 0x0015u, cpu->y);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0012u, cpu->y));
    cpu->zero = (cpu->accumulator & 0x0001u) == 0;
    if (!cpu->zero) {
        LoadA16(cpu, (uint16_t)~Read16AbsoluteIndexed(
            memory, cpu, 0x0013u, cpu->y));
        IncrementA16(cpu);
        StoreAAbsolute16(memory, cpu, 0x0013u, cpu->y);
    }
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0012u, cpu->y));
    cpu->zero = (cpu->accumulator & 0x0002u) == 0;
    if (!cpu->zero) {
        LoadA16(cpu, (uint16_t)~Read16AbsoluteIndexed(
            memory, cpu, 0x0015u, cpu->y));
        IncrementA16(cpu);
        StoreAAbsolute16(memory, cpu, 0x0015u, cpu->y);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $86:83C3: while the timer runs, move the object and refresh its
   sprite and speed on a depth change; else hide the sprite. */
static void TitleObjectStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* 83C3 */
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0008u, cpu->y));
    if (cpu->zero) {
        SetAccumulatorWidth(cpu, 1);                           /* 83E9 */
        LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
        StoreZeroAbsolute8(memory, cpu, 0x0001u, cpu->x);
        return;
    }
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    StoreAAbsolute16(memory, cpu, 0x0008u, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    SimulateJsrFrame(memory, cpu, 0x83d5u);
    TitleObjectProject(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));        /* 83D6 */
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x0017u, cpu->y));
    if (cpu->zero)
        return;
    StoreAAbsolute8(memory, cpu, 0x0017u, cpu->y);
    SimulateJsrFrame(memory, cpu, 0x83e3u);
    TitleObjectSprite(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x83e6u);
    TitleObjectSpeed(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

/* Y += 13, the particle stride. */
static void TitleNextParticle(Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, cpu->y);
    cpu->carry = 0;
    Add16Value(cpu, 0x000du);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $86:85C2: while the sprite shows, start one free particle at the
   sprite's place with the object's depth. */
static void TitleObjectSpawn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));                       /* 85C2 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
    LoadAAbsolute8(memory, cpu, 0x0001u, cpu->x);
    if (!cpu->zero) {
        LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_PARTICLES));
        LoadA8(cpu, TITLE_PARTICLE_SLOTS);
        StoreADirect8(memory, cpu, 0x04u);
        for (;;) {
            LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);      /* 85D6 */
            if (cpu->zero)
                break;
            TitleNextParticle(cpu);
            DecrementDirect8(memory, cpu, 0x04u);
            if (cpu->zero)
                goto done;
        }
        LoadA8(cpu, 0x01u);                                    /* 85EB */
        StoreAAbsolute8(memory, cpu, 0x0001u, cpu->y);
        LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));
        LoadAAbsolute8(memory, cpu, 0x0017u, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x0002u, cpu->y);
        SetAccumulatorWidth(cpu, 0);
        And16(cpu, 0x00ffu);
        AslA16(cpu);
        TransferAToX(cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x868a87u, cpu->x)));
        StoreAAbsolute16(memory, cpu, 0x0003u, cpu->y);
        LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0005u, cpu->x));
        StoreAAbsolute16(memory, cpu, 0x0005u, cpu->y);
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x0007u, cpu->x));
        StoreAAbsolute16(memory, cpu, 0x0007u, cpu->y);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x15b9u, 0);               /* palette */
        AslA8(cpu);
        StoreAAbsolute8(memory, cpu, 0x000bu, cpu->y);
        LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
        LoadAAbsolute8(memory, cpu, 0x000cu, cpu->x);
        StoreAAbsolute8(memory, cpu, 0x000cu, cpu->y);
        LoadA8(cpu, 0x02u);
        StoreAAbsolute8(memory, cpu, 0x000au, cpu->y);         /* frame delay */
        LoadA8(cpu, 0x18u);
        cpu->carry = 1;
        Sbc8(cpu, 0x03u);
        StoreAAbsolute8(memory, cpu, 0x0009u, cpu->y);         /* lifetime */
    }
done:
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $86:856F: particle frames every 2 ticks up to depth 10, then end
   of life. */
static void TitleObjectParticles(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));                       /* 856F */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_PARTICLES));
    LoadA8(cpu, TITLE_PARTICLE_SLOTS);
    StoreADirect8(memory, cpu, 0x04u);
    do {
        LoadAAbsolute8(memory, cpu, 0x0001u, cpu->y);          /* 857B */
        if (!cpu->zero) {
            TransferYToX(cpu);
            StepMemory8(memory, cpu, AbsoluteIndexedAddress(cpu, 0x000au, cpu->x), -1);
            if (cpu->zero) {
                LoadA8(cpu, 0x00u);
                ExchangeAccumulatorBytes(cpu);
                LoadAAbsolute8(memory, cpu, 0x0002u, cpu->y);
                Compare8(cpu, A8(cpu), 0x0au);
                if (!cpu->zero) {
                    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                    StoreAAbsolute8(memory, cpu, 0x0002u, cpu->y);
                    AslA8(cpu);
                    TransferAToX(cpu);
                    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x868a87u, cpu->x)));
                    StoreAAbsolute8(memory, cpu, 0x0003u, cpu->y);
                    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x868a88u, cpu->x)));
                    StoreAAbsolute8(memory, cpu, 0x0004u, cpu->y);
                    LoadA8(cpu, 0x02u);
                    StoreAAbsolute8(memory, cpu, 0x000au, cpu->y);
                }
            }
            TransferYToX(cpu);                                 /* 85A9 */
            StepMemory8(memory, cpu, AbsoluteIndexedAddress(cpu, 0x0009u, cpu->x), -1);
            if (cpu->zero)
                StoreZeroAbsolute8(memory, cpu, 0x0001u, cpu->x);
        }
        TitleNextParticle(cpu);                                /* 85B2 */
        DecrementDirect8(memory, cpu, 0x04u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $86:838C: step the three title objects whose flag bit 7 is set. */
Lufia2ExecutionResult Lufia2TitleObjects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x86838cu);
    LoadX16(cpu, 0x0000u);                                     /* 838C */
    do {
        SetAccumulatorWidth(cpu, 0);                           /* 838F */
        PushIndex(memory, cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x868a41u, cpu->x)));
        StoreAAbsolute16(memory, cpu, TITLE_OBJECT, 0);
        TransferAToY(cpu);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x868a47u, cpu->x)));
        StoreAAbsolute16(memory, cpu, TITLE_OBJECT_SPRITE, 0);
        LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x868a4du, cpu->x)));
        StoreAAbsolute16(memory, cpu, TITLE_OBJECT_PARTICLES, 0);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x0012u, cpu->y);
        And8(cpu, 0x80u);
        if (!cpu->zero) {
            SimulateJsrFrame(memory, cpu, 0x83b3u);
            TitleObjectStep(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            SimulateJsrFrame(memory, cpu, 0x83b6u);
            TitleObjectSpawn(memory, cpu);
            SimulateRtsFrame(memory, cpu);
            SimulateJsrFrame(memory, cpu, 0x83b9u);
            TitleObjectParticles(memory, cpu);
            SimulateRtsFrame(memory, cpu);
        }
        cpu->x = PullIndexValue(memory, cpu);                  /* 83BA */
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0006u);
    } while (!cpu->zero);
    return ExecutionReturned(0x8683c2u);                       /* RTS */
}

/* $86:86FC: 13 layer positions, 24-bit ($1587/$1597/$15A7), step by
   the words at $86:8726; the top byte takes only the carry. */
static void TitleLayerScroll(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, 0x0000u);                                     /* 86FC */
    LoadY16(cpu, cpu->x);
    do {
        LoadAAbsolute8(memory, cpu, 0x1587u, cpu->y);          /* 8700 */
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, LongIndexedAddress(0x868726u, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x1587u, cpu->y);
        LoadAAbsolute8(memory, cpu, 0x1597u, cpu->y);
        Adc8(cpu, Read8(memory, LongIndexedAddress(0x868727u, cpu->x)));
        StoreAAbsolute8(memory, cpu, 0x1597u, cpu->y);
        LoadAAbsolute8(memory, cpu, 0x15a7u, cpu->y);
        Adc8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, 0x15a7u, cpu->y);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, 0x000du);
    } while (!cpu->zero);
}

/* (dp) in the data bank. */
static uint32_t TitleDirectPointer(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint8_t offset) {
    return (((uint32_t)cpu->data_bank << 16) +
        Read16Direct(memory, cpu, offset)) & 0x00ffffffu;
}

/* $86:8695: when layer 10 crosses 8 pixels, the next of 64 frames:
   30 words from $7E:B3C4 + 2n (stride $80) to $7E:C3C0; $1566 = 2. */
static void TitleTileAnimation(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x15a1u, 0);                   /* 8695 */
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, 0x15b7u, 0));
    if (cpu->zero)
        return;
    StoreAAbsolute8(memory, cpu, 0x15b7u, 0);
    StepMemory8(memory, cpu, AbsoluteIndexedAddress(cpu, 0x15b8u, 0), 1);
    LoadAAbsolute8(memory, cpu, 0x15b8u, 0);
    Compare8(cpu, A8(cpu), 0x40u);
    if (cpu->zero)
        StoreZeroAbsolute8(memory, cpu, 0x15b8u, 0);
    PushDataBank(memory, cpu);                                 /* 86B3 */
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x15b8u, 0));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0xb3c4u);
    StoreADirect16(memory, cpu, 0x19u);
    LoadA16(cpu, 0xc3c0u);
    StoreADirect16(memory, cpu, 0x11u);
    LoadY16(cpu, 0x001eu);
    do {
        LoadA16(cpu, Read16Long(memory, TitleDirectPointer(memory, cpu, 0x19u)));
        Write16Long(memory, TitleDirectPointer(memory, cpu, 0x11u),
            cpu->accumulator);
        Increment16Direct(memory, cpu, 0x11u);
        Increment16Direct(memory, cpu, 0x11u);
        LoadADirect16(memory, cpu, 0x19u);
        cpu->carry = 0;
        Add16Value(cpu, 0x0080u);
        StoreADirect16(memory, cpu, 0x19u);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x02u);
    StoreAAbsolute8(memory, cpu, 0x1566u, 0);
    PullDataBank(memory, cpu);
}

/* $86:86ED: title layer scroll and tile animation; $1566 bit 0. */
Lufia2ExecutionResult Lufia2TitleLayers(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x8686edu);
    SimulateJsrFrame(memory, cpu, 0x86efu);                    /* 86ED */
    TitleLayerScroll(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0x86f2u);
    TitleTileAnimation(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x1566u, 0);                   /* 86F3 */
    Or8(cpu, 0x01u);
    StoreAAbsolute8(memory, cpu, 0x1566u, 0);
    return ExecutionReturned(0x8686fbu);                       /* RTS */
}

/* $86:8996: every 3rd frame rotate colours 1-8 of four palette
   rows ($04A0/$04C0/$04E0/$0500), step $15B9 down mod 8, $73 = $80. */
Lufia2ExecutionResult Lufia2TitlePaletteCycle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t rows[4] = {0x04a0u, 0x04c0u, 0x04e0u, 0x0500u};
    unsigned row;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868996u);
    SetAccumulatorWidth(cpu, 0);                               /* 8996 */
    SetIndexWidth(cpu, 0);
    StepAbsolute16(memory, cpu, 0x14b5u, -1);
    if (cpu->zero) {
        LoadA16(cpu, 0x0003u);
        StoreAAbsolute16(memory, cpu, 0x14b5u, 0);
        for (row = 0; row < 4u; ++row) {                       /* 89A3 */
            LoadA16(cpu, Read16AbsoluteIndexed(
                memory, cpu, (uint16_t)(rows[row] + 2u), 0));
            PushAccumulator16(memory, cpu);
        }
        LoadY16(cpu, 0x0002u);
        do {
            for (row = 0; row < 4u; ++row) {                   /* 89B6 */
                LoadA16(cpu, Read16AbsoluteIndexed(
                    memory, cpu, (uint16_t)(rows[row] + 2u), cpu->y));
                StoreAAbsolute16(memory, cpu, rows[row], cpu->y);
            }
            IncrementY16(cpu);
            IncrementY16(cpu);
            Compare16(cpu, cpu->y, 0x0010u);
        } while (!cpu->zero);
        for (row = 4; row-- > 0;) {                            /* 89D5 */
            PullAccumulator16(memory, cpu);
            StoreAAbsolute16(memory, cpu, rows[row], cpu->y);
        }
        SetAccumulatorWidth(cpu, 1);
        StepMemory8(memory, cpu, AbsoluteIndexedAddress(cpu, 0x15b9u, 0), -1);
        LoadAAbsolute8(memory, cpu, 0x15b9u, 0);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero) {
            LoadA8(cpu, 0x07u);
            StoreAAbsolute8(memory, cpu, 0x15b9u, 0);
        }
        LoadA8(cpu, 0x80u);                                    /* 89F6 */
        StoreADirect8(memory, cpu, 0x73u);
    }
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x8689fcu);                       /* RTS */
}
