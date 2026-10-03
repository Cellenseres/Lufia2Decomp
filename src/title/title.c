/* Intro logos and title state flow. */

#include <stdbool.h>

#include "core/cpu_internal.h"
#include "lufia2/title.h"
#include "system/system_internal.h"
#include "system/wram.h"

/* Title screen state. The three title objects at $C400/$C418/$C430 move on
 * ellipses and shed particles; their records and the particle records share
 * a 13 byte layout for the sprite fields. */
enum {
    TITLE_OBJ_CENTER_X = 0x00u,
    TITLE_OBJ_CENTER_Y = 0x02u,
    TITLE_OBJ_ANGLE_X = 0x04u,
    TITLE_OBJ_ANGLE_Y = 0x06u,
    TITLE_OBJ_TIMER = 0x08u,
    TITLE_OBJ_RADIUS_X = 0x0au, /* 8.8 fixed point, as is the Y radius */
    TITLE_OBJ_RADIUS_Y = 0x0cu,
    TITLE_OBJ_RADIUS_X_STEP = 0x0eu,
    TITLE_OBJ_RADIUS_Y_STEP = 0x10u,
    TITLE_OBJ_FLAGS = 0x12u, /* bit 7: active; bits 0 and 1: angle direction */
    TITLE_OBJ_ANGLE_X_STEP = 0x13u,
    TITLE_OBJ_ANGLE_Y_STEP = 0x15u,
    TITLE_OBJ_DEPTH = 0x17u,
    TITLE_FLAG_ANGLE_X_BACK = 0x0001u,
    TITLE_FLAG_ANGLE_Y_BACK = 0x0002u,
    TITLE_ANGLE_WRAP = 0xb400u,
    /* Sprite and particle records. */
    TITLE_SPRITE_ACTIVE = 0x01u,
    TITLE_PARTICLE_FRAME = 0x02u,
    TITLE_SPRITE_DATA = 0x03u, /* word: pointer to the OAM layout */
    TITLE_SPRITE_X = 0x05u,
    TITLE_SPRITE_Y = 0x07u,
    TITLE_PARTICLE_LIFETIME = 0x09u,
    TITLE_PARTICLE_DELAY = 0x0au,
    TITLE_PARTICLE_PALETTE = 0x0bu,
    TITLE_PARTICLE_ATTRIBUTES = 0x0cu,
    TITLE_PARTICLE_SIZE = 0x0du,
    TITLE_DEPTH_SPRITES = 0x868ac0u,
    TITLE_DEPTH_SPEEDS = 0x868a71u,
    TITLE_PARTICLE_FRAMES = 0x868a87u,
    /* Background layers. */
    TITLE_SCROLL_FRACTION = 0x1587u, /* 24 bit positions, one byte per array */
    TITLE_SCROLL_LOW = 0x1597u,
    TITLE_SCROLL_HIGH = 0x15a7u,
    TITLE_SCROLL_SPEEDS = 0x868726u,
    TITLE_LAYER_COUNT = 0x000du,
    TITLE_SCROLL_COUNTER = 0x15a1u,
    TITLE_ANIM_LAST = 0x15b7u,
    TITLE_ANIM_FRAME = 0x15b8u,
    TITLE_ANIM_FRAMES = 0x40u,
    TITLE_ANIM_SOURCE = 0xb3c4u,
    TITLE_ANIM_DESTINATION = 0xc3c0u,
    TITLE_ANIM_ROWS = 0x001eu,
    TITLE_ANIM_ROW_STRIDE = 0x0080u,
    TITLE_UPDATE_FLAGS = 0x1566u, /* bit 0: scrolled, bit 1: tiles changed */
    TITLE_PALETTE_TIMER = 0x14b5u,
    TITLE_PALETTE_PHASE = 0x15b9u,
};

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
/* Intro state machine ($80:92A4). Direct page $50 is the state, an index into
 * the handler table at $80:92B7, and $4E counts frames within a state. */
