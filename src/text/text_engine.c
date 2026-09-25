/* Text and cutscene script engine ($80:9CB8). */

#include "core/cpu_internal.h"
#include "lufia2/text.h"
#include "text/text_internal.h"
#include "system/wram.h"

/* Text engine WRAM. */
#define TEXT_WINDOW_STATE 0x099cu          /* bit 0: window buffer in use */
#define TEXT_GLYPH_ATTRIBUTE 0x09adu
#define TEXT_GLYPH 0x09afu                 /* glyph or sub-script number */
#define TEXT_GLYPH_DESTINATION 0x09b1u     /* $7E tilemap buffer address */
#define TEXT_GLYPH_UPLOAD 0x09b5u          /* VRAM upload source */
#define TEXT_SCRIPT_POINTER 0x09b7u
#define TEXT_SCRIPT_BANK 0x09b9u
#define TEXT_PRINT_DELAY 0x0b52u
#define TEXT_LINE_START 0x1250u
#define TEXT_CALLER_POINTER 0x1252u        /* sub-script return */
#define TEXT_CALLER_BANK 0x1254u           /* zero: no caller */
#define TEXT_RETURN_POINTER 0x1257u
#define TEXT_RETURN_BANK 0x1259u           /* zero: nothing queued */
#define TEXT_RETURN_COUNT 0x125au
#define TEXT_TYPING_SOUND 0x1260u          /* zero: silent */
#define TEXT_PROMPT_TIMER 0x1265u
#define TEXT_WAIT_ACTOR 0x1269u            /* negative: none */
#define TEXT_TYPING_SOUND_PHASE 0x7fd0c0u  /* sound every other glyph */
#define TEXT_PRINT_COUNTDOWN 0x7fd0ffu

/* $80:C0B7: next text byte from DB:Y; Y past $FFFF moves the bank. */
static void TextNextByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);              /* C0B7 */
    IncrementY16(cpu);
    if (!cpu->negative) {
        PushAccumulator8(memory, cpu);                         /* C0BD */
        Push8(memory, cpu, PackStatus(cpu));
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJslFrame(memory, cpu, 0x80u, 0xbd3bu);
    Push8(memory, cpu, PackStatus(cpu));                       /* C7C2 */
    PushDataBank(memory, cpu);
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, TEXT_GLYPH_ATTRIBUTE, 0);
    TransferAToX(cpu);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80c815u, cpu->x)));
    StoreADirect8(memory, cpu, 0x57u);
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TEXT_GLYPH, 0));
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
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, TEXT_GLYPH_DESTINATION, 0));
        Write16Absolute(memory, cpu, TEXT_GLYPH_UPLOAD, cpu->y);
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    TextDrawGlyph(memory, cpu);                                /* BD38 */
    StoreAImmediate8(memory, cpu, 0x01u, SNES_DMAP(0));
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TEXT_GLYPH_UPLOAD, 0));
    Write16Absolute(memory, cpu, SNES_A1TL(0), cpu->accumulator);
    Subtract16(cpu, 0xd000u);
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x1800u);
    Write16Absolute(memory, cpu, 0x0079u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    StoreAImmediate8(memory, cpu, 0x7eu, SNES_A1B(0));
    LoadX16(cpu, 0x0020u);
    Write16Absolute(memory, cpu, SNES_DASL(0), cpu->x);
    StoreAImmediate8(memory, cpu, 0x18u, SNES_BBAD(0));
    LoadA8(cpu, 0x41u);
    StoreADirect8(memory, cpu, 0x75u);
    SimulateRtsFrame(memory, cpu);
}

/* $80:9DB0: PLP, PLB, RTL. */
static Lufia2ExecutionResult TextEngineExit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2ExecutionResult result) {
    UnpackStatus(cpu, Pull8(memory, cpu));                     /* 9DB0 */
    PullDataBank(memory, cpu);
    result.pc = 0x809db2u;
    return result;
}

/* $80:C0EC: previous text byte; Y below $8000 moves the bank back. */
static void TextPrevByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    cpu->y = (uint16_t)(cpu->y - 1u);                          /* C0EC */
    SetNz16(cpu, cpu->y);
    if (!cpu->negative) {
        PushAccumulator8(memory, cpu);                         /* C0EF */
        Push8(memory, cpu, PackStatus(cpu));
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
        DecrementA8(cpu);
        StoreAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    LoadAAbsolute8(memory, cpu, TEXT_WINDOW_STATE, 0);
    And8(cpu, 0xfeu);
    StoreAAbsolute8(memory, cpu, TEXT_WINDOW_STATE, 0);
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    LoadAAbsolute8(memory, cpu, TEXT_WINDOW_STATE, 0);         /* C1FD */
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StoreAAbsolute8(memory, cpu, 0x09b0u, 0);                  /* 9E54 */
    ExchangeAccumulatorBytes(cpu);
    StoreAAbsolute8(memory, cpu, TEXT_GLYPH, 0);
    Write16Absolute(memory, cpu, TEXT_CALLER_POINTER, cpu->y);
    LoadAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
    StoreAAbsolute8(memory, cpu, TEXT_CALLER_BANK, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TEXT_GLYPH, 0));
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x8eea00u, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0xea00u);
    Write16Absolute(memory, cpu, TEXT_SCRIPT_POINTER, cpu->accumulator);
    LoadY16(cpu, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x8eu);
    StoreAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
}

