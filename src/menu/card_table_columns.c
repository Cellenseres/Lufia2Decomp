#include "menu/card_table_services.h"
#include "lufia2/card_table.h"
#include "system/wram.h"

enum {
    COLUMN_NUMBER = 0x11u,
    COLUMN_STEP = 0x13u,
    COLUMN_POSITION = 0x15u,
    COLUMN_SPRITE_OFFSET = 0x17u,
    COLUMN_SYMBOL_POINTER = 0x08u,
    COLUMN_SYMBOL_BANK = 0x0au,
    COLUMN_COORDINATE = 0x22u,
    COLUMN_SYMBOL_OFFSET = 0x24u,
    COLUMN_OAM_OFFSET = 0x26u,
    COLUMN_ROWS = 0x28u,
    COLUMN_SPEEDS = WRAM_CARD_GAME_OPENING_CHOICES & 0xffffu,
    COLUMN_STEPS = (WRAM_POKER_OPPONENT_VALUES + 3u) & 0xffffu,
    COLUMN_POSITIONS = (WRAM_POKER_ROUND_STAKE + 3u) & 0xffffu,
    COLUMN_SYMBOLS = WRAM_CARD_GAME_TILE_BUFFER & 0xffffu,
    COLUMN_TILES = COLUMN_SYMBOLS + 174u,
    COLUMN_OAM = WRAM_FIELD_OAM_LOW_BUFFER & 0xffffu
};

static bool AdvanceColumn(TableService *service) {
    const Lufia2Memory *memory = service->memory;
    Lufia2CpuState *cpu = service->cpu;
    OpSta(memory, cpu, OpDp(cpu, COLUMN_STEP));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbsX(cpu, COLUMN_POSITIONS));
    OpSta(memory, cpu, OpDp(cpu, COLUMN_POSITION));
    OpCmpValue(cpu, 0x1c00u);
    if (!cpu->negative) {
        OpLda(memory, cpu, OpAbsX(cpu, COLUMN_SPEEDS));
        OpDecA(cpu);
        OpCmpValue(cpu, 5u);
        if (cpu->negative) OpLoadA(cpu, 25u);
        OpSta(memory, cpu, OpAbsX(cpu, COLUMN_SPEEDS));
        OpStz(memory, cpu, OpAbsX(cpu, COLUMN_POSITIONS));
        if (!TableServiceCall(service, 0x84ea12u, 0x84ea4bu, false))
            return false;
        OpLda(memory, cpu, OpDp(cpu, COLUMN_POSITION));
        cpu->carry = 1u;
        OpSbcValue(cpu, 0x1c00u);
        OpSta(memory, cpu, OpDp(cpu, COLUMN_STEP));
    }
    OpSta(memory, cpu, OpAbsX(cpu, COLUMN_POSITIONS));
    return true;
}

static void AdvanceColumnSprites(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, 24u);
    OpTxa(cpu);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    RorA16(cpu);
    RorA16(cpu);
    RorA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, COLUMN_SPRITE_OFFSET));
    RorA16(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, COLUMN_SPRITE_OFFSET));
    OpTax(cpu);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, COLUMN_OAM));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, COLUMN_STEP));
        OpSta(memory, cpu, OpAbsX(cpu, COLUMN_OAM));
        for (unsigned byte = 0u; byte < 4u; ++byte) OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2CardTableAdvanceColumns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, true))
        return ExecutionHandoff(cpu, 0x84e9e3u);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84ea4au), true};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, COLUMN_NUMBER));
    do {
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, COLUMN_STEPS));
        if (!cpu->zero) {
            if (!AdvanceColumn(&service)) return service.result;
            AdvanceColumnSprites(memory, cpu);
        }
        OpStepMem(memory, cpu, OpDp(cpu, COLUMN_NUMBER), 1);
        OpLda(memory, cpu, OpDp(cpu, COLUMN_NUMBER));
        OpCmpValue(cpu, 3u);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return service.result;
}

