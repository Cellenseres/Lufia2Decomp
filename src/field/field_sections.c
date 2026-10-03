/* Map section tables ($80:EBAA-$80:ED0D). */

#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "core/wram_view.h"
#include "field/field_internal.h"
#include "lufia2/system.h"
#include "system/dp_scratch.h"
#include "system/wram.h"

/* Section table in bank $7F, one slot per section. */
enum {
    SECTION_RECORD = 0xd000u,         /* address of the section record */
    SECTION_ATTRIBUTE_DATA = 0xd008u, /* address of the attribute cells */
    SECTION_WIDTH = 0xd010u,
    SECTION_HEIGHT = 0xd018u,
    SECTION_FIRST_WORD = 0xd020u,
    SECTION_NEXT_SLOT = 0xd038u, /* number of used slots, times two */
    SECTION_RECORD_HEADER = 6u,  /* bytes before the first record */
    SECTION_DATA_OFFSET = 4u,    /* attribute cells follow word, width, height */
    SECTION_PACKED_ATTRIBUTES = 0xc000u,
    WRAM_FIELD_SECTION_HEIGHT = 0x05bbu
};

/* Work bytes. */
enum {
    SECTION_DP_SOURCE = 0x5du, /* read pointer into the map header */
    SECTION_DP_COUNT = 0x58u,
    SECTION_DP_PACKED = 0x54u
};

/* Section record byte or word via the data bank. */
static uint8_t SectionByte(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
                           uint16_t record, uint16_t offset) {
    return Read8(memory, AbsoluteIndexedAddress(cpu, record, offset));
}

static uint16_t SectionWord(const Lufia2Memory *memory, const Lufia2CpuState *cpu,
                            uint16_t record, uint16_t offset) {
    return Read16AbsoluteIndexed(memory, cpu, record, offset);
}

/* Attribute bits 4-5 of the cell at offset cell. */
static uint8_t SectionAttributeBits(const Lufia2Memory *memory,
                                    const Lufia2CpuState *cpu, uint16_t cell) {
    return SectionByte(memory, cpu, 1u, cell) & 0x30u;
}

/* Read the map header's sections into $7F:D000. */
Lufia2ExecutionResult Lufia2FieldReadSections(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram dp = WramViewOfCaller(memory, cpu);
    const Lufia2Wram registers = WramViewLong(memory);
    Lufia2Wram map;
    uint16_t record, slot, product;
    uint8_t count;

    OpSetDataBank(memory, cpu, 0x7fu);
    map = WramViewOfCaller(memory, cpu);
    record = WramRead16(dp, SECTION_DP_SOURCE);
    count = SectionByte(memory, cpu, record, 0u);
    LoadA8(cpu, count);
    WramWrite(dp, SECTION_DP_COUNT, count);
    WramWrite(dp, SECTION_DP_COUNT + 1u, 0u);

    slot = WramRead16(map, SECTION_NEXT_SLOT);
    WramWrite16(map, SECTION_NEXT_SLOT, (uint16_t)(slot + 2u * count));
    record = (uint16_t)(record + SECTION_RECORD_HEADER);
    WramWrite16(dp, SECTION_DP_SOURCE, record);

    do {
        const uint8_t width = SectionByte(memory, cpu, record, 2u);
        const uint8_t height = SectionByte(memory, cpu, record, 3u);

        WramWrite16At(map, SECTION_RECORD, slot, record);
        WramWrite16At(map, SECTION_FIRST_WORD, slot,
                      SectionWord(memory, cpu, record, 0u));
        WramWrite(map, SECTION_WIDTH + slot, width);
        WramWrite(registers, SNES_WRMPYA, width);
        WramWrite(map, SECTION_HEIGHT + slot, height);
        WramWrite(registers, SNES_WRMPYB, height);
        WramWrite(map, SECTION_WIDTH + slot + 1u, (uint8_t)cpu->direct_page);
        WramWrite(map, SECTION_HEIGHT + slot + 1u, (uint8_t)cpu->direct_page);

        /* The next record starts after the cells: two bytes each. */
        WramWrite16At(map, SECTION_ATTRIBUTE_DATA, slot,
                      (uint16_t)(record + SECTION_DATA_OFFSET));
        product = WramRead16(registers, SNES_RDMPYL);
        LoadA16(cpu, (uint16_t)(record + SECTION_DATA_OFFSET));
        cpu->carry = 0;
        Add16Value(cpu, product);
        Add16Value(cpu, WramRead16(registers, SNES_RDMPYL));
        record = cpu->accumulator;
        WramWrite16(dp, SECTION_DP_SOURCE, record);
        slot = (uint16_t)(slot + 2u);
        count = (uint8_t)(count - 1u);
        SetNz16(cpu, WramStep16(dp, SECTION_DP_COUNT, -1));
    } while (!cpu->zero);
    cpu->x = slot;
    cpu->y = 3u;
    cpu->index_is_8_bit = 0;
    SetAccumulatorWidth(cpu, 1);
    return ExecutionReturned(0x80ec17u);
}

