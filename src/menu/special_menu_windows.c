#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/special_menu.h"
#include "system/wram.h"

enum {
    WINDOW_DEPTH = WRAM_SPECIAL_MENU_WINDOW_DEPTH & 0xffffu,
    WINDOW_TILESET = WRAM_SPECIAL_MENU_WINDOW_STATE & 0xffffu,
    WINDOW_TILESET_OFFSET = WRAM_SPECIAL_MENU_WINDOW_TILE_OFFSET & 0xffffu,
    WINDOW_LEFT = WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu,
    WINDOW_TOP = WINDOW_LEFT + 2u,
    WINDOW_WIDTH = WINDOW_LEFT + 4u,
    WINDOW_HEIGHT = WINDOW_LEFT + 6u,
    WINDOW_BUFFER_USED = WRAM_SPECIAL_MENU_WINDOW_STYLE & 0xffffu,
    WINDOW_SAVED_BUFFER = WRAM_SPECIAL_MENU_WINDOW_SAVED_BUFFER & 0xffffu,
    WINDOW_SAVED_LEFT = WRAM_SPECIAL_MENU_WINDOW_SAVED_LEFT & 0xffffu,
    WINDOW_SAVED_TOP = WRAM_SPECIAL_MENU_WINDOW_SAVED_TOP & 0xffffu,
    WINDOW_SAVED_WIDTH = WRAM_SPECIAL_MENU_WINDOW_SAVED_WIDTH & 0xffffu,
    WINDOW_SAVED_HEIGHT = WRAM_SPECIAL_MENU_WINDOW_SAVED_HEIGHT & 0xffffu,
    WINDOW_ROW = WRAM_SPECIAL_MENU_WINDOW_WORK & 0xffffu,
    WINDOW_COPY_ROW = WINDOW_ROW + 2u,
    WINDOW_ROW_WIDTH = WINDOW_ROW + 4u,
    WINDOW_REMAINING_WIDTH = WINDOW_ROW + 6u,
    WINDOW_REMAINING_HEIGHT = WINDOW_ROW + 8u,
    WINDOW_INTERIOR_HEIGHT = WINDOW_ROW + 10u,
    WINDOW_SAVED_TILES = WRAM_SPECIAL_MENU_WINDOW_SAVED_TILES & 0xffffu,
    WINDOW_TILEMAP = 0x2000u,
    WINDOW_DIRTY_TILES = 0x3000u,
    WINDOW_OPERATION_LIMIT = 4096u
};

static bool WindowReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->index_is_8_bit && !cpu->decimal;
}

static void SelectWindowRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbs(cpu, WINDOW_DEPTH));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
}

static void LocateSavedWindow(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, WINDOW_SAVED_TOP));
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    RorA16(cpu);
    RorA16(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpLongX(cpu, 0x7e0000u | WINDOW_SAVED_LEFT));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpLongX(cpu, 0x7e0000u | WINDOW_SAVED_LEFT));
    OpSta(memory, cpu, OpAbs(cpu, WINDOW_ROW));
}

static void SaveWindowRecord(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    static const uint16_t fields[][2] = {
        {WINDOW_BUFFER_USED, WINDOW_SAVED_BUFFER},
        {WINDOW_LEFT, WINDOW_SAVED_LEFT}, {WINDOW_TOP, WINDOW_SAVED_TOP},
        {WINDOW_WIDTH, WINDOW_SAVED_WIDTH}, {WINDOW_HEIGHT, WINDOW_SAVED_HEIGHT}
    };
    for (unsigned i = 0u; i < sizeof fields / sizeof fields[0]; ++i) {
        OpLda(memory, cpu, OpAbs(cpu, fields[i][0]));
        OpSta(memory, cpu, OpAbsX(cpu, fields[i][1]));
    }
}