/* $80:9DDB: next text line, $1250 += $400. */
static void TextNewLine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    SetAccumulatorWidth(cpu, 0);                               /* 9DDB */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TEXT_LINE_START, 0));
    cpu->carry = 0;
    Add16Value(cpu, 0x0400u);
    Write16Absolute(memory, cpu, TEXT_LINE_START, cpu->accumulator);
    Write16Absolute(memory, cpu, TEXT_GLYPH_DESTINATION, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x10u);
    TestBitsAbsolute8(memory, cpu, TEXT_WINDOW_STATE, 1);
    TransferDirectToA(cpu);
    Write8(memory, TEXT_TYPING_SOUND_PHASE, A8(cpu));
    StoreZeroAbsolute8(memory, cpu, 0x09b3u, 0);
    SimulateRtsFrame(memory, cpu);
}

/* $80:BE30: flag A to byte $56 and mask $57; X 8-bit. */
static void TextFlagBit(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TextNextWord(memory, cpu, 0xa3c8u);                        /* A3C6 */
    SetAccumulatorWidth(cpu, 0);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x09a0u, 0));
    Write16Absolute(memory, cpu, TEXT_SCRIPT_BANK, cpu->accumulator);
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
        LoadAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
        LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
        StoreAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
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
    {0xb7e6u, 0x1255u},               /* $47 */
    {0xb837u, TEXT_PROMPT_TIMER},                              /* $49 prompt timer */
    {0xb840u, TEXT_GLYPH_ATTRIBUTE},                           /* $4A glyph attribute */
    {0xb900u, TEXT_TYPING_SOUND},                              /* $52 typing sound */
    {0xbd6cu, 0x09dfu},               /* $7C */
    {0xbd75u, 0x09eeu},               /* $7D */
    {0xbd7eu, 0x09e0u},               /* $7E */
    {0xbd87u, 0x09e1u},               /* $7F */
    {0xbd90u, 0x09e2u},               /* $80 */
    {0xb696u, TEXT_PRINT_DELAY},                               /* $CB print delay */
    {0xb2a7u, 0x7ff8a0u}};            /* $74 */

/* $80:BF12/$80:BF43: gold $0A8A-$0A8C +/- A, capped/undone. */
static void TextGold(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint8_t add,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    if (add) {
        cpu->carry = 0;                                        /* BF15 */
        Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_GOLD, 0));
        Write16Absolute(memory, cpu, WRAM_GOLD, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x0a8cu, 0);
        Adc8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, 0x0a8cu, 0);
        Compare8(cpu, A8(cpu), 0x98u);
        if (cpu->carry) {
            SetAccumulatorWidth(cpu, 0);
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_GOLD, 0));
            Compare16(cpu, cpu->accumulator, 0x967fu);
            if (cpu->carry) {
                LoadA16(cpu, 0x967fu);                         /* 9,999,999 */
                Write16Absolute(memory, cpu, WRAM_GOLD, cpu->accumulator);
                SetAccumulatorWidth(cpu, 1);
                StoreAImmediate8(memory, cpu, 0x98u, 0x0a8cu);
            }
        }
    } else {
        Write16Direct(memory, cpu, 0x54u, cpu->accumulator);   /* BF46 */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_GOLD, 0));
        cpu->carry = 1;
        Add16Value(cpu, (uint16_t)~Read16Direct(memory, cpu, 0x54u));
        Write16Absolute(memory, cpu, WRAM_GOLD, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, 0x0a8cu, 0);
        Sbc8(cpu, 0x00u);
        StoreAAbsolute8(memory, cpu, 0x0a8cu, 0);
        if (!cpu->carry) {
            uint8_t high;

            SetAccumulatorWidth(cpu, 0);                       /* undo */
            LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, WRAM_GOLD, 0));
            cpu->carry = 0;
            Add16Value(cpu, Read16Direct(memory, cpu, 0x54u));
            Write16Absolute(memory, cpu, WRAM_GOLD, cpu->accumulator);
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t return_address) {
    SimulateJsrFrame(memory, cpu, return_address);
    StoreAAbsolute8(memory, cpu, SNES_CGADSUB, 0);             /* AF77 */
    StoreAImmediate8(memory, cpu, 0xe0u, SNES_COLDATA);
    TextNextByte(memory, cpu, 0xaf81u);
    StoreAAbsolute8(memory, cpu, 0x1286u, 0);
    StoreAAbsolute8(memory, cpu, SNES_COLDATA, 0);
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
    StoreZeroAbsolute8(memory, cpu, SNES_WRDIVL, 0);
    StoreAAbsolute8(memory, cpu, SNES_WRDIVH, 0);
    TextNextByte(memory, cpu, 0xafa8u);
    StoreAAbsolute8(memory, cpu, SNES_WRDIVB, 0);
    StoreAImmediate8(memory, cpu, 0x00u, SNES_CGWSEL);
    LoadA8(cpu, 0x80u);
    TestBitsAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 1);
    StoreZeroAbsolute8(memory, cpu, 0x1288u, 0);
    LoadAAbsolute8(memory, cpu, SNES_RDDIVH, 0);
    StoreAAbsolute8(memory, cpu, 0x1289u, 0);
    SimulateRtsFrame(memory, cpu);
}

