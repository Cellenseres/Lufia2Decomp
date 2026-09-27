/* Glyph buffer and window row upload setup ($80:C56E, C784, C23D). */
#include "core/cpu_internal.h"
#include "lufia2/text.h"
#include "text/text_internal.h"
#include "actor/actor_internal.h"

/* $80:BF6F: find an actor id, excluding slots with flag $04. Missing ids
 * leave the old offsets intact, exactly as the original caller expects. */
static void TextFindWindowActor(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect8(memory, cpu, 0x54u);
    LoadX16(cpu, 0);
    for (;;) {
        LoadAAbsolute8(memory, cpu, 0x0622u, cpu->x);
        And8(cpu, 4);
        if (cpu->zero) {
            LoadAAbsolute8(memory, cpu, 0x05fau, cpu->x);
            Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x54u));
            if (cpu->zero) {
                StoreXDirect16(memory, cpu, 0xa7u);
                SimulateJslFrame(memory, cpu, 0x80u, 0xbf8fu);
                Lufia2ActorRecordOffsets(memory, cpu);
                SimulateRtlFrame(memory, cpu);
                cpu->carry = 0;
                return;
            }
        }
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x28u);
        if (cpu->zero) { cpu->carry = 1; return; }
    }
}

/* $80:C557: packed byte coordinates to a tilemap byte offset. TDC supplies
 * the low byte of the row word; its high byte is not assumed to be zero. */
static void TextWindowTileOffset(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    StoreADirect16(memory, cpu, 0x4eu);
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x4fu));
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    LsrA16(cpu); LsrA16(cpu);
    StoreADirect16(memory, cpu, 0x51u);
    LoadADirect16(memory, cpu, 0x4eu);
    And16(cpu, 0xffu);
    AslA16(cpu);
    Add16Value(cpu, Read16Direct(memory, cpu, 0x51u));
}

/* $80:C52C: border row, including alternating tiles and the mirrored end.
 * A zero width still executes the original 65,536-iteration loop. */
static void TextWindowBorderRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    PushIndex(memory, cpu);
    cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1255u, 0));
    StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
    IncrementX16(cpu); IncrementX16(cpu);
    LoadY16(cpu, Read16Direct(memory, cpu, 0x63u));
    IncrementA16(cpu); IncrementA16(cpu);
    do {
        StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 1u));
        IncrementX16(cpu); IncrementX16(cpu);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    And16(cpu, 0xfffeu);
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    LoadA16(cpu, (uint16_t)(cpu->accumulator - 1u));
    Or16(cpu, 0x4000u);
    StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
    PullAccumulator16(memory, cpu); /* PHX is deliberately consumed by PLA. */
    cpu->carry = 0;
    Add16Value(cpu, 0x40u);
    TransferAToX(cpu);
}

/* $80:C33F-C42F: actor-relative placement and speech-tail adjustment. */
static void TextWindowActorPosition(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadAAbsolute8(memory, cpu, 0x09abu, 0);
    SimulateJslFrame(memory, cpu, 0x80u, 0xc345u);
    TextFindWindowActor(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x125cu, 0);
    AslA8(cpu); Adc8(cpu, 2);
    StoreADirect8(memory, cpu, 0x54u);
    LoadXDirect(memory, cpu, 0xa9u);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fde3eu, cpu->x)));
    Subtract16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a6u, 0));
    And16(cpu, 0xfff0u);
    LsrA16(cpu); LsrA16(cpu); LsrA16(cpu);
    PushAccumulator16(memory, cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x7fddaeu, cpu->x)));
    Subtract16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x05a4u, 0));
    LsrA16(cpu); LsrA16(cpu); LsrA16(cpu);
    Add16Value(cpu, 0);
    StoreADirect16(memory, cpu, 0x5du);
    PullAccumulator16(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    StoreADirect8(memory, cpu, 0x5eu);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);
    Compare8(cpu, A8(cpu), 4);
    if (!cpu->zero) goto above;
below:                                                       /* C38F */
    Write8(memory, DirectAddress(cpu, 0x55u), 0);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5eu));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    StoreADirect8(memory, cpu, 0x66u);
    cpu->carry = 0; Adc8(cpu, DirectByte(memory, cpu, 0x54u));
    Compare8(cpu, A8(cpu), 0x1cu);
    if (!cpu->carry) goto horizontal;
above:                                                       /* C37F */
    LoadA8(cpu, 0xffu); StoreADirect8(memory, cpu, 0x55u);
    LoadA8(cpu, DirectByte(memory, cpu, 0x5eu));
    cpu->carry = 1; Sbc8(cpu, DirectByte(memory, cpu, 0x54u));
    cpu->carry = 1; Sbc8(cpu, 2);
    StoreADirect8(memory, cpu, 0x66u);
    if (cpu->negative) goto below;