enum {
    INTRO_DP_FRAME = 0x4eu,
    INTRO_DP_STATE = 0x50u,
    INTRO_DP_FINISHED = 0x6au, /* cleared by the last state */
    INTRO_HANDLER_TABLE = 0x92b7u,
    INTRO_HANDLER_SCROLL_ROWS = 0x92cbu,
    INTRO_HANDLER_FIRST_LOGO = 0x92feu,
    INTRO_HANDLER_FADE_IN = 0x930cu,
    INTRO_HANDLER_HOLD = 0x9320u,
    INTRO_HANDLER_FADE_OUT = 0x9330u,
    INTRO_HANDLER_SECOND_LOGO = 0x9346u,
    INTRO_HANDLER_FINISH = 0x9354u,
    INTRO_FADE_FRAMES = 0x20u,
    INTRO_HOLD_FRAMES = 0x78u,
    INTRO_FIRST_LOGO = 0x2000u, /* $7E source of the logo tiles */
    INTRO_SECOND_LOGO = 0x2800u,
    INTRO_LOGO_BYTES = 0x0700u,
    INTRO_VRAM_ROWS = 0x4000u,    /* VRAM word address of the scroll rows */
    INTRO_ROW_SOURCE = 0x7e4004u, /* words: source offset and byte count */
    INTRO_ROW_COUNT = 0x7e4006u,
    DMA_CHANNEL_6 = 0x40u, /* MDMAEN bit */
    DMA_WORD_TO_VRAM = 0x01u,
    DMA_VRAM_DATA_PORT = 0x18u,
};

/* $80:9357: DMA $700 bytes from $7E:X to VRAM word address Y on channel 6. */
static void IntroVramDma(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreWordAbsolute(memory, cpu, SNES_A1TL(6), cpu->x);      /* 9357 */
    StoreWordAbsolute(memory, cpu, SNES_VMADDL, cpu->y);
    StoreAImmediate8(memory, cpu, 0x7eu, SNES_A1B(6));
    LoadX16(cpu, INTRO_LOGO_BYTES);
    StoreWordAbsolute(memory, cpu, SNES_DASL(6), cpu->x);
    StoreAImmediate8(memory, cpu, DMA_WORD_TO_VRAM, SNES_DMAP(6));
    StoreAImmediate8(memory, cpu, DMA_VRAM_DATA_PORT, SNES_BBAD(6));
    StoreAImmediate8(memory, cpu, DMA_CHANNEL_6, SNES_MDMAEN);
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
    IncrementDirect8(memory, cpu, INTRO_DP_STATE);
    Write8(memory, DirectAddress(cpu, INTRO_DP_FRAME), 0x00u);
}

/* State $92CB: DMA the scroll rows at $7E:4004 plus $4000 to VRAM. */
static void IntroScrollRows(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, INTRO_ROW_SOURCE));
    cpu->carry = 0;
    Add16Value(cpu, INTRO_VRAM_ROWS);
    StoreWordAbsolute(memory, cpu, SNES_A1TL(6), cpu->accumulator);
    LoadA16(cpu, Read16Long(memory, INTRO_ROW_COUNT));
    StoreWordAbsolute(memory, cpu, SNES_DASL(6), cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadX16(cpu, INTRO_VRAM_ROWS);
    StoreWordAbsolute(memory, cpu, SNES_VMADDL, cpu->x);
    StoreAImmediate8(memory, cpu, 0x7eu, SNES_A1B(6));
    StoreAImmediate8(memory, cpu, DMA_WORD_TO_VRAM, SNES_DMAP(6));
    StoreAImmediate8(memory, cpu, DMA_VRAM_DATA_PORT, SNES_BBAD(6));
    StoreAImmediate8(memory, cpu, DMA_CHANNEL_6, SNES_MDMAEN);
    IncrementDirect8(memory, cpu, INTRO_DP_STATE);
}