/* Pack section $05AA's attribute bits into $7F:C000. */
Lufia2ExecutionResult Lufia2FieldPackSectionAttributes(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram registers = WramViewLong(memory); /* bank $00 */
    uint16_t cell, group = 0, groups;
    uint8_t packed;

    TransferDirectToA(cpu);
    LoadA8(cpu, WramRead(registers, WRAM_FIELD_LAYER_TABLE_OFFSET));
    TransferAToX(cpu);
    WramWrite(registers, SNES_WRMPYA, WramReadAt(wram, SECTION_WIDTH, cpu->x));
    WramWrite(registers, SNES_WRMPYB, WramReadAt(wram, SECTION_HEIGHT, cpu->x));
    cell = WramRead16At(wram, SECTION_ATTRIBUTE_DATA, cpu->x);
    LoadA16(cpu, WramRead16(registers, SNES_RDMPYL));
    cpu->carry = 0;
    Add16Value(cpu, 3u); /* round up to whole groups of four cells */
    LsrA16(cpu);
    LsrA16(cpu);
    groups = cpu->accumulator;
    WramWrite16(wram, SECTION_DP_COUNT, groups);
    SetAccumulatorWidth(cpu, 1);

    do {
        packed = (uint8_t)(SectionAttributeBits(memory, cpu, cell) >> 4);

        WramWrite(wram, SECTION_DP_PACKED, packed);
        cell = (uint16_t)(cell + 2u);
        packed |= (uint8_t)(SectionAttributeBits(memory, cpu, cell) >> 2);
        WramWrite(wram, SECTION_DP_PACKED, packed);
        cell = (uint16_t)(cell + 2u);
        packed |= SectionAttributeBits(memory, cpu, cell);
        WramWrite(wram, SECTION_DP_PACKED, packed);
        cell = (uint16_t)(cell + 2u);
        packed |= (uint8_t)(SectionAttributeBits(memory, cpu, cell) << 2);
        cell = (uint16_t)(cell + 2u);
        Write8(memory, AbsoluteIndexedAddress(cpu, SECTION_PACKED_ATTRIBUTES, group),
               packed);
        ++group;
        Compare16(cpu, group, WramRead16(wram, SECTION_DP_COUNT));
    } while (!cpu->zero);

    /* Exit: last packed byte in A, byte count in Y. */
    cpu->accumulator = (uint16_t)((groups & 0xff00u) | packed);
    cpu->x = cell;
    cpu->y = group;
    return ExecutionReturned(0x80ec77u);
}

/* Publish section $05AA's size at $05B9 and $05BB. */
Lufia2ExecutionResult Lufia2FieldSectionSize(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    const Lufia2Wram wram = WramViewOfCaller(memory, cpu);
    const Lufia2Wram registers = WramViewLong(memory); /* bank $00 */

    SetAccumulatorWidth(cpu, 1);
    TransferDirectToA(cpu);
    LoadA8(cpu, WramRead(registers, WRAM_FIELD_LAYER_TABLE_OFFSET));
    TransferAToX(cpu);
    LoadA8(cpu, WramReadAt(wram, SECTION_WIDTH, cpu->x));
    WramWrite(registers, WRAM_FIELD_SECTION_WIDTH, A8(cpu));
    LoadA8(cpu, WramReadAt(wram, SECTION_HEIGHT, cpu->x));
    WramWrite(registers, WRAM_FIELD_SECTION_HEIGHT, A8(cpu));
    TransferDirectToA(cpu);
    WramWrite(registers, WRAM_FIELD_SECTION_WIDTH + 1u, A8(cpu));
    WramWrite(registers, WRAM_FIELD_SECTION_HEIGHT + 1u, A8(cpu));
    return ExecutionReturned(0x80ec97u);
}