/* What the engine does after an opcode. */
enum {
    TEXT_OPCODE_NEXT,
    TEXT_OPCODE_RELOAD,
    TEXT_OPCODE_EXIT,
    TEXT_OPCODE_HANDOFF
};

/* Handlers in the $80:CA14 opcode table. */
enum TextOpcodeHandler {
    TEXT_OP_END = 0x9d4c,                                      /* $00, $42 */
    TEXT_OP_NEW_LINE = 0x9dd5,                                 /* $03 */
    TEXT_OP_SUB_SCRIPT_05 = 0x9e45,                            /* $05 */
    TEXT_OP_SUB_SCRIPT_06 = 0x9e4e,                            /* $06 */
    TEXT_OP_PRINT_NAME = 0x9ed6,                               /* $09 */
    TEXT_OP_QUESTION_NEW_LINE = 0x9ef0,                        /* $0C */
    TEXT_OP_EXCLAMATION_NEW_LINE = 0x9ef4,                     /* $0D */
    TEXT_OP_COMMA_NEW_LINE = 0x9ef8,                           /* $0E */
    TEXT_OP_PERIOD_NEW_LINE = 0x9efc,                          /* $0F */
    TEXT_OP_GOTO_IF_FLAG = 0xa288,                             /* $15 */
    TEXT_OP_SET_FLAG = 0xa392,                                 /* $1A */
    TEXT_OP_CLEAR_FLAG = 0xa3ac,                               /* $1B */
    TEXT_OP_GOTO = 0xa3c6,                                     /* $1C */
    TEXT_OP_SET_VARIABLE = 0xa3df,                             /* $1D */
    TEXT_OP_ADD_VARIABLE = 0xa3ed,                             /* $1E */
    TEXT_OP_ADD_GOLD = 0xa480,                                 /* $22 */
    TEXT_OP_SUBTRACT_GOLD = 0xa4cb,                            /* $26 */
    TEXT_OP_SKIP_2 = 0xa4d4,                                   /* $27 */
    TEXT_OP_SKIP_3 = 0xa5bc,                                   /* $2A */
    TEXT_OP_WAIT_FOR_ACTOR = 0xa80f,                           /* $33 */
    TEXT_OP_CLEAR_D100 = 0xac18,                               /* $AA */
    TEXT_OP_B5 = 0xadd1,                                       /* $B5 */
    TEXT_OP_COLOR_FADE_95 = 0xaf53,                            /* $95 */
    TEXT_OP_COLOR_FADE_94 = 0xaf5b,                            /* $94 */
    TEXT_OP_COLOR_FADE_96 = 0xaf63,                            /* $96 */
    TEXT_OP_STORE_D4F3 = 0xb117,                               /* $C1 */
    TEXT_OP_WAIT_FRAMES = 0xb2eb,                              /* $37 */
    TEXT_OP_WAIT_FOR_PARTY = 0xb397,                           /* $3C */
    TEXT_OP_FORCED_BLANK = 0xb484,                             /* $3E */
    TEXT_OP_FULL_BRIGHTNESS = 0xb48c,                          /* $3F */
    TEXT_OP_RAISE_0B62 = 0xb69f,                               /* $CC */
    TEXT_OP_WAIT_D0FC = 0xb89d,                                /* $71 */
    TEXT_OP_SKIP_1_4F = 0xb8ed,                                /* $4F */
    TEXT_OP_TYPING_SOUND_OFF = 0xb8f3,                         /* $50 */
    TEXT_OP_STOP_COLOR_FADES = 0xb955,                         /* $C5 */
    TEXT_OP_STOP_SHAKE = 0xb962,                               /* $8A */
    TEXT_OP_WAIT_FOR_EFFECTS = 0xb98f,                         /* $57 */
    TEXT_OP_WAIT_FOR_FADE = 0xb99d,                            /* $76 */
    TEXT_OP_START_SHAKE = 0xba1c,                              /* $5A */
    TEXT_OP_SKIP_1_5F = 0xbc05,                                /* $5F */
    TEXT_OP_WRITE_PPU = 0xbc0b,                                /* $60 */
    TEXT_OP_SKIP_BRANCH = 0xbc3d,                              /* $68 */
};

/* $33: wait until actor $1269 stops moving. */
static unsigned TextOpWaitForActor(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    LoadAAbsolute8(memory, cpu, TEXT_WAIT_ACTOR, 0);
    if (cpu->negative) {
        *handoff = 0x80a834u;
        return TEXT_OPCODE_HANDOFF;
    }
    TransferDirectToA(cpu);
    LoadAAbsolute8(memory, cpu, TEXT_WAIT_ACTOR, 0);
    TransferAToX(cpu);
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
    And8(cpu, 0x88u);
    if (!cpu->zero) {
        TextPrevByte(memory, cpu, 0xa822u);
        return TEXT_OPCODE_EXIT;
    }
    LoadA8(cpu, 0xffu);                                    /* A826 */
    StoreAAbsolute8(memory, cpu, TEXT_WAIT_ACTOR, 0);
    TextNextByte(memory, cpu, 0xa82du);
    TextNextByte(memory, cpu, 0xa830u);
    return TEXT_OPCODE_NEXT;
}