horizontal:                                                  /* C39D */
    LoadA8(cpu, DirectByte(memory, cpu, 0x63u));
    LsrA8(cpu);
    cpu->carry = 1; Sbc8(cpu, DirectByte(memory, cpu, 0x5du));
    LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
    StoreADirect8(memory, cpu, 0x65u);
    LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
    if (cpu->negative) {
        LoadA8(cpu, 1); StoreADirect8(memory, cpu, 0x65u);
    } else {
        cpu->carry = 0; Adc8(cpu, DirectByte(memory, cpu, 0x63u));
        cpu->carry = 1; Sbc8(cpu, 0x1du);
        if (cpu->carry) {
            LoadA8(cpu, 0x1du);
            cpu->carry = 1; Sbc8(cpu, DirectByte(memory, cpu, 0x63u));
            StoreADirect8(memory, cpu, 0x65u);
        }
    }
    LoadA8(cpu, 0xe0u); StoreADirect8(memory, cpu, 0x5bu);
    DecrementDirect8(memory, cpu, 0x5eu);
    LoadA8(cpu, DirectByte(memory, cpu, 0x55u));
    if (!cpu->zero) {
        DecrementDirect8(memory, cpu, 0x5eu);
        DecrementDirect8(memory, cpu, 0x5eu);
    } else {
        ExchangeAccumulatorBytes(cpu);
        LoadA8(cpu, 0x60u); StoreADirect8(memory, cpu, 0x5bu);
        ExchangeAccumulatorBytes(cpu);
    }
    IncrementDirect8(memory, cpu, 0x5du);
    LoadXDirect(memory, cpu, 0xa7u);
    LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
    BitImmediate8(cpu, 2);
    if (!cpu->zero) {
        IncrementDirect8(memory, cpu, 0x5du);
        IncrementDirect8(memory, cpu, 0x5du);
    }
    LoadAAbsolute8(memory, cpu, 0x0692u, cpu->x);
    Compare8(cpu, A8(cpu), 6);
    if (cpu->zero) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
        cpu->carry = 1; Sbc8(cpu, 4);
        StoreADirect8(memory, cpu, 0x5du);
        LoadA8(cpu, 0x40u); TestBitsDirect(memory, cpu, 0x5bu, 0);
        LoadA8(cpu, Read8(memory, LongIndexedAddress(0x7fe216u, cpu->x)));
        BitImmediate8(cpu, 2);
        if (!cpu->zero) {
            DecrementDirect8(memory, cpu, 0x5du);
            DecrementDirect8(memory, cpu, 0x5du);
        }
    }
    LoadA8(cpu, DirectByte(memory, cpu, 0x5du));
    LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
    Compare8(cpu, A8(cpu), DirectByte(memory, cpu, 0x65u));
    if (!cpu->carry) StoreADirect8(memory, cpu, 0x65u);
    else {
        LoadA8(cpu, DirectByte(memory, cpu, 0x65u));
        cpu->carry = 0; Adc8(cpu, DirectByte(memory, cpu, 0x63u));
        LoadA8(cpu, (uint8_t)(A8(cpu) - 1u));
        cpu->carry = 1; Sbc8(cpu, DirectByte(memory, cpu, 0x5du));
        if (!cpu->carry) {
            LoadA8(cpu, (uint8_t)(A8(cpu) ^ 0xffu));
            LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
            cpu->carry = 0; Adc8(cpu, DirectByte(memory, cpu, 0x65u));
            StoreADirect8(memory, cpu, 0x65u);
        }
    }
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, 0x65u);
    SimulateJsrFrame(memory, cpu, 0xc427u);
    TextWindowTileOffset(memory, cpu); SimulateRtsFrame(memory, cpu);
    StoreADirect16(memory, cpu, 0x60u);
    LoadADirect16(memory, cpu, 0x5du);
    SimulateJsrFrame(memory, cpu, 0xc42eu);
    TextWindowTileOffset(memory, cpu); SimulateRtsFrame(memory, cpu);
    StoreADirect16(memory, cpu, 0x5du);
}

/* $80:C305: construct the window frame and optional speech tail in WRAM.
 * Entry M1X0. The explicit-position path reaches C431 with M1 and decodes
 * A9 FC FF 8D 9E 05 as LDA #FC; SBC $059E8D,X (not LDA #FFFC; STA $059E).
 * DB becomes $7E; caller owns its saved DB frame. No MMIO writes. */
