/* Item and spell records ($81:F1C5, $81:F414). */

#include "core/cpu_internal.h"
#include "lufia2/item.h"

enum {
    ITEM = 0x0a06u,
    ITEM_RECORD = 0x0a09u,              /* pointer into bank $96 */
    SPELL = 0x0a0bu,
    SPELL_RECORD = 0x0a0du,             /* pointer into bank $95 */
    MASKS = 0x09fcu,                    /* 4 bytes: which words follow */
    MASK_BITS = 0x0a00u,
    NAME = 0x0b77u,
    FIELDS = 0x0b84u,
};

static void DecrementX(Lufia2CpuState *cpu) {
    cpu->x = (uint16_t)(cpu->x - 1u);
    SetNz16(cpu, cpu->x);
}

/* STA abs,X with a 16-bit accumulator. */
static void StoreA16Indexed(
    const Lufia2Memory *memory,
    const Lufia2CpuState *cpu,
    uint16_t address) {
    const uint32_t low = AbsoluteIndexedAddress(cpu, address, cpu->x);

    Write8(memory, low, (uint8_t)cpu->accumulator);
    Write8(memory, (low + 1u) & 0x00ffffffu,
        (uint8_t)(cpu->accumulator >> 8));
}

/* $81:F2A9: 12-character item name, trailing spaces cleared. */
static void ItemName(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x9eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 0);                               /* F2AE */
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, ITEM, 0));
    And16(cpu, 0x01ffu);
    AslA16(cpu);
    AslA16(cpu);
    PushAccumulator16(memory, cpu);
    AslA16(cpu);
    cpu->carry = 0;
    Add16Value(cpu, (uint16_t)(Read8(memory, (uint16_t)(cpu->stack + 1u)) |
        (Read8(memory, (uint16_t)(cpu->stack + 2u)) << 8)));   /* ADC $01,s */
    cpu->y = PullIndexValue(memory, cpu);
    TransferAToY(cpu);                                         /* item * 12 */
    LoadX16(cpu, 0x0000u);
    do {
        LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, 0xc7e8u, cpu->y));
        StoreA16Indexed(memory, cpu, NAME);
        IncrementY16(cpu);
        IncrementY16(cpu);
        IncrementX16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x000cu);
    } while (!cpu->zero);
    SetAccumulatorWidth(cpu, 1);
    StoreZeroAbsolute8(memory, cpu, NAME, cpu->x);             /* F2D3 */
    do {
        LoadAAbsolute8(memory, cpu, NAME, cpu->x);
        if (!cpu->zero) {
            Compare8(cpu, A8(cpu), 0x20u);
            if (!cpu->zero)
                break;
            StoreZeroAbsolute8(memory, cpu, NAME, cpu->x);
        }
        DecrementX(cpu);                                       /* F2E2 */
    } while (!cpu->negative);
    PullDataBank(memory, cpu);
}

/* $81:F291: item record pointer, $96:CF69 table. */
static void ItemRecordAddress(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 0);
    SetIndexWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, ITEM, 0));
    And16(cpu, 0x01ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x96cf69u, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0xcf69u);
    Write16Absolute(memory, cpu, ITEM_RECORD, cpu->accumulator);
    UnpackStatus(cpu, Pull8(memory, cpu));
}

/* $81:F1C5: name, 9 bytes, then masked words. */
Lufia2ExecutionResult Lufia2LoadItemRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    unsigned mask;

    Push8(memory, cpu, PackStatus(cpu));                       /* F1C5 */
    PushDataBank(memory, cpu);
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    LoadX16(cpu, 0x0badu);
    do {
        DecrementX(cpu);
        StoreZeroAbsolute8(memory, cpu, 0x0000u, cpu->x);
        Compare16(cpu, cpu->x, NAME);
    } while (!cpu->zero);
    SimulateJslFrame(memory, cpu, 0x81u, 0xf1dau);
    ItemName(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadA8(cpu, 0x96u);                                        /* F1DB */
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SimulateJslFrame(memory, cpu, 0x81u, 0xf1e2u);
    ItemRecordAddress(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, ITEM_RECORD, 0));
    LoadX16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        StoreAAbsolute8(memory, cpu, FIELDS, cpu->x);
        IncrementY16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0009u);
    } while (!cpu->zero);
    for (mask = 0; mask < 4u; ++mask) {                        /* F1F6 */
        LoadAAbsolute8(memory, cpu, (uint16_t)mask, cpu->y);
        StoreAAbsolute8(memory, cpu, (uint16_t)(MASKS + mask), 0);
    }
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    IncrementY16(cpu);
    for (mask = 0; mask < 4u; ++mask) {                        /* F212 */
        const uint32_t bits = AbsoluteIndexedAddress(
            cpu, (uint16_t)(MASKS + mask), 0);

        StoreA8Absolute(memory, cpu, MASK_BITS, 0x08u);
        do {
            const uint8_t value = Read8(memory, bits);         /* LSR */

            cpu->carry = value & 1u;
            Write8(memory, bits, (uint8_t)(value >> 1));
            if (cpu->carry) {
                LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
                IncrementY16(cpu);
                StoreAAbsolute8(memory, cpu, FIELDS, cpu->x);
                LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
                IncrementY16(cpu);
                StoreAAbsolute8(memory, cpu, (uint16_t)(FIELDS + 1u), cpu->x);
            }
            IncrementX16(cpu);
            IncrementX16(cpu);
            {
                const uint32_t count =
                    AbsoluteIndexedAddress(cpu, MASK_BITS, 0);
                const uint8_t left = (uint8_t)(Read8(memory, count) - 1u);

                Write8(memory, count, left);
                SetNz8(cpu, left);
            }
        } while (!cpu->zero);
    }
    PullDataBank(memory, cpu);                                 /* F28E */
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f290u);
}