/* State $930C: brightness follows half the frame count for 32 frames. */
static void IntroFadeIn(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, DirectByte(memory, cpu, INTRO_DP_FRAME));
    LsrA8(cpu);
    StoreAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
    LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, INTRO_DP_FRAME) + 1u));
    StoreADirect8(memory, cpu, INTRO_DP_FRAME);
    Compare8(cpu, A8(cpu), INTRO_FADE_FRAMES);
    if (cpu->carry) {
        IncrementDirect8(memory, cpu, INTRO_DP_STATE);
        Write8(memory, DirectAddress(cpu, INTRO_DP_FRAME), 0x00u);
    }
}

/* State $9320: hold the logo for 120 frames; the fade-out then starts from
 * 32. */
static void IntroHold(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, INTRO_DP_FRAME) + 1u));
    StoreADirect8(memory, cpu, INTRO_DP_FRAME);
    Compare8(cpu, A8(cpu), INTRO_HOLD_FRAMES);
    if (cpu->carry) {
        LoadA8(cpu, INTRO_FADE_FRAMES);
        StoreADirect8(memory, cpu, INTRO_DP_FRAME);
        IncrementDirect8(memory, cpu, INTRO_DP_STATE);
    }
}

/* State $9330: brightness follows half the frame count down to zero, then
 * the screen is blanked. */
static void IntroFadeOut(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, (uint8_t)(DirectByte(memory, cpu, INTRO_DP_FRAME) - 1u));
    StoreADirect8(memory, cpu, INTRO_DP_FRAME);
    if (cpu->negative) {
        Write8(memory, DirectAddress(cpu, INTRO_DP_FRAME), 0x00u);
        IncrementDirect8(memory, cpu, INTRO_DP_STATE);
        StoreAImmediate8(memory, cpu, BRIGHTNESS_FORCED_BLANK, WRAM_BRIGHTNESS);
    } else {
        LsrA8(cpu);
        StoreAAbsolute8(memory, cpu, WRAM_BRIGHTNESS, 0);
    }
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
    LoadA8(cpu, DirectByte(memory, cpu, INTRO_DP_STATE));
    AslA8(cpu);
    TransferAToX(cpu);
    handler =
        (uint16_t)(Read8(memory, ProgramAddress(
                                     cpu, (uint16_t)(INTRO_HANDLER_TABLE + cpu->x))) |
                   (Read8(memory, ProgramAddress(cpu, (uint16_t)(INTRO_HANDLER_TABLE +
                                                                 1u + cpu->x)))
                    << 8));
    if (handler != INTRO_HANDLER_SCROLL_ROWS && handler != INTRO_HANDLER_FIRST_LOGO &&
        handler != INTRO_HANDLER_FADE_IN && handler != INTRO_HANDLER_HOLD &&
        handler != INTRO_HANDLER_FADE_OUT && handler != INTRO_HANDLER_SECOND_LOGO &&
        handler != INTRO_HANDLER_FINISH)
        return ExecutionHandoff(cpu, 0x8092b1u);
    SimulateJsrFrame(memory, cpu, 0x92b3u);
    switch (handler) {
    case INTRO_HANDLER_SCROLL_ROWS:
        IntroScrollRows(memory, cpu);
        break;
    case INTRO_HANDLER_FIRST_LOGO:
        IntroLogoUpload(memory, cpu, INTRO_FIRST_LOGO, 0x9306u);
        break;
    case INTRO_HANDLER_FADE_IN:
        IntroFadeIn(memory, cpu);
        break;
    case INTRO_HANDLER_HOLD:
        IntroHold(memory, cpu);
        break;
    case INTRO_HANDLER_FADE_OUT:
        IntroFadeOut(memory, cpu);
        break;
    case INTRO_HANDLER_SECOND_LOGO:
        IntroLogoUpload(memory, cpu, INTRO_SECOND_LOGO, 0x934eu);
        break;
    default:                                       /* 9354 */
        Write8(memory, DirectAddress(cpu, INTRO_DP_FINISHED), 0x00u);
        break;
    }
    SimulateRtsFrame(memory, cpu);
    PullDataBank(memory, cpu);                                 /* 92B4 */
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8092b6u);
}