Lufia2ExecutionResult Lufia2TextBuildWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Write8(memory, DirectAddress(cpu, 0x5du), 0);
    Write8(memory, DirectAddress(cpu, 0x5eu), 0);
    LoadAAbsolute8(memory, cpu, 0x125bu, 0);
    Write8(memory, 0x7fd089u, A8(cpu));
    StoreADirect8(memory, cpu, 0x63u);
    Write8(memory, DirectAddress(cpu, 0x64u), 0);
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    StoreADirect8(memory, cpu, 0x56u);
    Write8(memory, DirectAddress(cpu, 0x57u), 0);
    TransferDirectToA(cpu); Write8(memory, 0x7fd08au, A8(cpu));
    LoadAAbsolute8(memory, cpu, 0x099cu, 0); And8(cpu, 4);
    if (!cpu->zero) IncrementDirect8(memory, cpu, 0x63u);
    LoadAAbsolute8(memory, cpu, 0x1255u, 0);
    if (!cpu->zero) {
        LoadA8(cpu, DirectByte(memory, cpu, 0x63u)); And8(cpu, 1);
        if (!cpu->zero) IncrementDirect8(memory, cpu, 0x63u);
    }
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x125du, 0));
    if (!cpu->zero) {
        Write16Direct(memory, cpu, 0x60u, cpu->y);
        LoadA8(cpu, 0xfcu);                                    /* C431 M1 */
        Sbc8(cpu, Read8(memory, LongIndexedAddress(0x059e8du, cpu->x)));
    } else {
        TextWindowActorPosition(memory, cpu);
        LoadA16(cpu, 0xfffcu);                                /* C431 M0 */
        StoreAAbsolute16(memory, cpu, 0x059eu, 0);
    }
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7eu); PushAccumulator8(memory, cpu); PullDataBank(memory, cpu);
    TransferDirectToA(cpu); LoadAAbsolute8(memory, cpu, 0x125cu, 0);
    SetAccumulatorWidth(cpu, 0); StoreADirect16(memory, cpu, 0x58u);
    LoadADirect16(memory, cpu, 0x60u);
    Write16Long(memory, 0x7fd085u, cpu->accumulator);
    TransferAToX(cpu);
    LoadA16(cpu, 0x20d6u);
    SimulateJsrFrame(memory, cpu, 0xc451u);
    TextWindowBorderRow(memory, cpu); SimulateRtsFrame(memory, cpu);
    LoadA16(cpu, 0x20dau); cpu->carry = 0;
    Add16Value(cpu, Read16AbsoluteIndexed(memory, cpu, 0x1255u, 0));
    StoreADirect16(memory, cpu, 0x65u);
    do {                                                     /* C45B */
        PushIndex(memory, cpu);
        LoadADirect16(memory, cpu, 0x65u);
        StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
        IncrementA16(cpu); StoreAAbsolute16(memory, cpu, 0x3040u, cpu->x);
        IncrementX16(cpu); IncrementX16(cpu);
        LoadY16(cpu, Read16Direct(memory, cpu, 0x63u));
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x099cu, 0));
        cpu->zero = (cpu->accumulator & 4u) == 0;
        if (!cpu->zero) {
            LoadA16(cpu, 0x20d7u);
            StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
            StoreAAbsolute16(memory, cpu, 0x3040u, cpu->x);
            IncrementX16(cpu); IncrementX16(cpu);
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        }
        LoadA16(cpu, 0x20d7u);
        do {
            StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
            StoreAAbsolute16(memory, cpu, 0x3040u, cpu->x);
            IncrementX16(cpu); IncrementX16(cpu);
            LoadY16(cpu, (uint16_t)(cpu->y - 1u));
        } while (!cpu->zero);
        LoadADirect16(memory, cpu, 0x65u); Or16(cpu, 0x4000u);
        StoreAAbsolute16(memory, cpu, 0x3000u, cpu->x);
        IncrementA16(cpu); StoreAAbsolute16(memory, cpu, 0x3040u, cpu->x);
        PullAccumulator16(memory, cpu);
        cpu->carry = 0; Add16Value(cpu, 0x80u); TransferAToX(cpu);
        Decrement16Direct(memory, cpu, 0x58u);
    } while (!cpu->zero);
    LoadA16(cpu, 0xa0d6u);
    SimulateJsrFrame(memory, cpu, 0xc4a6u);
    TextWindowBorderRow(memory, cpu); SimulateRtsFrame(memory, cpu);
    LoadY16(cpu, Read16Direct(memory, cpu, 0x5du));
    if (!cpu->zero) {
        LoadX16(cpu, 0);
        LoadADirect16(memory, cpu, 0x5au);
        if (cpu->negative) LoadX16(cpu, 0x0cu);
        AslA16(cpu);
        if (cpu->negative) {
            TransferXToA(cpu); cpu->carry = 0;
            Add16Value(cpu, 6); TransferAToX(cpu);
        }
        SetAccumulatorWidth(cpu, 1); cpu->carry = 0;
        /* Six consecutive ADCs deliberately propagate carry between tiles. */
        static const uint16_t destinations[6] = {0x3000,0x3002,0x3040,0x3042,0x3080,0x3082};
        for (unsigned i = 0; i < 6u; ++i) {
            LoadA8(cpu, Read8(memory, LongIndexedAddress(0x80c514u + i, cpu->x)));
            Adc8(cpu, Read8(memory, AbsoluteIndexedAddress(cpu, 0x1255u, 0)));
            StoreAAbsolute8(memory, cpu, destinations[i], cpu->y);
        }
        LoadA8(cpu, DirectByte(memory, cpu, 0x5bu));
        for (unsigned i = 0; i < 6u; ++i)
            StoreAAbsolute8(memory, cpu, (uint16_t)(destinations[i] + 1u), cpu->y);
    }
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x80c513u);
}