/* $37: wait until frame counter $42 reaches n. */
static unsigned TextOpWaitFrames(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x20u);
    TestBitsAbsolute8(memory, cpu, WRAM_TEXT_STATE, 1);
    if (cpu->zero)
        Write8(memory, DirectAddress(cpu, DP_FRAME_COUNTER), 0x00u);
    LoadA8(cpu, DirectByte(memory, cpu, DP_FRAME_COUNTER));    /* B2F4 */
    Compare8(cpu, A8(cpu),
        Read8(memory, AbsoluteIndexedAddress(cpu, 0x0000u, cpu->y)));
    if (!cpu->carry) {
        TextPrevByte(memory, cpu, 0xb2fdu);
        return TEXT_OPCODE_EXIT;
    }
    TextNextByte(memory, cpu, 0xb303u);                    /* B301 */
    LoadA8(cpu, 0x20u);
    TestBitsAbsolute8(memory, cpu, WRAM_TEXT_STATE, 0);
    return TEXT_OPCODE_NEXT;
}

/* $3C: wait until no party actor is moving. */
static unsigned TextOpWaitForParty(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    unsigned i;

    LoadAAbsolute8(memory, cpu, TEXT_WAIT_ACTOR, 0);
    if (cpu->negative) {
        *handoff = 0x80b3dfu;
        return TEXT_OPCODE_HANDOFF;
    }
    LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, 0);
    for (i = 1; i < 5u; ++i)
        Or8(cpu, Read8(memory,
            AbsoluteIndexedAddress(cpu, (uint16_t)(WRAM_ACTOR_STATE + i), 0)));
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
            LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
            Or8(cpu, 0x04u);
            StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
            cpu->x = (uint16_t)(cpu->x - 1u);
            SetNz16(cpu, cpu->x);
        } while (!cpu->zero);
    } else {
        LoadX16(cpu, 0x0004u);                             /* B3C9 */
        LoadA8(cpu, 0xffu);
        StoreAAbsolute8(memory, cpu, TEXT_WAIT_ACTOR, 0);
        do {
            StoreAAbsolute8(memory, cpu, 0x09a1u, cpu->x);
            cpu->x = (uint16_t)(cpu->x - 1u);
            SetNz16(cpu, cpu->x);
        } while (!cpu->negative);
    }
    LoadA8(cpu, 0xffu);                                    /* B3D7 */
    StoreAAbsolute8(memory, cpu, TEXT_WAIT_ACTOR, 0);
    return TEXT_OPCODE_NEXT;
}

/* $00/$42: end, or return from a sub-script. */
static unsigned TextOpEnd(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    LoadAAbsolute8(memory, cpu, TEXT_CALLER_BANK, 0);
    if (cpu->zero) {
        *handoff = 0x809d69u;
        return TEXT_OPCODE_HANDOFF;
    }
    StoreAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);         /* back to caller */
    LoadA8(cpu, 0x10u);
    TestBitsAbsolute8(memory, cpu, WRAM_TEXT_STATE, 0);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TEXT_CALLER_POINTER, 0));
    Write16Absolute(memory, cpu, TEXT_SCRIPT_POINTER, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(memory, cpu, TEXT_CALLER_BANK, 0);
    return TEXT_OPCODE_RELOAD;
}

/* $68: skip the branch when $05B3 bit 4 is set. */
static unsigned TextOpSkipBranch(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint32_t *handoff) {
    LoadAAbsolute8(memory, cpu, 0x05b3u, 0);
    BitImmediate8(cpu, 0x10u);
    if (cpu->zero) {
        *handoff = 0x80bc44u;
        return TEXT_OPCODE_HANDOFF;
    }
    TextNextByte(memory, cpu, 0xbc4eu);
    TextNextByte(memory, cpu, 0xbc51u);
    return TEXT_OPCODE_NEXT;
}

/* $03: new line. */
static unsigned TextOpNewLine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TextNewLine(memory, cpu, 0x9dd7u);
    return TEXT_OPCODE_NEXT;
}

/* $05/$06: call a sub-script from table 0 or 1. */
static unsigned TextOpSubScript(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
    ExchangeAccumulatorBytes(cpu);
    LoadA8(cpu, handler == TEXT_OP_SUB_SCRIPT_05 ? 0x00u : 0x01u);
    TextSubScript(memory, cpu);
    return TEXT_OPCODE_NEXT;
}

/* $0C-$0F: print ? ! , . then a new line. */
static unsigned TextOpPunctuationNewLine(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    LoadA8(cpu, handler == TEXT_OP_QUESTION_NEW_LINE      ? '?'
                : handler == TEXT_OP_EXCLAMATION_NEW_LINE ? '!'
                : handler == TEXT_OP_COMMA_NEW_LINE       ? ','
                                                          : '.');
    StoreAAbsolute8(memory, cpu, TEXT_GLYPH, 0);
    StoreZeroAbsolute8(memory, cpu, 0x09b0u, 0);
    Write16Absolute(memory, cpu, TEXT_SCRIPT_POINTER, cpu->y);
    TextGlyphUpload(memory, cpu, 0x9f09u);
    TextNewLine(memory, cpu, 0x9f0cu);
    return TEXT_OPCODE_EXIT;
}

