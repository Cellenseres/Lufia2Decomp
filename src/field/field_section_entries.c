#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/field.h"
#include "system/wram.h"

enum {
    SECTION_RECORDS = (WRAM_FIELD_SECTION_RECORDS & 0xffffu),
    SECTION_CELLS = (WRAM_FIELD_LAYER_CELL_BASE & 0xffffu),
    SECTION_WIDTHS = (WRAM_FIELD_LAYER_WIDTH & 0xffffu),
    SECTION_HEIGHTS = (WRAM_FIELD_LAYER_HEIGHT & 0xffffu),
    SECTION_HEADERS = (WRAM_FIELD_LAYER_SECTION_WORD & 0xffffu),
    SECTION_NEXT_SLOT = (WRAM_FIELD_SECTION_NEXT_SLOT & 0xffffu),
    SECTION_PACKED_CELLS = (WRAM_FIELD_PACKED_ATTRIBUTES & 0xffffu),
    SECTION_HEADER_BYTES = 6u,
    SECTION_CELL_OFFSET = 4u,
    SECTION_ATTRIBUTE_BITS = 0x30u,
    SECTION_SOURCE = 0x5du,
    SECTION_COUNT = 0x58u,
    SECTION_PACKED_BYTE = 0x54u,
    MAP_DESTINATION = 0x2du,
    MAP_RESOURCE_BASE = WRAM_FIELD_MAP_RESOURCE_BASE,
    MAP_SECTION_HEIGHT = WRAM_FIELD_SECTION_HEIGHT
};

static uint8_t SectionEntryReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x80u && !cpu->decimal;
}

static void ReadSectionSource(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint16_t pointer = Read16Direct(memory, cpu, SECTION_SOURCE);
    OpLda(memory, cpu, AbsoluteIndexedAddress(cpu, pointer, cpu->y));
}

static void LoadSectionRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, SECTION_SOURCE));
    OpSta(memory, cpu, OpAbsX(cpu, SECTION_RECORDS));
    LoadY16(cpu, 0u);
    ReadSectionSource(memory, cpu);
    OpSta(memory, cpu, OpAbsX(cpu, SECTION_HEADERS));
    OpSepWidths(cpu, 0x20u);
    LoadY16(cpu, 2u);
    ReadSectionSource(memory, cpu);
    OpSta(memory, cpu, OpAbsX(cpu, SECTION_WIDTHS));
    OpSta(memory, cpu, SNES_WRMPYA);
    IncrementY16(cpu);
    ReadSectionSource(memory, cpu);
    OpSta(memory, cpu, OpAbsX(cpu, SECTION_HEIGHTS));
    OpSta(memory, cpu, SNES_WRMPYB);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, SECTION_WIDTHS + 1u));
    OpSta(memory, cpu, OpAbsX(cpu, SECTION_HEIGHTS + 1u));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, SECTION_SOURCE));
    cpu->carry = 0u;
    OpAdcValue(cpu, SECTION_CELL_OFFSET);
    OpSta(memory, cpu, OpAbsX(cpu, SECTION_CELLS));
    cpu->carry = 0u;
    OpAdc(memory, cpu, SNES_RDMPYL);
    OpAdc(memory, cpu, SNES_RDMPYL);
    OpSta(memory, cpu, OpDp(cpu, SECTION_SOURCE));
    IncrementX16(cpu);
    IncrementX16(cpu);
    OpStepMem(memory, cpu, OpDp(cpu, SECTION_COUNT), -1);
}

Lufia2ExecutionResult Lufia2FieldLoadSectionRecords(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SectionEntryReady(cpu) || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x80ebaau);
    OpSetDataBank(memory, cpu, 0x7fu);
    LoadY16(cpu, 0u);
    ReadSectionSource(memory, cpu);
    OpSta(memory, cpu, OpDp(cpu, SECTION_COUNT));
    OpStz(memory, cpu, OpDp(cpu, SECTION_COUNT + 1u));
    OpRepWidths(cpu, 0x30u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SECTION_NEXT_SLOT, 0u));
    OpLda(memory, cpu, OpDp(cpu, SECTION_COUNT));
    AslA16(cpu);
    OpAdc(memory, cpu, OpAbs(cpu, SECTION_NEXT_SLOT));
    OpSta(memory, cpu, OpAbs(cpu, SECTION_NEXT_SLOT));
    OpLda(memory, cpu, OpDp(cpu, SECTION_SOURCE));
    cpu->carry = 0u;
    OpAdcValue(cpu, SECTION_HEADER_BYTES);
    OpSta(memory, cpu, OpDp(cpu, SECTION_SOURCE));
    do {
        LoadSectionRecord(memory, cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x80ec17u);
}