/* $86:88E5: OAM entry of title particle X. */
static void TitleParticleSprite(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* 88E5 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_SPRITE_DATA, cpu->x));
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
    LoadAAbsolute8(memory, cpu, TITLE_SPRITE_X, cpu->x); /* OAM x */
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x05u));
    StoreAAbsolute8(memory, cpu, 0x0100u, cpu->y);
    IncrementY16(cpu);
    LoadAAbsolute8(memory, cpu, TITLE_SPRITE_Y, cpu->x); /* OAM y */
    cpu->carry = 1;
    Sbc8(cpu, DirectByte(memory, cpu, 0x04u));
    StoreAAbsolute8(memory, cpu, 0x0100u, cpu->y);
    IncrementY16(cpu);
    StoreYDirect16(memory, cpu, 0x06u);
    cpu->y = PullIndexValue(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* 8921 */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_PARTICLE_PALETTE, cpu->x));
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
        LoadAAbsolute8(memory, cpu, TITLE_SPRITE_X, cpu->x); /* x - offset */
        cpu->carry = 1;
        Sbc8(cpu, DirectByte(memory, cpu, 0x05u));
        LoadAAbsolute8(memory, cpu, (TITLE_SPRITE_X + 1u), cpu->x);
        Sbc8(cpu, 0x00u);
    } else {
        LoadA8(cpu, DirectByte(memory, cpu, 0x05u));           /* 8969 */
        LoadA8(cpu, (uint8_t)~A8(cpu));                        /* EOR #$FF */
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        cpu->carry = 0;
        Adc8(cpu, AbsoluteByte(memory, cpu, TITLE_SPRITE_X, cpu->x));
        LoadAAbsolute8(memory, cpu, (TITLE_SPRITE_X + 1u), cpu->x);
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

/* $86:88BE: OAM entries for the 75 title particles. */
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
        LoadAAbsolute8(memory, cpu, TITLE_SPRITE_ACTIVE, cpu->x); /* 88CC */
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
        Add16Value(cpu, TITLE_PARTICLE_SIZE);
        TransferAToX(cpu);
        SetAccumulatorWidth(cpu, 1);
        DecrementDirect8(memory, cpu, 0x15u);
    } while (!cpu->zero);
    return ExecutionReturned(0x8688e4u);                       /* RTS */
}

/* Working registers of the object being stepped. */
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
    Compare16(cpu, cpu->accumulator, TITLE_ANGLE_WRAP);
    if (!cpu->carry)
        return;
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_OBJ_FLAGS, cpu->y));
    And16(cpu, flag);
    if (!cpu->zero) {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, angle, cpu->y));
        cpu->carry = 0;
        Add16Value(cpu, TITLE_ANGLE_WRAP);
    } else {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, angle, cpu->y));
        Subtract16(cpu, TITLE_ANGLE_WRAP);
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
    StoreAAbsolute16(memory, cpu, WRAM_SYSTEM_MULTIPLY_A, 0);
}

