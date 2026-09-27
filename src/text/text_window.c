/* Glyph buffer and window row upload setup ($80:C56E, C784, C23D). */
#include "core/cpu_internal.h"
#include "lufia2/text.h"
#include "text/text_internal.h"

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

/* $80:C23D prefix. The unknown placement/frame-wait calls keep their frames. */
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
    if (cpu->zero)
        return ExecutionHandoff(cpu, 0x80c279u);
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
    return ExecutionHandoff(cpu, 0x80c274u);
}