/* $15: goto when the event flag is set. */
static unsigned TextOpGotoIfFlag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
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
}

/* $1A/$1B: set or clear an event flag. */
static unsigned TextOpChangeFlag(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    PushY(memory, cpu);
    TextFlagBit(memory, cpu, (uint16_t)(handler + 6u));
    cpu->x = DirectByte(memory, cpu, 0x56u);
    SetNz8(cpu, (uint8_t)cpu->x);
    if (handler == TEXT_OP_SET_FLAG) {
        LoadAAbsolute8(memory, cpu, WRAM_EVENT_FLAGS, cpu->x);
        Or8(cpu, DirectByte(memory, cpu, 0x57u));
    } else {
        LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
        LoadA8(cpu, (uint8_t)(A8(cpu) & Read8(memory,
            AbsoluteIndexedAddress(cpu, WRAM_EVENT_FLAGS, cpu->x))));
    }
    StoreAAbsolute8(memory, cpu, WRAM_EVENT_FLAGS, cpu->x);
    SetIndexWidth(cpu, 0);
    cpu->y = PullIndexValue(memory, cpu);
    TextNextByte(memory, cpu, (uint16_t)(handler + 0x16u));
    return TEXT_OPCODE_NEXT;
}

/* $1C: goto. */
static unsigned TextOpGoto(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TextGoto(memory, cpu);
    return TEXT_OPCODE_NEXT;
}

/* $1D: store a byte into variable $079E+n. */
static unsigned TextOpSetVariable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    TextNextByte(memory, cpu, 0xa3e2u);
    TransferAToX(cpu);
    TextNextByte(memory, cpu, 0xa3e6u);
    StoreAAbsolute8(memory, cpu, 0x079eu, cpu->x);
    return TEXT_OPCODE_NEXT;
}

/* $3E/$3F: forced blank or full brightness. */
static unsigned TextOpSetBrightness(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    StoreAImmediate8(memory, cpu,
        handler == TEXT_OP_FORCED_BLANK ? BRIGHTNESS_FORCED_BLANK
                                        : BRIGHTNESS_FULL,
        WRAM_BRIGHTNESS);
    return TEXT_OPCODE_NEXT;
}

/* $4F/$5F: skip one operand byte. */
static unsigned TextOpSkipOperand(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
    return TEXT_OPCODE_NEXT;
}

/* $27/$2A: skip two or three operand bytes. */
static unsigned TextOpSkipOperands(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    TextNextByte(memory, cpu, (uint16_t)(handler + 2u));
    TextNextByte(memory, cpu, (uint16_t)(handler + 5u));
    if (handler == TEXT_OP_SKIP_3)
        TextNextByte(memory, cpu, (uint16_t)(handler + 8u));
    return TEXT_OPCODE_NEXT;
}

/* $50: typing sound off. */
static unsigned TextOpTypingSoundOff(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StoreZeroAbsolute8(memory, cpu, TEXT_TYPING_SOUND, 0);
    return TEXT_OPCODE_NEXT;
}

/* $C1: two bytes into $7F:D4F3/D4F4. */
static unsigned TextOpStoreD4F3(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TextNextByte(memory, cpu, 0xb119u);
    Write8(memory, 0x7fd4f3u, A8(cpu));
    TextNextByte(memory, cpu, 0xb120u);
    Write8(memory, 0x7fd4f4u, A8(cpu));
    return TEXT_OPCODE_NEXT;
}

/* $5A: start a screen shake. */
static unsigned TextOpStartShake(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    LoadA8(cpu, 0x04u);
    TestBitsAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 1);
    for (i = 0; i < 3u; ++i) {
        TextNextByte(memory, cpu, (uint16_t)(0xba23u + 7u * i));
        Write8(memory, 0x7fd07eu + i, A8(cpu));
    }
    return TEXT_OPCODE_NEXT;
}

/* $8A: stop the screen shake. */
static unsigned TextOpStopShake(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned i;

    TransferDirectToA(cpu);
    for (i = 0; i < 4u; ++i)
        Write8(memory, 0x7fd081u + i, A8(cpu));
    LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
    And8(cpu, 0xfbu);
    StoreAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
    return TEXT_OPCODE_NEXT;
}

/* $C5: stop COLDATA and palette fades. */
static unsigned TextOpStopColorFades(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, 0x80u);
    TestBitsAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
    LoadA8(cpu, 0x01u);
    TestBitsAbsolute8(memory, cpu, WRAM_PALETTE_FADE, 0);
    return TEXT_OPCODE_NEXT;
}

/* $B5: clear $089D, set $085D. */
static unsigned TextOpB5(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    StoreZeroAbsolute8(memory, cpu, 0x089du, 0);
    StoreAImmediate8(memory, cpu, 0xffu, 0x085du);
    return TEXT_OPCODE_NEXT;
}

/* $57: wait until no screen effect runs. */
static unsigned TextOpWaitForEffects(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
    if (!cpu->zero) {
        TextPrevByte(memory, cpu, 0xb999u);
        return TEXT_OPCODE_EXIT;
    }
    return TEXT_OPCODE_NEXT;
}