/* $82:8000 then $86:8503: $1575 = signed magnitude * factor. */
static void TitleSignedProduct(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t multiply_return,
    uint16_t sign_return) {
    StoreAAbsolute16(memory, cpu, WRAM_SYSTEM_MULTIPLY_B, 0);
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

/* $86:83F2: move on the ellipse; returns the depth step. */
static void TitleObjectProject(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* 83F2 */
    LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));
    TitleAccumulate(memory, cpu, TITLE_OBJ_RADIUS_X, TITLE_OBJ_RADIUS_X_STEP);
    TitleAccumulate(memory, cpu, TITLE_OBJ_RADIUS_Y, TITLE_OBJ_RADIUS_Y_STEP);
    TitleAccumulate(memory, cpu, TITLE_OBJ_ANGLE_X, TITLE_OBJ_ANGLE_X_STEP);
    TitleWrapAngle(memory, cpu, TITLE_OBJ_ANGLE_X, TITLE_FLAG_ANGLE_X_BACK);
    SetAccumulatorWidth(cpu, 1);                               /* 843B */
    LoadAAbsolute8(memory, cpu, TITLE_OBJ_ANGLE_X + 1u, cpu->y);
    Lufia2CallCosine(memory, cpu, 0x86u, 0x8443u);
    TitleTrigMagnitude(memory, cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_OBJ_RADIUS_X + 1u, cpu->y));
    And16(cpu, 0x00ffu);
    TitleSignedProduct(memory, cpu, 0x845cu, 0x845fu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_OBJ_CENTER_X, cpu->y));
    cpu->carry = 0;
    Add16Value(cpu, TitleWord(memory, cpu, TITLE_PRODUCT));    /* sprite x */
    StoreAAbsolute16(memory, cpu, TITLE_SPRITE_X, cpu->x);
    SetAccumulatorWidth(cpu, 1);                               /* 846A */
    LoadAAbsolute8(memory, cpu, TITLE_OBJ_ANGLE_X + 1u, cpu->y);
    Lufia2CallSine(memory, cpu, 0x86u, 0x8472u);
    TitleTrigMagnitude(memory, cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_OBJ_RADIUS_Y + 1u, cpu->y));
    And16(cpu, 0x00ffu);
    TitleSignedProduct(memory, cpu, 0x848bu, 0x848eu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_OBJ_CENTER_Y, cpu->y));
    Subtract16(cpu, TitleWord(memory, cpu, TITLE_PRODUCT));    /* sprite y */
    StoreAAbsolute16(memory, cpu, TITLE_SPRITE_Y, cpu->x);
    TitleAccumulate(memory, cpu, TITLE_OBJ_ANGLE_Y, TITLE_OBJ_ANGLE_Y_STEP); /* 8499 */
    TitleWrapAngle(memory, cpu, TITLE_OBJ_ANGLE_Y, TITLE_FLAG_ANGLE_Y_BACK);
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
    LoadAAbsolute8(memory, cpu, TITLE_OBJ_DEPTH, cpu->x);
    AslA8(cpu);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(TITLE_DEPTH_SPRITES, cpu->x)));
    StoreAAbsolute8(memory, cpu, TITLE_SPRITE_DATA, cpu->y);
    LoadA8(cpu, Read8(memory, LongIndexedAddress((TITLE_DEPTH_SPRITES + 1u), cpu->x)));
    StoreAAbsolute8(memory, cpu, (TITLE_SPRITE_DATA + 1u), cpu->y);
}

/* $86:8530: angle steps for the depth. */
static void TitleObjectSpeed(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));                       /* 8530 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_OBJ_DEPTH, cpu->y));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(TITLE_DEPTH_SPEEDS, cpu->x)));
    StoreAAbsolute16(memory, cpu, TITLE_OBJ_ANGLE_X_STEP, cpu->y);
    LsrA16(cpu);
    StoreAAbsolute16(memory, cpu, TITLE_OBJ_ANGLE_Y_STEP, cpu->y);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_OBJ_FLAGS, cpu->y));
    cpu->zero = (cpu->accumulator & TITLE_FLAG_ANGLE_X_BACK) == 0;
    if (!cpu->zero) {
        LoadA16(cpu, (uint16_t)~Read16AbsoluteIndexed(memory, cpu,
                                                      TITLE_OBJ_ANGLE_X_STEP, cpu->y));
        IncrementA16(cpu);
        StoreAAbsolute16(memory, cpu, TITLE_OBJ_ANGLE_X_STEP, cpu->y);
    }
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_OBJ_FLAGS, cpu->y));
    cpu->zero = (cpu->accumulator & TITLE_FLAG_ANGLE_Y_BACK) == 0;
    if (!cpu->zero) {
        LoadA16(cpu, (uint16_t)~Read16AbsoluteIndexed(memory, cpu,
                                                      TITLE_OBJ_ANGLE_Y_STEP, cpu->y));
        IncrementA16(cpu);
        StoreAAbsolute16(memory, cpu, TITLE_OBJ_ANGLE_Y_STEP, cpu->y);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $86:83C3: move the object while its timer runs. */