/* $81:F446: spell record pointer, $95:FA5B table. */
static void SpellRecordAddress(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    SetAccumulatorWidth(cpu, 0);
    LoadA16(cpu, Read16AbsoluteIndexed(memory, cpu, SPELL, 0));
    And16(cpu, 0x00ffu);
    AslA16(cpu);
    TransferAToX(cpu);
    LoadA16(cpu, Read16Long(memory, LongIndexedAddress(0x95fa5bu, cpu->x)));
    cpu->carry = 0;
    Add16Value(cpu, 0xfa5bu);
    Write16Absolute(memory, cpu, SPELL_RECORD, cpu->accumulator);
    SetAccumulatorWidth(cpu, 1);
}

/* $81:F414: name, D low byte as terminator, 11 bytes. */
Lufia2ExecutionResult Lufia2LoadSpellRecord(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    PushDataBank(memory, cpu);                                 /* F414 */
    LoadA8(cpu, 0x95u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    SimulateJsrFrame(memory, cpu, 0xf41bu);
    SpellRecordAddress(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, SPELL_RECORD, 0));
    LoadX16(cpu, 0x0000u);
    do {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, LongIndexedAddress(NAME, cpu->x), A8(cpu));
        IncrementY16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0008u);
    } while (!cpu->zero);
    TransferDirectToA(cpu);                                    /* TDC */
    Write8(memory, LongIndexedAddress(NAME, cpu->x), A8(cpu));
    IncrementX16(cpu);
    do {
        LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
        Write8(memory, LongIndexedAddress(NAME, cpu->x), A8(cpu));
        IncrementY16(cpu);
        IncrementX16(cpu);
        Compare16(cpu, cpu->x, 0x0014u);
    } while (!cpu->zero);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x81f445u);
}

/* $81:F194: A = first record byte of item $0A06. */
Lufia2ExecutionResult Lufia2ItemRecordByte(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    Push8(memory, cpu, PackStatus(cpu));
    SetAccumulatorWidth(cpu, 1);
    SetIndexWidth(cpu, 0);
    SimulateJslFrame(memory, cpu, 0x81u, 0xf19cu);
    ItemRecordAddress(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, ITEM_RECORD, 0));
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x96u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadAAbsolute8(memory, cpu, 0x0000u, cpu->y);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x81f1aau);
}

static Lufia2ExecutionResult SpellRecordAt(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint16_t return_address, uint16_t offset,
    uint32_t rtl) {
    SimulateJsrFrame(memory, cpu, return_address);
    SpellRecordAddress(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    LoadY16(cpu, Read16AbsoluteIndexed(memory, cpu, SPELL_RECORD, 0));
    PushDataBank(memory, cpu);
    LoadA8(cpu, 0x95u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    LoadAAbsolute8(memory, cpu, offset, cpu->y);
    PullDataBank(memory, cpu);
    return ExecutionReturned(rtl);
}

/* $81:F3F4: A = spell record byte $0C. */
Lufia2ExecutionResult Lufia2SpellRecordByteC(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return SpellRecordAt(memory, cpu, 0xf3f6u, 0x000cu, 0x81f403u);
}

/* $81:F404: A = spell record byte 8. */
Lufia2ExecutionResult Lufia2SpellRecordByte8(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    return SpellRecordAt(memory, cpu, 0xf406u, 0x0008u, 0x81f413u);
}