static bool DrawWindowRow(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                          Lufia2PushedChildCall child, void *context,
                          uint32_t site, Lufia2ExecutionResult *result) {
    if (!CallChildWithFrame(memory, cpu, child, context, site, 0x84a36au, 2u, 0x84u)) {
        *result = ExecutionReturned(site);
        result->flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!WindowReady(cpu) || cpu->accumulator_is_8_bit) {
        *result = ExecutionHandoff(cpu, (uint32_t)cpu->program_bank << 16 |
                                   (uint16_t)(site + 3u));
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2SpecialMenuDrawWindowRow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!WindowReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84a36au);
    OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, WINDOW_ROW_WIDTH, 0u));
    OpWriteX(memory, cpu, OpAbs(cpu, WINDOW_REMAINING_WIDTH), cpu->x);
    OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, WINDOW_ROW, 0u));
    OpTay(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, WINDOW_TILEMAP));
    OpIncA(cpu);
    OpStepMem(memory, cpu, OpAbs(cpu, WINDOW_REMAINING_WIDTH), -1);
    OpStepMem(memory, cpu, OpAbs(cpu, WINDOW_REMAINING_WIDTH), -1);
    unsigned tiles = 0u;
    do {
        if (tiles++ == WINDOW_OPERATION_LIMIT)
            return ExecutionHandoff(cpu, 0x84a37eu);
        OpInx(cpu);
        OpInx(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, WINDOW_TILEMAP));
        OpStepMem(memory, cpu, OpAbs(cpu, WINDOW_REMAINING_WIDTH), -1);
    } while (!cpu->zero);
    OpInx(cpu);
    OpInx(cpu);
    OpIncA(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, WINDOW_TILEMAP));
    OpIncA(cpu);
    OpLda(memory, cpu, OpAbs(cpu, WINDOW_ROW));
    cpu->carry = 0u;
    OpAdcValue(cpu, 64u);
    OpSta(memory, cpu, OpAbs(cpu, WINDOW_ROW));
    OpTya(cpu);
    return ExecutionReturned(0x84a39au);
}

Lufia2ExecutionResult Lufia2SpecialMenuPushWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !WindowReady(cpu))
        return ExecutionHandoff(cpu, 0x84a2b3u);
    PushIndex(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    SelectWindowRecord(memory, cpu);
    SaveWindowRecord(memory, cpu);
    LocateSavedWindow(memory, cpu);
    OpSta(memory, cpu, OpAbs(cpu, WINDOW_COPY_ROW));
    OpLda(memory, cpu, OpAbsX(cpu, WINDOW_SAVED_WIDTH));
    OpSta(memory, cpu, OpAbs(cpu, WINDOW_ROW_WIDTH));
    OpSta(memory, cpu, OpAbs(cpu, WINDOW_REMAINING_WIDTH));
    OpLda(memory, cpu, OpAbsX(cpu, WINDOW_SAVED_HEIGHT));
    OpSta(memory, cpu, OpAbs(cpu, WINDOW_REMAINING_HEIGHT));
    OpDecA(cpu);
    OpDecA(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WINDOW_INTERIOR_HEIGHT));
    OpLdy(cpu, Read16AbsoluteIndexed(memory, cpu, WINDOW_BUFFER_USED, 0u));
    unsigned tiles = 0u;
    do {
        OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, WINDOW_COPY_ROW, 0u));
        do {
            if (tiles++ == WINDOW_OPERATION_LIMIT)
                return ExecutionHandoff(cpu, 0x84a30du);
            OpLda(memory, cpu, OpAbsX(cpu, WINDOW_TILEMAP));
            OpStz(memory, cpu, OpAbsX(cpu, WINDOW_DIRTY_TILES));
            OpSta(memory, cpu, OpAbsY(cpu, WINDOW_SAVED_TILES));
            OpInx(cpu);
            OpInx(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpAbs(cpu, WINDOW_REMAINING_WIDTH), -1);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpAbs(cpu, WINDOW_ROW_WIDTH));
        OpSta(memory, cpu, OpAbs(cpu, WINDOW_REMAINING_WIDTH));
        OpLda(memory, cpu, OpAbs(cpu, WINDOW_COPY_ROW));
        cpu->carry = 0u;
        OpAdcValue(cpu, 64u);
        OpSta(memory, cpu, OpAbs(cpu, WINDOW_COPY_ROW));
        OpStepMem(memory, cpu, OpAbs(cpu, WINDOW_REMAINING_HEIGHT), -1);
    } while (!cpu->zero);
    OpWriteX(memory, cpu, OpAbs(cpu, WINDOW_BUFFER_USED), cpu->y);
    OpLdx(cpu, 0u);
    OpLda(memory, cpu, OpAbs(cpu, WINDOW_TILESET));
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpAbs(cpu, WINDOW_TILESET_OFFSET));
    RolA16(cpu);
    RolA16(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, 0x7e0000u | WINDOW_TILESET_OFFSET);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0d20u);
    Lufia2ExecutionResult result = ExecutionReturned(0x84a369u);
    if (!DrawWindowRow(memory, cpu, child, context, 0x84a34du, &result))
        return result;
    cpu->carry = 0u;
    OpAdcValue(cpu, 3u);
    unsigned rows = 0u;
    do {
        if (rows++ == WINDOW_OPERATION_LIMIT)
            return ExecutionHandoff(cpu, 0x84a354u);
        if (!DrawWindowRow(memory, cpu, child, context, 0x84a354u, &result))
            return result;
        OpStepMem(memory, cpu, OpAbs(cpu, WINDOW_INTERIOR_HEIGHT), -1);
    } while (!cpu->zero);
    cpu->carry = 0u;
    OpAdcValue(cpu, 3u);
    if (!DrawWindowRow(memory, cpu, child, context, 0x84a360u, &result))
        return result;
    OpStepMem(memory, cpu, OpAbs(cpu, WINDOW_DEPTH), 1);
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return result;
}