/* $71: wait for $7F:D0FC frames. */
static unsigned TextOpWaitD0FC(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadA8(cpu, (uint8_t)(Read8(memory, 0x7fd0fcu) - 1u));
    Write8(memory, 0x7fd0fcu, A8(cpu));
    if (!cpu->zero) {
        TextPrevByte(memory, cpu, 0xb8adu);
        return TEXT_OPCODE_EXIT;
    }
    return TEXT_OPCODE_NEXT;
}

/* $76: wait until the brightness fade ends. */
static unsigned TextOpWaitForFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, WRAM_FADE_CONTROL, 0);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, WRAM_SCREEN_EFFECTS, 0);
        BitImmediate8(cpu, 0x03u);
        if (cpu->zero)
            return TEXT_OPCODE_NEXT;
    }
    TextPrevByte(memory, cpu, 0xb9aeu);
    return TEXT_OPCODE_EXIT;
}

/* $AA: clear $7F:D100-D103. */
static unsigned TextOpClearD100(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    TransferDirectToA(cpu);
    Write16Long(memory, 0x7fd100u, cpu->accumulator);
    Write16Long(memory, 0x7fd102u, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
    return TEXT_OPCODE_NEXT;
}

/* $CC: raise $0B62 to $7F:D4F7. */
static unsigned TextOpRaise0B62(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x0b62u, 0);
    if (cpu->zero) {
        LoadA8(cpu, Read8(memory, 0x7fd4f7u));
        Compare8(cpu, A8(cpu),
            Read8(memory, AbsoluteIndexedAddress(cpu, 0x0b62u, 0)));
        if (cpu->carry)
            StoreAAbsolute8(memory, cpu, 0x0b62u, 0);
    }
    return TEXT_OPCODE_NEXT;
}

/* $60: write a byte to PPU register $2100+n. */
static unsigned TextOpWritePpu(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    TextNextByte(memory, cpu, 0xbc0eu);
    TransferAToX(cpu);
    TextNextByte(memory, cpu, 0xbc12u);
    StoreAAbsolute8(memory, cpu, SNES_INIDISP, cpu->x);
    return TEXT_OPCODE_NEXT;
}

/* $1E: add a byte to variable $079E+n. */
static unsigned TextOpAddVariable(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    TransferDirectToA(cpu);
    TextNextByte(memory, cpu, 0xa3f0u);
    TransferAToX(cpu);
    TextNextByte(memory, cpu, 0xa3f4u);
    cpu->carry = 0;
    Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x079eu, cpu->x)));
    StoreAAbsolute8(memory, cpu, 0x079eu, cpu->x);
    return TEXT_OPCODE_NEXT;
}

/* $09: print the name buffer at $0BAD. */
static unsigned TextOpPrintName(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Write16Absolute(memory, cpu, TEXT_CALLER_POINTER, cpu->y);
    LoadAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
    StoreAAbsolute8(memory, cpu, TEXT_CALLER_BANK, 0);
    LoadA8(cpu, 0x10u);
    TestBitsAbsolute8(memory, cpu, WRAM_TEXT_STATE, 1);
    LoadY16(cpu, 0x0badu);
    Write16Absolute(memory, cpu, TEXT_SCRIPT_POINTER, cpu->y);
    StoreZeroAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
    return TEXT_OPCODE_RELOAD;
}

/* $22/$26: add or subtract gold. */
static unsigned TextOpChangeGold(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    TextNextWord(memory, cpu, (uint16_t)(handler + 2u));
    TextGold(memory, cpu, handler == TEXT_OP_ADD_GOLD, (uint16_t)(handler + 5u));
    return TEXT_OPCODE_NEXT;
}

/* $94-$96: COLDATA fade with preset colour bits. */
static unsigned TextOpColorFade(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    uint16_t handler) {
    LoadA8(cpu, handler == TEXT_OP_COLOR_FADE_95 ? 0x53u
        : handler == TEXT_OP_COLOR_FADE_94 ? 0x93u : 0xd3u);
    TextColorFade(memory, cpu, (uint16_t)(handler + 4u));
    return TEXT_OPCODE_NEXT;
}