/* JSL $80:8E9D from bank $80. */
static void FieldDecompress(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    SimulateJslFrame(memory, cpu, 0x80u, (uint16_t)(site + 3u));
    (void)Lufia2DecompressResource(memory, cpu);
    SimulateRtlFrame(memory, cpu);
}

/* $80:ECF2: $2D += $58 (the decompressed size). */
static void FieldAdvanceDestination(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    OpRepWidths(cpu, 0x20u);                                         /* ECF2 */
    OpLda(memory, cpu, OpDp(cpu, 0x2du));
    cpu->carry = 0;
    OpAdc(memory, cpu, OpDp(cpu, DP_SCRATCH_E));
    OpSta(memory, cpu, OpDp(cpu, 0x2du));
    OpSepWidths(cpu, 0x20u);
    SimulateRtsFrame(memory, cpu);                             /* ECFD */
}

/* $80:ECFE (M0): A = [$7F:(A + $D03A)] + $D03A. */
static void FieldRelativeWord(
    const Lufia2Memory *memory, Lufia2CpuState *cpu, uint16_t site) {
    SimulateJsrFrame(memory, cpu, (uint16_t)(site + 2u));
    cpu->carry = 0;                                            /* ECFE */
    OpAdc(memory, cpu, 0x7fd03au);
    OpTax(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
    cpu->carry = 0;
    OpAdc(memory, cpu, 0x7fd03au);
    SimulateRtsFrame(memory, cpu);                             /* ED0D */
}

Lufia2ExecutionResult Lufia2FieldDecompressMapData(
    const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A)); /* EC98 */
    OpLda(memory, cpu, OpDp(cpu, 0x2du));
    OpSta(memory, cpu, 0x7fd03au);
    OpSta(memory, cpu, OpDp(cpu, 0x60u));
    OpSepWidths(cpu, 0x20u);
    LoadA8(cpu, 0x7fu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    FieldDecompress(memory, cpu, 0xeca8u);
    FieldAdvanceDestination(memory, cpu, 0xecacu);
    OpRepWidths(cpu, 0x20u);                                         /* ECAF */
    OpLda(memory, cpu, 0x7fd03au);
    cpu->carry = 0;
    OpAdcValue(cpu, 0x0010u);
    OpSta(memory, cpu, WRAM_FIELD_METATILE_BASE);
    LoadA16(cpu, 0x0004u);
    FieldRelativeWord(memory, cpu, 0xecc0u);
    OpSta(memory, cpu, WRAM_FIELD_METATILE_ATTRIBUTE_BASE);
    OpSepWidths(cpu, 0x20u);                                         /* ECC7 */
    TransferDirectToA(cpu);
    OpLda(memory, cpu, OpDp(cpu, 0x29u));
    if (!cpu->zero) {
        OpAndValue(cpu, 0x0fu);                                /* ECCE */
        OpRepWidths(cpu, 0x20u);
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0166u);
        OpSta(memory, cpu, OpDp(cpu, DP_SCRATCH_A));
        OpLda(memory, cpu, OpDp(cpu, 0x2du));
        OpSta(memory, cpu, OpDp(cpu, 0x60u));
        cpu->carry = 0;
        OpAdcValue(cpu, 0x0010u);
        OpSta(memory, cpu, WRAM_FIELD_ALTERNATE_METATILE_BASE);
        OpSepWidths(cpu, 0x20u);
        LoadA8(cpu, 0x7fu);
        OpSta(memory, cpu, OpDp(cpu, 0x62u));
        FieldDecompress(memory, cpu, 0xeceau);
        FieldAdvanceDestination(memory, cpu, 0xeceeu);
    }
    return ExecutionReturned(0x80ecf1u);                       /* ECF1 */
}
