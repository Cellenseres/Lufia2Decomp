#include "core/cpu_ops.h"
#include "lufia2/field.h"

enum {
    MIRRORED_PLANES = 0x4e,
    TILE_DESTINATION_TABLE = 0x54,
    TILES_REMAINING = 0x58,
    METATILE_SOURCE = 0x5d,
    TILE_ATTRIBUTES = 0x65,
    ROWS_REMAINING = 0x91,
};

/* Reverse a byte's bits: one bitplane row flipped. */
static uint8_t MirrorBits(uint8_t value) {
    value = (uint8_t)(((value & 0x55u) << 1) | ((value >> 1) & 0x55u));
    value = (uint8_t)(((value & 0x33u) << 2) | ((value >> 2) & 0x33u));
    return (uint8_t)((value << 4) | (value >> 4));
}

Lufia2ExecutionResult Lufia2FieldMirrorPlaneByte(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    (void)memory;
    if (cpu->program_bank != 0x80u || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xf40au);
    const uint16_t original = cpu->accumulator;
    cpu->accumulator = MirrorBits((uint8_t)original);
    cpu->carry = (original & 0x8000u) != 0;
    SetNz8(cpu, (uint8_t)cpu->accumulator);
    return ExecutionReturned(0x80f429u);
}

/* Finish a simulated JSR; hand off on a foreign return. */
static Lufia2ExecutionResult CompleteLocalCall(const Lufia2Memory *memory,
                                               Lufia2CpuState *cpu, uint16_t expected) {
    const uint8_t low = Pull8(memory, cpu);
    const uint8_t high = Pull8(memory, cpu);
    const uint16_t actual = (uint16_t)(low | ((uint16_t)high << 8));
    const uint32_t next = ((uint32_t)cpu->program_bank << 16) | (uint16_t)(actual + 1u);
    return actual == expected ? ExecutionReturned(next) : ExecutionHandoff(cpu, next);
}

/* Calls the byte mirror through a simulated JSR frame. */
static Lufia2ExecutionResult MirrorPlaneByte(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu, uint16_t frame) {
    SimulateJsrFrame(memory, cpu, frame);
    Lufia2ExecutionResult result = Lufia2FieldMirrorPlaneByte(memory, cpu);
    if (result.flow != LUFIA2_EXECUTION_RETURNED)
        return result;
    return CompleteLocalCall(memory, cpu, frame);
}

Lufia2ExecutionResult Lufia2FieldMirrorPlaneWord(const Lufia2Memory *memory,
                                                 Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xf3f1u);
    OpSta(memory, cpu, OpDp(cpu, MIRRORED_PLANES));
    OpSepWidths(cpu, 0x20u);
    for (unsigned plane = 0; plane < 2u; ++plane) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpDp(cpu, (uint8_t)(MIRRORED_PLANES + plane)));
        Lufia2ExecutionResult result =
            MirrorPlaneByte(memory, cpu, plane == 0u ? 0xf3fau : 0xf402u);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        OpSta(memory, cpu, OpDp(cpu, (uint8_t)(MIRRORED_PLANES + plane)));
    }
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, MIRRORED_PLANES));
    return ExecutionReturned(0x80f409u);
}

/* Copy one plane word, mirrored when asked. */
static Lufia2ExecutionResult CopyPlaneWord(const Lufia2Memory *memory,
                                           Lufia2CpuState *cpu, uint16_t offset,
                                           uint16_t frame, uint8_t mirror) {
    OpLda(memory, cpu, OpAbsX(cpu, offset));
    if (mirror) {
        SimulateJsrFrame(memory, cpu, frame);
        Lufia2ExecutionResult result = Lufia2FieldMirrorPlaneWord(memory, cpu);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        result = CompleteLocalCall(memory, cpu, frame);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
    }
    OpSta(memory, cpu, OpAbsY(cpu, offset));
    return ExecutionReturned(0);
}