Lufia2ExecutionResult Lufia2TextClearGlyphBuffer(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));                       /* C784 */
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    Write16Absolute(memory, cpu, 0x09afu, 0);
    Write16Absolute(memory, cpu, 0x09b3u, 0);
    LoadA16(cpu, 0xd000u);
    StoreAAbsolute16(memory, cpu, 0x09b1u, 0);
    StoreAAbsolute16(memory, cpu, 0x1250u, 0);
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16Long(memory, 0x0009adu));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x80c7beu, cpu->x)));
    LoadX16(cpu, 0x0ffcu);
    do {                                                     /* C7AF */
        StoreAAbsolute16(memory, cpu, 0xd000u, cpu->x);
        StoreAAbsolute16(memory, cpu, 0xd002u, cpu->x);
        LoadX16(cpu, (uint16_t)(cpu->x - 1u)); LoadX16(cpu, (uint16_t)(cpu->x - 1u));
        LoadX16(cpu, (uint16_t)(cpu->x - 1u)); LoadX16(cpu, (uint16_t)(cpu->x - 1u));
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80c7bdu);
}

/* $80:C5DD: write both tile rows, retaining the LSR carry for ADC. */
static void TextWriteWindowRow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, Read8(memory, 0x7fd087u));
    ExchangeAccumulatorBytes(cpu);
    SetAccumulatorWidth(cpu, 0);
    LsrA16(cpu);
    StoreADirect16(memory, cpu, 0x54u);
    Add16Value(cpu, Read16Long(memory, 0x7fd085u));
    TransferAToX(cpu);
    StoreADirect16(memory, cpu, 0x56u);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0x099cu, 0));
    cpu->zero = (cpu->accumulator & 4u) == 0;
    if (!cpu->zero) {
        IncrementX16(cpu);
        IncrementX16(cpu);
    }
    LoadA16(cpu, Read16Long(memory, 0x7fd089u));
    And16(cpu, 0x00ffu);
    TransferAToY(cpu);
    LoadADirect16(memory, cpu, 0x54u);
    LsrA16(cpu);
    Or16(cpu, 0x2100u);
    StoreADirect16(memory, cpu, 0x54u);
    do {                                                     /* C60B */
        Write16Long(memory, LongIndexedAddress(0x7e3042u, cpu->x), cpu->accumulator);
        IncrementA16(cpu);
        Write16Long(memory, LongIndexedAddress(0x7e3082u, cpu->x), cpu->accumulator);
        IncrementA16(cpu);
        IncrementX16(cpu); IncrementX16(cpu);
        LoadY16(cpu, (uint16_t)(cpu->y - 1u));
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
}