static void ReadCellAttribute(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, 1u));
    OpAndValue(cpu, SECTION_ATTRIBUTE_BITS);
}

static void AdvanceAttributeCell(Lufia2CpuState *cpu) {
    IncrementX16(cpu);
    IncrementX16(cpu);
}

static void PackFourCellAttributes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    ReadCellAttribute(memory, cpu);
    for (unsigned bit = 0u; bit < 4u; ++bit)
        LsrA8(cpu);
    OpSta(memory, cpu, OpDp(cpu, SECTION_PACKED_BYTE));
    AdvanceAttributeCell(cpu);
    ReadCellAttribute(memory, cpu);
    LsrA8(cpu);
    LsrA8(cpu);
    OpTestBits(memory, cpu, OpDp(cpu, SECTION_PACKED_BYTE), 1u);
    AdvanceAttributeCell(cpu);
    ReadCellAttribute(memory, cpu);
    OpTestBits(memory, cpu, OpDp(cpu, SECTION_PACKED_BYTE), 1u);
    AdvanceAttributeCell(cpu);
    ReadCellAttribute(memory, cpu);
    AslA8(cpu);
    AslA8(cpu);
    OpOra(memory, cpu, OpDp(cpu, SECTION_PACKED_BYTE));
    OpSta(memory, cpu, OpAbsY(cpu, SECTION_PACKED_CELLS));
    AdvanceAttributeCell(cpu);
    IncrementY16(cpu);
    OpCpy(cpu, Read16Direct(memory, cpu, SECTION_COUNT));
}

Lufia2ExecutionResult Lufia2FieldBuildPackedAttributes(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SectionEntryReady(cpu) || !cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x80ec18u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, WRAM_FIELD_LAYER_TABLE_OFFSET);
    TransferAToX(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, SECTION_WIDTHS));
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLda(memory, cpu, OpAbsX(cpu, SECTION_HEIGHTS));
    OpSta(memory, cpu, SNES_WRMPYB);
    OpRepWidths(cpu, 0x20u);
    LoadX16(cpu, Read16AbsoluteIndexed(memory, cpu, SECTION_CELLS, cpu->x));
    OpLda(memory, cpu, SNES_RDMPYL);
    cpu->carry = 0u;
    OpAdcValue(cpu, 3u);
    LsrA16(cpu);
    LsrA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, SECTION_COUNT));
    OpSepWidths(cpu, 0x20u);
    LoadY16(cpu, 0u);
    do {
        PackFourCellAttributes(memory, cpu);
    } while (!cpu->zero);
    return ExecutionReturned(0x80ec77u);
}

Lufia2ExecutionResult Lufia2FieldPublishSectionSize(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SectionEntryReady(cpu))
        return ExecutionHandoff(cpu, 0x80ec78u);
    OpSepWidths(cpu, 0x20u);
    TransferDirectToA(cpu);
    OpLda(memory, cpu, WRAM_FIELD_LAYER_TABLE_OFFSET);
    TransferAToX(cpu);
    OpLda(memory, cpu, OpAbsX(cpu, SECTION_WIDTHS));
    OpSta(memory, cpu, WRAM_FIELD_SECTION_WIDTH);
    OpLda(memory, cpu, OpAbsX(cpu, SECTION_HEIGHTS));
    OpSta(memory, cpu, MAP_SECTION_HEIGHT);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, WRAM_FIELD_SECTION_WIDTH + 1u);
    OpSta(memory, cpu, MAP_SECTION_HEIGHT + 1u);
    return ExecutionReturned(0x80ec97u);
}

Lufia2ExecutionResult Lufia2FieldAdvanceMapDestination(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SectionEntryReady(cpu))
        return ExecutionHandoff(cpu, 0x80ecf2u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, MAP_DESTINATION));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, SECTION_COUNT));
    OpSta(memory, cpu, OpDp(cpu, MAP_DESTINATION));
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x80ecfdu);
}

Lufia2ExecutionResult Lufia2FieldResolveMapOffset(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SectionEntryReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x80ecfeu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, MAP_RESOURCE_BASE);
    TransferAToX(cpu);
    OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
    cpu->carry = 0u;
    OpAdc(memory, cpu, MAP_RESOURCE_BASE);
    return ExecutionReturned(0x80ed0du);
}