/* Shift DP $65 left: carry vertical flip, sign horizontal. */
static void ShiftTileAttributes(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    const uint32_t address = OpDp(cpu, TILE_ATTRIBUTES);
    const uint16_t original = OpRead16(memory, address);
    const uint16_t shifted = (uint16_t)(original << 1);
    cpu->carry = (original & 0x8000u) != 0;
    Write8(memory, OpNextByte(address), (uint8_t)(shifted >> 8));
    Write8(memory, address, (uint8_t)shifted);
    SetNz16(cpu, shifted);
}

/* Copy one flipped 8x8 4bpp tile. */
static Lufia2ExecutionResult CopyFlippedTile(const Lufia2Memory *memory,
                                             Lufia2CpuState *cpu, uint8_t vertical) {
    uint8_t horizontal = 1u;
    uint32_t row_pc = 0x80f38cu;
    if (vertical) {
        OpTya(cpu);
        cpu->carry = 0;
        OpAdcValue(cpu, 14u);
        OpTay(cpu);
        OpLsrMem(memory, cpu, OpDp(cpu, ROWS_REMAINING));
        OpLda(memory, cpu, OpDp(cpu, TILE_ATTRIBUTES));
        horizontal = cpu->negative;
        row_pc = horizontal ? 0x80f3c1u : 0x80f3abu;
    }
    unsigned rows = 0;
    do {
        if (rows++ == 0x10000u)
            return ExecutionHandoff(cpu, row_pc);
        Lufia2ExecutionResult result =
            CopyPlaneWord(memory, cpu, 0u, vertical ? 0xf3c6u : 0xf391u, horizontal);
        if (result.flow != LUFIA2_EXECUTION_RETURNED)
            return result;
        if (vertical) {
            result = CopyPlaneWord(memory, cpu, 16u, 0xf3cfu, horizontal);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
        }
        OpInx(cpu);
        OpInx(cpu);
        if (vertical) {
            OpDey(cpu);
            OpDey(cpu);
        } else {
            OpIny(cpu);
            OpIny(cpu);
        }
        OpStepMem(memory, cpu, OpDp(cpu, ROWS_REMAINING), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0);
}

Lufia2ExecutionResult Lufia2FieldCopyMetatileGraphics(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x80u || cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal)
        return ExecutionHandoff(cpu, ((uint32_t)cpu->program_bank << 16) | 0xf35bu);
    OpWriteX(memory, cpu, OpDp(cpu, METATILE_SOURCE), cpu->x);
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, TILES_REMAINING));
    unsigned tiles = 0;
    do {
        if (tiles++ == 4096u)
            return ExecutionHandoff(cpu, 0x80f362u);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, TILE_DESTINATION_TABLE)));
        OpLda(memory, cpu, OpLongX(cpu, 0x80f42au));
        OpTay(cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpWriteX(memory, cpu, OpDp(cpu, TILE_DESTINATION_TABLE), cpu->x);
        OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, METATILE_SOURCE)));
        OpLda(memory, cpu, OpLongX(cpu, 0x7f0000u));
        OpSta(memory, cpu, OpDp(cpu, TILE_ATTRIBUTES));
        OpAndValue(cpu, 0x03ffu);
        for (unsigned bit = 0; bit < 5u; ++bit)
            OpAslA(cpu);
        OpAdcValue(cpu, 0x4000u);
        OpTax(cpu);
        OpLoadA(cpu, 16u);
        OpSta(memory, cpu, OpDp(cpu, ROWS_REMAINING));
        ShiftTileAttributes(memory, cpu);
        if (cpu->carry || cpu->negative) {
            Lufia2ExecutionResult result = CopyFlippedTile(memory, cpu, cpu->carry);
            if (result.flow != LUFIA2_EXECUTION_RETURNED)
                return result;
        } else {
            OpLoadA(cpu, 31u);
            PushDataBank(memory, cpu);
            OpMoveNext(memory, cpu, 0x7eu, 0x7eu);
            PullDataBank(memory, cpu);
        }
        OpStepMem(memory, cpu, OpDp(cpu, METATILE_SOURCE), 1);
        OpStepMem(memory, cpu, OpDp(cpu, METATILE_SOURCE), 1);
        OpStepMem(memory, cpu, OpDp(cpu, TILES_REMAINING), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x80f3f0u);
}
