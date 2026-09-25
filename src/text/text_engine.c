/* Text and cutscene script engine ($80:9CB8). */

#include "core/cpu_internal.h"
#include "text/text_internal.h"

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