Lufia2ExecutionResult Lufia2TextQueueWindowRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));                       /* C56E */
    PushY(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xc572u);
    TextWriteWindowRow(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    SetAccumulatorWidth(cpu, 0);
    LoadADirect16(memory, cpu, 0x54u);
    And16(cpu, 0x00ffu);
    AslA16(cpu); AslA16(cpu); AslA16(cpu);
    StoreADirect16(memory, cpu, 0x54u);
    AslA16(cpu);
    Add16Value(cpu, 0xd000u);
    StoreAAbsolute16(memory, cpu, SNES_A1TL(2), 0);
    LoadADirect16(memory, cpu, 0x54u);
    Add16Value(cpu, 0x1800u);
    StoreAAbsolute16(memory, cpu, 0x007du, 0);
    LoadADirect16(memory, cpu, 0x56u);
    cpu->carry = 0;
    Add16Value(cpu, 0x3040u);
    StoreAAbsolute16(memory, cpu, SNES_A1TL(1), 0);
    LoadADirect16(memory, cpu, 0x56u);
    LsrA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, 0x0820u);
    StoreAAbsolute16(memory, cpu, 0x007bu, 0);
    SetAccumulatorWidth(cpu, 1);
    LoadA8(cpu, 1);
    StoreAAbsolute8(memory, cpu, SNES_DMAP(2), 0);
    StoreAAbsolute8(memory, cpu, SNES_DMAP(1), 0);
    LoadA8(cpu, 0x7eu);
    StoreAAbsolute8(memory, cpu, SNES_A1B(2), 0);
    LoadA8(cpu, 0x7eu);
    StoreAAbsolute8(memory, cpu, SNES_A1B(1), 0);
    LoadA8(cpu, 0x18u);
    StoreAAbsolute8(memory, cpu, SNES_BBAD(2), 0);
    StoreAAbsolute8(memory, cpu, SNES_BBAD(1), 0);
    LoadX16(cpu, 0x0400u);
    Write16Absolute(memory, cpu, SNES_DASL(2), cpu->x);
    LoadX16(cpu, 0x0080u);
    Write16Absolute(memory, cpu, SNES_DASL(1), cpu->x);
    LoadA8(cpu, 0x42u);
    StoreADirect8(memory, cpu, 0x76u);
    LoadA8(cpu, 0x44u);
    StoreADirect8(memory, cpu, 0x77u);
    LoadA8(cpu, Read8(memory, 0x7fd087u));
    LoadA8(cpu, (uint8_t)(A8(cpu) + 1u));
    Write8(memory, 0x7fd087u, A8(cpu));
    cpu->y = PullIndexValue(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x80c5dcu);
}

/* $80:C23D: native placement, or exact C2A1 boundary before its first BF0B
 * frame wait. Its parent JSR, PHY, PHB and C274 JSR remain outstanding. */
Lufia2ExecutionResult Lufia2TextPrepareWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    LoadA8(cpu, 1);
    TestBitsAbsolute8(memory, cpu, 0x09a9u, 0);
    PushY(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x80u, 0xc246u);
    Lufia2TextClearGlyphBuffer(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    PushDataBank(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x09a7u, 0);
    BitImmediate8(cpu, 2);
    if (cpu->zero) {
        SimulateJsrFrame(memory, cpu, 0xc27bu);
        Lufia2TextBuildWindow(memory, cpu);
        SimulateRtsFrame(memory, cpu);
        LoadA8(cpu, 0x10u); TestBitsAbsolute8(memory, cpu, 0x099cu, 1);
        TransferDirectToA(cpu);
        Write8(memory, 0x7fd087u, A8(cpu));
        Write8(memory, 0x7fd088u, A8(cpu));
        LoadX16(cpu, 0xfffcu); Write16Absolute(memory, cpu, 0x059eu, cpu->x);
        PullDataBank(memory, cpu);
        cpu->y = PullIndexValue(memory, cpu);
        LoadA8(cpu, 1); TestBitsAbsolute8(memory, cpu, 0x09a9u, 1);
        LoadA8(cpu, 8); StoreADirect8(memory, cpu, 0x74u);
        LoadA8(cpu, 1); TestBitsAbsolute8(memory, cpu, 0x099cu, 1);
        return ExecutionReturned(0x80c2a0u);
    }
    LoadA8(cpu, 1);
    StoreAAbsolute8(memory, cpu, SNES_DMAP(0), 0);
    LoadX16(cpu, 0xd000u);
    Write16Absolute(memory, cpu, SNES_A1TL(0), cpu->x);
    LoadX16(cpu, 0x1800u);
    Write16Absolute(memory, cpu, 0x0079u, cpu->x);
    LoadA8(cpu, 0x7eu);
    StoreAAbsolute8(memory, cpu, SNES_A1B(0), 0);
    LoadA8(cpu, 0x18u);
    StoreAAbsolute8(memory, cpu, SNES_BBAD(0), 0);
    LoadX16(cpu, 0x0c00u);
    Write16Absolute(memory, cpu, SNES_DASL(0), cpu->x);
    LoadA8(cpu, 0x41u);
    StoreADirect8(memory, cpu, 0x75u);
    SimulateJsrFrame(memory, cpu, 0xc276u);
    return ExecutionHandoff(cpu, 0x80c2a1u);
}