static void TitleObjectStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);                               /* 83C3 */
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_OBJ_TIMER, cpu->y));
    if (cpu->zero) {
        SetAccumulatorWidth(cpu, 1);                           /* 83E9 */
        LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
        StoreZeroAbsolute8(memory, cpu, TITLE_SPRITE_ACTIVE, cpu->x);
        return;
    }
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    StoreAAbsolute16(memory, cpu, TITLE_OBJ_TIMER, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    SimulateJsrFrame(memory, cpu, 0x83d5u);
    TitleObjectProject(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));        /* 83D6 */
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, TITLE_OBJ_DEPTH, cpu->y));
    if (cpu->zero)
        return;
    StoreAAbsolute8(memory, cpu, TITLE_OBJ_DEPTH, cpu->y);
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
    Add16Value(cpu, TITLE_PARTICLE_SIZE);
    TransferAToY(cpu);
    SetAccumulatorWidth(cpu, 1);
}

/* $86:85EB: fill the free particle slot at Y from the object and its sprite. */
static void TitleStartParticle(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x01u); /* 85EB */
    StoreAAbsolute8(memory, cpu, TITLE_SPRITE_ACTIVE, cpu->y);
    LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT));
    LoadAAbsolute8(memory, cpu, TITLE_OBJ_DEPTH, cpu->x);
    StoreAAbsolute8(memory, cpu, TITLE_PARTICLE_FRAME, cpu->y);
    SetAccumulatorWidth(cpu, 0);
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(TITLE_PARTICLE_FRAMES, cpu->x)));
    StoreAAbsolute16(memory, cpu, TITLE_SPRITE_DATA, cpu->y);
    LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_SPRITE_X, cpu->x));
    StoreAAbsolute16(memory, cpu, TITLE_SPRITE_X, cpu->y);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_SPRITE_Y, cpu->x));
    StoreAAbsolute16(memory, cpu, TITLE_SPRITE_Y, cpu->y);
    SetAccumulatorWidth(cpu, 1);
    LoadAAbsolute8(memory, cpu, TITLE_PALETTE_PHASE, 0); /* palette */
    AslA8(cpu);
    StoreAAbsolute8(memory, cpu, TITLE_PARTICLE_PALETTE, cpu->y);
    LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
    LoadAAbsolute8(memory, cpu, TITLE_PARTICLE_ATTRIBUTES, cpu->x);
    StoreAAbsolute8(memory, cpu, TITLE_PARTICLE_ATTRIBUTES, cpu->y);
    LoadA8(cpu, 0x02u);
    StoreAAbsolute8(memory, cpu, TITLE_PARTICLE_DELAY, cpu->y); /* frame delay */
    LoadA8(cpu, 0x18u);
    cpu->carry = 1;
    Sbc8(cpu, 0x03u);
    StoreAAbsolute8(memory, cpu, TITLE_PARTICLE_LIFETIME, cpu->y); /* lifetime */
}