Lufia2ExecutionResult Lufia2SpecialMenuPopWindow(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    enum { SAVED_WIDTH = WINDOW_COPY_ROW, COLUMNS_LEFT = WINDOW_ROW_WIDTH,
           ROWS_LEFT = WINDOW_REMAINING_WIDTH };
    if (!WindowReady(cpu))
        return ExecutionHandoff(cpu, 0x84a39bu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpStepMem(memory, cpu, OpAbs(cpu, WINDOW_DEPTH), -1);
    SelectWindowRecord(memory, cpu);
    LocateSavedWindow(memory, cpu);
    OpLda(memory, cpu, OpAbsX(cpu, WINDOW_SAVED_WIDTH));
    OpSta(memory, cpu, OpAbs(cpu, SAVED_WIDTH));
    OpSta(memory, cpu, OpAbs(cpu, COLUMNS_LEFT));
    OpLda(memory, cpu, OpAbsX(cpu, WINDOW_SAVED_HEIGHT));
    OpSta(memory, cpu, OpAbs(cpu, ROWS_LEFT));
    OpLdy(cpu, Read16AbsoluteIndexed(memory, cpu, WINDOW_SAVED_BUFFER, cpu->x));
    OpWriteX(memory, cpu, OpAbs(cpu, WINDOW_BUFFER_USED), cpu->y);
    unsigned tiles = 0u;
    do {
        OpLdx(cpu, Read16AbsoluteIndexed(memory, cpu, WINDOW_ROW, 0u));
        do {
            if (tiles++ == WINDOW_OPERATION_LIMIT)
                return ExecutionHandoff(cpu, 0x84a3d3u);
            OpLda(memory, cpu, OpAbsY(cpu, WINDOW_SAVED_TILES));
            OpStz(memory, cpu, OpAbsX(cpu, WINDOW_DIRTY_TILES));
            OpSta(memory, cpu, OpAbsX(cpu, WINDOW_TILEMAP));
            OpInx(cpu);
            OpInx(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpAbs(cpu, COLUMNS_LEFT), -1);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpAbs(cpu, SAVED_WIDTH));
        OpSta(memory, cpu, OpAbs(cpu, COLUMNS_LEFT));
        OpLda(memory, cpu, OpAbs(cpu, WINDOW_ROW));
        cpu->carry = 0u;
        OpAdcValue(cpu, 64u);
        OpSta(memory, cpu, OpAbs(cpu, WINDOW_ROW));
        OpStepMem(memory, cpu, OpAbs(cpu, ROWS_LEFT), -1);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84a3fbu);
}