/* Script opcodes behind JMP ($CA14,x); others hand off. */
static unsigned TextScriptOpcode(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
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
    case TEXT_OP_WAIT_FOR_ACTOR:
        return TextOpWaitForActor(memory, cpu, handoff);
    case TEXT_OP_WAIT_FRAMES:
        return TextOpWaitFrames(memory, cpu);
    case TEXT_OP_WAIT_FOR_PARTY:
        return TextOpWaitForParty(memory, cpu, handoff);
    case TEXT_OP_END:
        return TextOpEnd(memory, cpu, handoff);
    case TEXT_OP_SKIP_BRANCH:
        return TextOpSkipBranch(memory, cpu, handoff);
    case TEXT_OP_NEW_LINE:
        return TextOpNewLine(memory, cpu);
    case TEXT_OP_SUB_SCRIPT_05:
    case TEXT_OP_SUB_SCRIPT_06:
        return TextOpSubScript(memory, cpu, handler);
    case TEXT_OP_QUESTION_NEW_LINE:
    case TEXT_OP_EXCLAMATION_NEW_LINE:
    case TEXT_OP_COMMA_NEW_LINE:
    case TEXT_OP_PERIOD_NEW_LINE:
        return TextOpPunctuationNewLine(memory, cpu, handler);
    case TEXT_OP_GOTO_IF_FLAG:
        return TextOpGotoIfFlag(memory, cpu);
    case TEXT_OP_SET_FLAG:
    case TEXT_OP_CLEAR_FLAG:
        return TextOpChangeFlag(memory, cpu, handler);
    case TEXT_OP_GOTO:
        return TextOpGoto(memory, cpu);
    case TEXT_OP_SET_VARIABLE:
        return TextOpSetVariable(memory, cpu);
    case TEXT_OP_FORCED_BLANK:
    case TEXT_OP_FULL_BRIGHTNESS:
        return TextOpSetBrightness(memory, cpu, handler);
    case TEXT_OP_SKIP_1_4F:
    case TEXT_OP_SKIP_1_5F:
        return TextOpSkipOperand(memory, cpu, handler);
    case TEXT_OP_SKIP_2:
    case TEXT_OP_SKIP_3:
        return TextOpSkipOperands(memory, cpu, handler);
    case TEXT_OP_TYPING_SOUND_OFF:
        return TextOpTypingSoundOff(memory, cpu);
    case TEXT_OP_STORE_D4F3:
        return TextOpStoreD4F3(memory, cpu);
    case TEXT_OP_START_SHAKE:
        return TextOpStartShake(memory, cpu);
    case TEXT_OP_STOP_SHAKE:
        return TextOpStopShake(memory, cpu);
    case TEXT_OP_STOP_COLOR_FADES:
        return TextOpStopColorFades(memory, cpu);
    case TEXT_OP_B5:
        return TextOpB5(memory, cpu);
    case TEXT_OP_WAIT_FOR_EFFECTS:
        return TextOpWaitForEffects(memory, cpu);
    case TEXT_OP_WAIT_D0FC:
        return TextOpWaitD0FC(memory, cpu);
    case TEXT_OP_WAIT_FOR_FADE:
        return TextOpWaitForFade(memory, cpu);
    case TEXT_OP_CLEAR_D100:
        return TextOpClearD100(memory, cpu);
    case TEXT_OP_RAISE_0B62:
        return TextOpRaise0B62(memory, cpu);
    case TEXT_OP_WRITE_PPU:
        return TextOpWritePpu(memory, cpu);
    case TEXT_OP_ADD_VARIABLE:
        return TextOpAddVariable(memory, cpu);
    case TEXT_OP_PRINT_NAME:
        return TextOpPrintName(memory, cpu);
    case TEXT_OP_ADD_GOLD:
    case TEXT_OP_SUBTRACT_GOLD:
        return TextOpChangeGold(memory, cpu, handler);
    case TEXT_OP_COLOR_FADE_95:
    case TEXT_OP_COLOR_FADE_94:
    case TEXT_OP_COLOR_FADE_96:
        return TextOpColorFade(memory, cpu, handler);
    default:
        return TEXT_OPCODE_HANDOFF;
    }
}