Lufia2ExecutionResult Lufia2CardTableRefreshColumns(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !TableServiceReady(cpu, false) || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84ea4bu);
    TableService service = {memory, cpu, child, context,
        ExecutionReturned(0x84eaa4u), false};
    OpPushX(memory, cpu);
    PushY(memory, cpu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, COLUMN_SYMBOL_BANK));
    OpRepWidths(cpu, 0x20u);
    static const uint32_t calls[] = {0x84ea6cu, 0x84ea85u, 0x84ea9eu};
    for (unsigned column = 0u; column < 3u; ++column) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(COLUMN_POSITIONS + column * 2u)));
        if (!cpu->zero) continue;
        OpLdx(cpu, (uint16_t)(COLUMN_SYMBOLS + column * 58u));
        OpWriteX(memory, cpu, OpDp(cpu, COLUMN_SYMBOL_POINTER), cpu->x);
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(COLUMN_SPEEDS + column * 2u)));
        cpu->carry = 0u;
        RolA16(cpu);
        OpTay(cpu);
        OpLdx(cpu, (uint16_t)(column * 96u));
        OpLoadA(cpu, (uint16_t)(0x0638u + column * 0x38u));
        if (!TableServiceCall(&service, calls[column], 0x84eaa5u, false))
            return service.result;
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    OpPullY(memory, cpu);
    OpPullX(memory, cpu);
    return service.result;
}

static void DrawColumnSymbol(const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdy(cpu, Read16Direct(memory, cpu, COLUMN_SYMBOL_OFFSET));
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, COLUMN_SYMBOL_POINTER));
    OpCpy(cpu, 48u);
    if (cpu->zero) OpLoadA(cpu, 9u);
    else {
        OpCmpValue(cpu, 8u);
        if (!cpu->negative) {
            cpu->carry = 1u;
            OpSbcValue(cpu, 3u);
        }
    }
    cpu->carry = 0u;
    RolA16(cpu);
    RolA16(cpu);
    RolA16(cpu);
    OpTax(cpu);
    OpIny(cpu);
    OpIny(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, COLUMN_SYMBOL_OFFSET), cpu->y);
    OpLdy(cpu, Read16Direct(memory, cpu, COLUMN_OAM_OFFSET));
    static const uint16_t offsets[] = {0u, 0x10u, 0x1000u, 0x1010u};
    for (unsigned cell = 0u; cell < 4u; ++cell) {
        OpLda(memory, cpu, OpDp(cpu, COLUMN_COORDINATE));
        if (cell) {
            cpu->carry = 0u;
            OpAdcValue(cpu, offsets[cell]);
        }
        OpSta(memory, cpu, OpAbsY(cpu, COLUMN_OAM));
        OpIny(cpu);
        OpIny(cpu);
        if (cell) {
            OpInx(cpu);
            OpInx(cpu);
        }
        OpLda(memory, cpu, OpAbsX(cpu, COLUMN_TILES));
        OpSta(memory, cpu, OpAbsY(cpu, COLUMN_OAM));
        OpIny(cpu);
        OpIny(cpu);
    }
    OpWriteX(memory, cpu, OpDp(cpu, COLUMN_OAM_OFFSET), cpu->y);
    OpLda(memory, cpu, OpDp(cpu, COLUMN_COORDINATE));
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x1c00u);
    OpSta(memory, cpu, OpDp(cpu, COLUMN_COORDINATE));
}

Lufia2ExecutionResult Lufia2CardTableDrawColumn(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!TableServiceReady(cpu, true) || cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84eaa5u);
    OpSta(memory, cpu, OpDp(cpu, COLUMN_COORDINATE));
    for (unsigned byte = 0u; byte < 4u; ++byte) OpDey(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, COLUMN_SYMBOL_OFFSET), cpu->y);
    OpWriteX(memory, cpu, OpDp(cpu, COLUMN_OAM_OFFSET), cpu->x);
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, COLUMN_ROWS));
    do {
        DrawColumnSymbol(memory, cpu);
        OpStepMem(memory, cpu, OpDp(cpu, COLUMN_ROWS), -1);
    } while (!cpu->zero);
    return ExecutionReturned(0x84eb35u);
}
