#include "cave/wram.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "lufia2/ancient_cave.h"

enum {
    DIAGNOSTIC_TILEMAP = 0x7e3000u,
    DIAGNOSTIC_ROW_LEFT = 0x54u,
    DIAGNOSTIC_VALUE = 0x55u,
    DIAGNOSTIC_GLYPH_BASE = 0x44u,
    DIAGNOSTIC_ATTRIBUTE = 0x20u,
    DIAGNOSTIC_MAP_START = 0x02deu,
    DIAGNOSTIC_ROW_WIDTH = 16u,
    DIAGNOSTIC_LINK_LIMIT = 64u
};

static void DrawDiagnosticValue(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpLongX(cpu, DIAGNOSTIC_TILEMAP));
    OpLoadA(cpu, 0x1eu);
    OpSta(memory, cpu, OpLongX(cpu, DIAGNOSTIC_TILEMAP + 2u));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DIAGNOSTIC_VALUE));
    for (unsigned shift = 0u; shift < 4u; ++shift)
        OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x20u);
    OpSta(memory, cpu, OpLongX(cpu, DIAGNOSTIC_TILEMAP + 4u));
    OpLda(memory, cpu, OpDp(cpu, DIAGNOSTIC_VALUE));
    OpAndValue(cpu, 0x0fu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x20u);
    OpSta(memory, cpu, OpLongX(cpu, DIAGNOSTIC_TILEMAP + 6u));
    OpLoadA(cpu, DIAGNOSTIC_ATTRIBUTE);
    for (unsigned cell = 0u; cell < 4u; ++cell)
        OpSta(memory, cpu, OpLongX(cpu, DIAGNOSTIC_TILEMAP + 1u + cell * 2u));
}

Lufia2ExecutionResult Lufia2CaveDrawDiagnosticValue(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x8eb23fu);
    DrawDiagnosticValue(memory, cpu);
    return ExecutionReturned(0x8eb274u);
}

static void DivideDiagnosticCoordinate(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpSta(memory, cpu, SNES_WRDIVL);
    TransferDirectToA(cpu);
    OpSta(memory, cpu, SNES_WRDIVH);
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, SNES_WRDIVB);
    SimulateJsrFrame(memory, cpu, 0xb286u);
    SimulateRtsFrame(memory, cpu);
}

Lufia2ExecutionResult Lufia2CaveDivideDiagnosticCoordinate(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x8eb275u);
    DivideDiagnosticCoordinate(memory, cpu);
    return ExecutionReturned(0x8eb287u);
}

static void DrawRoomGrid(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLoadA(cpu, DIAGNOSTIC_ROW_WIDTH);
    OpSta(memory, cpu, OpDp(cpu, DIAGNOSTIC_ROW_LEFT));
    OpLdx(cpu, DIAGNOSTIC_MAP_START);
    OpLdy(cpu, 0u);
    for (;;) {
        OpLda(memory, cpu, OpAbsY(cpu, CAVE_ROOM_GRID));
        cpu->carry = 0u;
        OpAdcValue(cpu, DIAGNOSTIC_GLYPH_BASE);
        OpCmpValue(cpu, DIAGNOSTIC_GLYPH_BASE);
        if (cpu->zero)
            TransferDirectToA(cpu);
        OpSta(memory, cpu, OpLongX(cpu, DIAGNOSTIC_TILEMAP));
        OpLoadA(cpu, DIAGNOSTIC_ATTRIBUTE);
        OpSta(memory, cpu, OpLongX(cpu, DIAGNOSTIC_TILEMAP + 1u));
        OpIny(cpu);
        OpCpy(cpu, CAVE_GRID_BYTES);
        if (cpu->zero)
            break;
        OpInx(cpu);
        OpInx(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, DIAGNOSTIC_ROW_LEFT), -1);
        if (!cpu->zero)
            continue;
        OpRepWidths(cpu, 0x20u);
        OpTxa(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x20u);
        OpTax(cpu);
        OpSepWidths(cpu, 0x20u);
        OpLoadA(cpu, DIAGNOSTIC_ROW_WIDTH);
        OpSta(memory, cpu, OpDp(cpu, DIAGNOSTIC_ROW_LEFT));
    }
}