/* $86:85C2: start one particle at the sprite. */
static void TitleObjectSpawn(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));                       /* 85C2 */
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_SPRITE));
    LoadAAbsolute8(memory, cpu, TITLE_SPRITE_ACTIVE, cpu->x);
    if (!cpu->zero) {
        bool free_slot = true;

        LoadY16(cpu, TitleWord(memory, cpu, TITLE_OBJECT_PARTICLES));
        LoadA8(cpu, TITLE_PARTICLE_SLOTS);
        StoreADirect8(memory, cpu, 0x04u);
        for (;;) {
            LoadAAbsolute8(memory, cpu, TITLE_SPRITE_ACTIVE, cpu->y); /* 85D6 */
            if (cpu->zero)
                break;
            TitleNextParticle(cpu);
            DecrementDirect8(memory, cpu, 0x04u);
            if (cpu->zero) {
                free_slot = false;
                break;
            }
        }
        if (free_slot)
            TitleStartParticle(memory, cpu);
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $86:856F: particle animation. */
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
        LoadAAbsolute8(memory, cpu, TITLE_SPRITE_ACTIVE, cpu->y); /* 857B */
        if (!cpu->zero) {
            TransferYToX(cpu);
            StepMemory8(memory, cpu,
                        AbsoluteIndexedAddress(cpu, TITLE_PARTICLE_DELAY, cpu->x), -1);
            if (cpu->zero) {
                LoadA8(cpu, 0x00u);
                ExchangeAccumulatorBytes(cpu);
                LoadAAbsolute8(memory, cpu, TITLE_PARTICLE_FRAME, cpu->y);
                Compare8(cpu, A8(cpu), 0x0au);
                if (!cpu->zero) {
                    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
                    StoreAAbsolute8(memory, cpu, TITLE_PARTICLE_FRAME, cpu->y);
                    AslA8(cpu);
                    TransferAToX(cpu);
                    LoadA8(cpu, Read8(memory, LongIndexedAddress(TITLE_PARTICLE_FRAMES,
                                                                 cpu->x)));
                    StoreAAbsolute8(memory, cpu, TITLE_SPRITE_DATA, cpu->y);
                    LoadA8(cpu,
                           Read8(memory, LongIndexedAddress(
                                             (TITLE_PARTICLE_FRAMES + 1u), cpu->x)));
                    StoreAAbsolute8(memory, cpu, (TITLE_SPRITE_DATA + 1u), cpu->y);
                    LoadA8(cpu, 0x02u);
                    StoreAAbsolute8(memory, cpu, TITLE_PARTICLE_DELAY, cpu->y);
                }
            }
            TransferYToX(cpu);                                 /* 85A9 */
            StepMemory8(memory, cpu,
                        AbsoluteIndexedAddress(cpu, TITLE_PARTICLE_LIFETIME, cpu->x),
                        -1);
            if (cpu->zero)
                StoreZeroAbsolute8(memory, cpu, TITLE_SPRITE_ACTIVE, cpu->x);
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

/* $86:86FC: step the 13 layer positions. */
static void TitleLayerScroll(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadX16(cpu, 0x0000u);                                     /* 86FC */
    LoadY16(cpu, cpu->x);
    do {
        LoadAAbsolute8(memory, cpu, TITLE_SCROLL_FRACTION, cpu->y); /* 8700 */
        cpu->carry = 0;
        Adc8(cpu, Read8(memory, LongIndexedAddress(TITLE_SCROLL_SPEEDS, cpu->x)));
        StoreAAbsolute8(memory, cpu, TITLE_SCROLL_FRACTION, cpu->y);
        LoadAAbsolute8(memory, cpu, TITLE_SCROLL_LOW, cpu->y);
        Adc8(cpu,
             Read8(memory, LongIndexedAddress((TITLE_SCROLL_SPEEDS + 1u), cpu->x)));
        StoreAAbsolute8(memory, cpu, TITLE_SCROLL_LOW, cpu->y);
        LoadAAbsolute8(memory, cpu, TITLE_SCROLL_HIGH, cpu->y);
        Adc8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, TITLE_SCROLL_HIGH, cpu->y);
        IncrementX16(cpu);
        IncrementX16(cpu);
        IncrementY16(cpu);
        Compare16(cpu, cpu->y, TITLE_LAYER_COUNT);
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

/* $86:8695: next background frame every 8 pixels. */
static void TitleTileAnimation(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, TITLE_SCROLL_COUNTER, 0); /* 8695 */
    LsrA8(cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    Compare8(cpu, A8(cpu), AbsoluteByte(memory, cpu, TITLE_ANIM_LAST, 0));
    if (cpu->zero)
        return;
    StoreAAbsolute8(memory, cpu, TITLE_ANIM_LAST, 0);
    StepMemory8(memory, cpu, AbsoluteIndexedAddress(cpu, TITLE_ANIM_FRAME, 0), 1);
    LoadAAbsolute8(memory, cpu, TITLE_ANIM_FRAME, 0);
    Compare8(cpu, A8(cpu), TITLE_ANIM_FRAMES);
    if (cpu->zero)
        StoreZeroAbsolute8(memory, cpu, TITLE_ANIM_FRAME, 0);
    PushDataBank(memory, cpu);                                 /* 86B3 */
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TITLE_ANIM_FRAME, 0));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, TITLE_ANIM_SOURCE);
    StoreADirect16(memory, cpu, 0x19u);
    LoadA16(cpu, TITLE_ANIM_DESTINATION);
    StoreADirect16(memory, cpu, 0x11u);
    LoadY16(cpu, TITLE_ANIM_ROWS);
    do {
        LoadA16(cpu, Read16Long(memory, TitleDirectPointer(memory, cpu, 0x19u)));
        Write16Long(memory, TitleDirectPointer(memory, cpu, 0x11u),
            cpu->accumulator);
        Increment16Direct(memory, cpu, 0x11u);
        Increment16Direct(memory, cpu, 0x11u);
        LoadADirect16(memory, cpu, 0x19u);
        cpu->carry = 0;
        Add16Value(cpu, TITLE_ANIM_ROW_STRIDE);
        StoreADirect16(memory, cpu, 0x19u);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x02u);
    StoreAAbsolute8(memory, cpu, TITLE_UPDATE_FLAGS, 0);
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
    LoadAAbsolute8(memory, cpu, TITLE_UPDATE_FLAGS, 0); /* 86F3 */
    Or8(cpu, 0x01u);
    StoreAAbsolute8(memory, cpu, TITLE_UPDATE_FLAGS, 0);
    return ExecutionReturned(0x8686fbu);                       /* RTS */
}