/* $80:9CB8 text step: plain characters native, the rest on LLE. */
Lufia2ExecutionResult Lufia2TextEngineStepBody(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2ExecutionResult result) {
    unsigned opcodes;
    unsigned words = 0;

    LoadA8(cpu, Read8(memory, TEXT_PRINT_COUNTDOWN));          /* 9CB8 */
    if (!cpu->zero) {
        Compare8(cpu, A8(cpu), 0x03u);
        if (!cpu->carry) {
            DecrementA8(cpu);
            Write8(memory, TEXT_PRINT_COUNTDOWN, A8(cpu));
            result.pc = 0x809cc7u;
            return result;
        }
        TransferDirectToA(cpu);                                /* 9CC8 */
        Write8(memory, TEXT_PRINT_COUNTDOWN, A8(cpu));
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
        LoadAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);      /* 9CD9 */
        PushAccumulator8(memory, cpu);
        PullDataBank(memory, cpu);
        LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, TEXT_SCRIPT_POINTER, 0));
        LoadAAbsolute8(memory, cpu, TEXT_RETURN_BANK, 0);
        if (cpu->zero)
            break;
        {
            const uint32_t count = AbsoluteIndexedAddress(cpu, TEXT_RETURN_COUNT, 0);
            const uint8_t left = (uint8_t)(Read8(memory, count) - 1u);

            Write8(memory, count, left);
            SetNz8(cpu, left);
        }
        if (!cpu->zero)
            break;
        SetAccumulatorWidth(cpu, 0);                           /* 9CEB */
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, TEXT_RETURN_POINTER, 0));
        Write16Absolute(memory, cpu, TEXT_SCRIPT_POINTER, cpu->accumulator);
        SetAccumulatorWidth(cpu, 1);
        LoadAAbsolute8(memory, cpu, TEXT_RETURN_BANK, 0);
        StoreAAbsolute8(memory, cpu, TEXT_SCRIPT_BANK, 0);
        StoreZeroAbsolute8(memory, cpu, TEXT_RETURN_BANK, 0);
    }
    for (;;) {
        uint16_t handler;
        uint32_t handoff = 0x809d3bu;

        Write16Absolute(memory, cpu, TEXT_SCRIPT_POINTER, cpu->y); /* 9D00 */
        StoreZeroAbsolute8(memory, cpu, 0x0563u, 0);
        TransferDirectToA(cpu);
        LoadAAbsolute8(memory, cpu, WRAM_TEXT_STATE, 0);
        And8(cpu, 0x01u);
        if (cpu->zero) {
            TextCloseWindow(memory, cpu, 0x9d30u);             /* 9D2E */
        } else {
            LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);      /* 9D0E */
            Compare8(cpu, A8(cpu), 0x10u);
            if (cpu->carry) {
                StoreAAbsolute8(memory, cpu, TEXT_GLYPH, 0);
                StoreZeroAbsolute8(memory, cpu, 0x09b0u, 0);
                Compare8(cpu, A8(cpu), 0x80u);
                if (!cpu->carry)
                    break;                                     /* BCE4 */
                if (words >= 4096u) {
                    result = ExecutionHandoff(cpu, 0x809d22u);
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
            result = ExecutionHandoff(cpu, handoff);
            result.dispatches = handoff == 0x809d3bu ? opcodes : 0;
            return result;
        }
    }
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);                   /* BCE4 */
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        LoadA8(cpu, 0x10u);
        TestBitsAbsolute8(memory, cpu, TEXT_WINDOW_STATE, 0);
        if (!cpu->zero)
            return ExecutionHandoff(cpu, 0x80bcf2u);           /* C56E */
    }
    LoadAAbsolute8(memory, cpu, WRAM_TEXT_STATE, 0);           /* BCF5 */
    And8(cpu, 0x10u);
    if (!cpu->zero)
        IncrementY16(cpu);
    else
        TextNextByte(memory, cpu, 0xbd01u);
    Write16Absolute(memory, cpu, TEXT_SCRIPT_POINTER, cpu->y); /* BD02 */
    TextGlyphUpload(memory, cpu, 0xbd07u);
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);                   /* BD08 */
    BitImmediate8(cpu, 0x02u);
    if (cpu->zero) {
        LoadAAbsolute8(memory, cpu, TEXT_PRINT_DELAY, 0);
        Write8(memory, TEXT_PRINT_COUNTDOWN, A8(cpu));
        LoadAAbsolute8(memory, cpu, 0x1255u, 0);
        Compare8(cpu, A8(cpu), 0x10u);
        if (!cpu->zero) {
            LoadAAbsolute8(memory, cpu, TEXT_TYPING_SOUND, 0);
            if (!cpu->zero) {
                LoadA8(cpu, (uint8_t)(Read8(memory, TEXT_TYPING_SOUND_PHASE) ^ 0x01u));
                Write8(memory, TEXT_TYPING_SOUND_PHASE, A8(cpu));
                if (!cpu->zero) {
                    LoadAAbsolute8(memory, cpu, TEXT_TYPING_SOUND, 0);
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
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SimulateJsrFrame(memory, cpu, 0x9ca5u);
    Push8(memory, cpu, PackStatus(cpu));                       /* C11C */
    SetIndexWidth(cpu, 1);
    cpu->x = 0x27u;
    do {
        LoadAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
        And8(cpu, 0xfeu);
        StoreAAbsolute8(memory, cpu, WRAM_ACTOR_STATE, cpu->x);
        cpu->x = (uint8_t)(cpu->x - 1u);
        SetNz8(cpu, (uint8_t)cpu->x);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    SimulateRtsFrame(memory, cpu);
}

/* $80:9C80: text box waits for its timer or the A/X buttons. */
Lufia2ExecutionResult Lufia2TextPromptTick(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu,
    Lufia2ExecutionResult result) {
    LoadAAbsolute8(memory, cpu, TEXT_PROMPT_TIMER, 0);         /* 9C80 */
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
        And8(cpu, DirectByte(memory, cpu, DP_BUTTONS_HELD));   /* C81E */
        if (!cpu->zero)
            TestBitsDirect(memory, cpu, DP_BUTTONS_PRESSED, 0);
        SimulateRtsFrame(memory, cpu);
        if (cpu->zero)
            return result;
    }
    LoadA8(cpu, 0x08u);                                        /* 9C96 */
    TestBitsAbsolute8(memory, cpu, WRAM_TEXT_STATE, 0);
    if (!cpu->zero) {
        StoreZeroAbsolute8(memory, cpu, WRAM_TEXT_STATE, 0);
        TextCloseWindow(memory, cpu, 0x9ca2u);
        TextReleaseActors(memory, cpu);
        return result;
    }
    LoadAAbsolute8(memory, cpu, WRAM_TEXT_STATE, 0);           /* 9CA8 */
    And8(cpu, 0xfdu);
    StoreAAbsolute8(memory, cpu, WRAM_TEXT_STATE, 0);
    return Lufia2TextEngineStepBody(memory, cpu, result);
}

/* $80:9CB8: text engine step, JSL entry. */
Lufia2ExecutionResult Lufia2TextEngineStep(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    if (!cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x809cb8u);
    return Lufia2TextEngineStepBody(memory, cpu, ExecutionReturned(0x809db2u));
}
