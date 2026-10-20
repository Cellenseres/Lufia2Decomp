#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/text.h"

enum {
    ROM_DISPLAY_TEXT = 0x919affu,
    ROM_DISPLAY_GLYPHS = 0x8eb59bu,
    DISPLAY_TILEMAP = 0x2000u,
    DISPLAY_SECOND_ROW = 0x2040u,
    DISPLAY_TILE_STRIP = 0x2012u,
    DISPLAY_STRIP_SECOND_ROW = 0x2052u,
    DP_STRIP_COLUMNS = 0x54u,
    DP_CLEAR_COLUMNS = 0x56u,
    DP_TEXT_LENGTH = 0x58u,
    DP_TEXT_LENGTH_HIGH = 0x59u,
    DP_DISPLAY_ROW = 0x60u,
    DP_TEXT_STATE = 0x65u
};

static bool DisplayEntry(const Lufia2CpuState *cpu, uint8_t byte_tiles) {
    return cpu->program_bank == 0x8eu && !cpu->direct_page &&
           cpu->accumulator_is_8_bit == byte_tiles &&
           !cpu->index_is_8_bit && !cpu->decimal;
}

Lufia2ExecutionResult Lufia2AlternateDisplayMeasureText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 1u))
        return ExecutionHandoff(cpu, 0x8eb4fcu);
    TransferDirectToA(cpu);
    OpPushX(memory, cpu);
    OpStz(memory, cpu, OpDp(cpu, DP_TEXT_LENGTH));
    OpStz(memory, cpu, OpDp(cpu, DP_TEXT_LENGTH_HIGH));
    for (;;) {
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT));
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_TEXT_LENGTH), 1);
    }
    OpPullX(memory, cpu);
    return ExecutionReturned(0x8eb50eu);
}

Lufia2ExecutionResult Lufia2AlternateDisplayDrawText(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 1u))
        return ExecutionHandoff(cpu, 0x8eb50fu);
    for (;;) {
        TransferDirectToA(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_TEXT));
        if (cpu->zero)
            break;
        OpPushX(memory, cpu);
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x20u);
        OpTax(cpu);
        OpLda(memory, cpu, OpLongX(cpu, ROM_DISPLAY_GLYPHS));
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_TILEMAP));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_SECOND_ROW));
        OpPullX(memory, cpu);
        OpIny(cpu);
        OpIny(cpu);
        OpInx(cpu);
    }
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, DP_TEXT_STATE));
    OpInx(cpu);
    return ExecutionReturned(0x8eb533u);
}

Lufia2ExecutionResult Lufia2AlternateDisplayDrawTileStrip(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 0u))
        return ExecutionHandoff(cpu, 0x8eb534u);
    Push8(memory, cpu, (uint8_t)(cpu->accumulator >> 8));
    Push8(memory, cpu, (uint8_t)cpu->accumulator);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS));
    PullAccumulator16(memory, cpu);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_TILE_STRIP));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x10u);
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_STRIP_SECOND_ROW));
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x0eu);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_STRIP_COLUMNS), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x8eb54fu);
}

Lufia2ExecutionResult Lufia2AlternateDisplayUploadTilemap(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 1u))
        return ExecutionHandoff(cpu, 0x8eb557u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_VMADDL), cpu->x);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_DASL(0)), cpu->y);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DMAP(0)));
    OpLdx(cpu, 0x6000u);
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_A1TL(0)), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_A1B(0)));
    OpLoadA(cpu, 0x18u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_BBAD(0)));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, SNES_MDMAEN));
    return ExecutionReturned(0x8eb577u);
}

Lufia2ExecutionResult Lufia2AlternateDisplayAdvanceRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!DisplayEntry(cpu, 1u))
        return ExecutionHandoff(cpu, 0x8eb578u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, DP_DISPLAY_ROW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x80u);
    OpAndValue(cpu, 0x0fffu);
    OpSta(memory, cpu, OpDp(cpu, DP_DISPLAY_ROW));
    OpTay(cpu);
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, DP_CLEAR_COLUMNS));
    TransferDirectToA(cpu);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_TILEMAP));
        OpSta(memory, cpu, OpAbsY(cpu, DISPLAY_SECOND_ROW));
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DP_CLEAR_COLUMNS), -1);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    return ExecutionReturned(0x8eb59au);
}