/* $86:8996: rotate title palette colors 1-8. */
Lufia2ExecutionResult Lufia2TitlePaletteCycle(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    static const uint16_t rows[4] = {0x04a0u, 0x04c0u, 0x04e0u, 0x0500u};
    unsigned row;

    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x868996u);
    SetAccumulatorWidth(cpu, 0);                               /* 8996 */
    SetIndexWidth(cpu, 0);
    StepAbsolute16(memory, cpu, TITLE_PALETTE_TIMER, -1);
    if (cpu->zero) {
        LoadA16(cpu, 0x0003u);
        StoreAAbsolute16(memory, cpu, WRAM_MENU_LIST_TOP_ROW, 0);
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
        StepMemory8(memory, cpu, AbsoluteIndexedAddress(cpu, TITLE_PALETTE_PHASE, 0),
                    -1);
        LoadAAbsolute8(memory, cpu, TITLE_PALETTE_PHASE, 0);
        Compare8(cpu, A8(cpu), 0xffu);
        if (cpu->zero) {
            LoadA8(cpu, 0x07u);
            StoreAAbsolute8(memory, cpu, TITLE_PALETTE_PHASE, 0);
        }
        LoadA8(cpu, 0x80u);                                    /* 89F6 */
        StoreADirect8(memory, cpu, DP_NMI_UPLOAD_FLAGS);
    }
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x8689fcu);                       /* RTS */
}