static void DrawStairMarker(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, CAVE_STAIR_COLUMN));
    SimulateJsrFrame(memory, cpu, 0xb198u);
    DivideDiagnosticCoordinate(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    OpLda(memory, cpu, SNES_RDDIVL);
    OpAslA(cpu);
    OpSta(memory, cpu, OpDp(cpu, DIAGNOSTIC_ROW_LEFT));
    OpStz(memory, cpu, OpDp(cpu, DIAGNOSTIC_VALUE));
    OpLda(memory, cpu, OpAbs(cpu, CAVE_STAIR_ROW));
    SimulateJsrFrame(memory, cpu, 0xb1a7u);
    DivideDiagnosticCoordinate(memory, cpu);
    SimulateRtsFrame(memory, cpu);
    OpLda(memory, cpu, SNES_RDDIVL);
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, 0u);
    OpRepWidths(cpu, 0x20u);
    OpLsrA(cpu);
    OpLsrA(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, DIAGNOSTIC_ROW_LEFT));
    OpAdcValue(cpu, 0x031eu);
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x11u);
    OpSta(memory, cpu, OpLongX(cpu, DIAGNOSTIC_TILEMAP));
}

static void PrintDiagnosticValue(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, uint8_t label, uint16_t returned) {
    ExchangeAccumulatorBytes(cpu);
    OpLoadA(cpu, label);
    SimulateJsrFrame(memory, cpu, returned);
    DrawDiagnosticValue(memory, cpu);
    SimulateRtsFrame(memory, cpu);
}

static void DrawDiagnosticValues(const Lufia2Memory *memory,
    Lufia2CpuState *cpu) {
    OpLdx(cpu, 0x04c2u);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_FLOOR));
    OpDecA(cpu);
    PrintDiagnosticValue(memory, cpu, 0x35u, 0xb1ceu);
    OpLdx(cpu, 0x0502u);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_STORY_CHEST_STATE));
    PrintDiagnosticValue(memory, cpu, 0x32u, 0xb1dau);
    OpLdx(cpu, 0x0542u);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_SCROLL_CHEST_STATE_LONG & 0xffffu));
    PrintDiagnosticValue(memory, cpu, 0x39u, 0xb1e6u);
    OpLdx(cpu, 0x0582u);
    OpLda(memory, cpu, WRAM_FIELD_REWARD_COUNT_B);
    PrintDiagnosticValue(memory, cpu, 0x3bu, 0xb1f3u);
    OpLdx(cpu, 0x05c2u);
    OpLda(memory, cpu, WRAM_FIELD_REWARD_COUNT_A);
    PrintDiagnosticValue(memory, cpu, 0x2bu, 0xb200u);
    OpLdx(cpu, 0x0602u);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_OBJECT_COUNT));
    PrintDiagnosticValue(memory, cpu, 0x36u, 0xb20cu);
    OpLdx(cpu, 0x0642u);
    OpLda(memory, cpu, OpAbs(cpu, CAVE_CHEST_COUNT));
    PrintDiagnosticValue(memory, cpu, 0x3du, 0xb218u);
    OpStz(memory, cpu, OpDp(cpu, DIAGNOSTIC_ROW_LEFT));
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, CAVE_LINKS));
        OpCmpValue(cpu, 0xffu);
        if (cpu->zero)
            break;
        OpStepMem(memory, cpu, OpDp(cpu, DIAGNOSTIC_ROW_LEFT), 1);
        OpInx(cpu);
        OpCpx(cpu, DIAGNOSTIC_LINK_LIMIT);
    } while (!cpu->carry);
    OpLdx(cpu, 0x0682u);
    OpLda(memory, cpu, OpDp(cpu, DIAGNOSTIC_ROW_LEFT));
    PrintDiagnosticValue(memory, cpu, 0x2du, 0xb237u);
}

Lufia2ExecutionResult Lufia2CaveDrawDiagnostics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x8eu || !cpu->accumulator_is_8_bit ||
        cpu->direct_page || cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x8eb150u);
    Push8(memory, cpu, PackStatus(cpu));
    PushDataBank(memory, cpu);
    OpRepWidths(cpu, 0x10u);
    OpLoadA(cpu, 0x7fu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    DrawRoomGrid(memory, cpu);
    DrawStairMarker(memory, cpu);
    DrawDiagnosticValues(memory, cpu);
    OpLoadA(cpu, 8u);
    OpTestBits(memory, cpu, OpDp(cpu, 0x74u), 1u);
    PullDataBank(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x8eb23eu);
}
