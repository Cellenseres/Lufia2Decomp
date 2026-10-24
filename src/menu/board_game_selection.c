#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/board_game.h"
#include "system/wram.h"

enum {
    BOARD_TILES = 0x2000u,
    BOARD_CELLS = 0x70b8u,
    BOARD_CELL_LOCATIONS = 0x70eau,
    BOARD_SELECTED_GROUP = WRAM_BOARD_GAME_SELECTED_GROUP & 0xffffu,
    BOARD_DRAWS_LEFT = WRAM_BOARD_GAME_DRAWS_REMAINING & 0xffffu,
    BOARD_SAVED_TILES = WRAM_BOARD_GAME_SAVED_CELL_TILES & 0xffffu,
    BOARD_SAVED_CELLS = WRAM_BOARD_GAME_SAVED_GROUP_CELLS & 0xffffu,
    GROUP_SPAN = 0x00u,
    GROUP_ROWS = 0x02u,
    GROUP_COLUMNS = 0x04u,
    GROUP_ORIGIN = 0x06u,
    GROUP_TABLE = 0x08u,
    GROUP_NEXT_ROW = 0x0bu,
    GROUP_DIRECTION = 0x22u,
    BOARD_CALL_LIMIT = 65536u
};

typedef struct BoardSelection {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} BoardSelection;

static bool SelectionReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static bool SelectionCall(BoardSelection *selection, uint32_t site,
                          uint32_t target) {
    Lufia2CpuState *cpu = selection->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    if (selection->calls++ == BOARD_CALL_LIMIT) {
        selection->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(selection->memory, cpu, selection->child,
                            selection->context, site, target, 2u, 0x84u)) {
        selection->result = ExecutionReturned(site);
        selection->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!SelectionReady(cpu) ||
        cpu->accumulator_is_8_bit != byte_accumulator) {
        selection->result = ExecutionHandoff(cpu, site + 3u);
        return false;
    }
    return true;
}

Lufia2ExecutionResult Lufia2BoardGameDrawRemainingCount(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionReady(cpu))
        return ExecutionHandoff(cpu, 0x84cd5fu);
    BoardSelection selection = {memory, cpu, child, context,
                                 ExecutionReturned(0x84cd94u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_DRAWS_LEFT));
    cpu->carry = 1u;
    OpSbcValue(cpu, 10u);
    if (cpu->negative) {
        OpLoadA(cpu, 0x096au);
        OpLdx(cpu, 0x04deu);
        if (!SelectionCall(&selection, 0x84cd71u, 0x84cd95u))
            return selection.result;
        OpLda(memory, cpu, OpAbs(cpu, BOARD_DRAWS_LEFT));
    } else {
        OpLoadA(cpu, 0x0961u);
        OpLdx(cpu, 0x04deu);
        if (!SelectionCall(&selection, 0x84cd7fu, 0x84cd95u))
            return selection.result;
        OpLda(memory, cpu, OpAbs(cpu, BOARD_DRAWS_LEFT));
        cpu->carry = 1u;
        OpSbcValue(cpu, 10u);
    }
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x0960u);
    OpLdx(cpu, 0x04e0u);
    if (!SelectionCall(&selection, 0x84cd90u, 0x84cd95u))
        return selection.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return selection.result;
}

Lufia2ExecutionResult Lufia2BoardGameDrawDigitTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SelectionReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84cd95u);
    OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
    OpTay(cpu);
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 64u);
    OpTax(cpu);
    OpTya(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 16u);
    OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
    return ExecutionReturned(0x84cda7u);
}

static void LocateGroupBorder(const Lufia2Memory *memory,
                              Lufia2CpuState *cpu, bool clearing) {
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_TABLE + 2u));
    OpLdx(cpu, 0xda31u);
    OpWriteX(memory, cpu, OpDp(cpu, GROUP_TABLE), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_SELECTED_GROUP));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, GROUP_TABLE));
    cpu->carry = 1u;
    OpSbcValue(cpu, 0x42u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_ORIGIN));
    OpLda(memory, cpu, OpAbs(cpu, BOARD_SELECTED_GROUP));
    OpAndValue(cpu, 1u);
    const bool vertical = !cpu->zero;
    OpLoadA(cpu, vertical ? 12u : 18u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_SPAN));
    OpLoadA(cpu, vertical ? 52u : 46u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_NEXT_ROW));
    OpLoadA(cpu, (uint16_t)((vertical ? 5u : 8u) + (clearing ? 2u : 0u)));
    OpSta(memory, cpu, OpDp(cpu, GROUP_COLUMNS));
    OpLoadA(cpu, vertical ? 8u : 5u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_ROWS));
}

static void DrawBorderHorizontal(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu, uint16_t first_tile) {
    OpLoadA(cpu, first_tile);
    OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
    OpInx(cpu);
    OpInx(cpu);
    OpLdy(cpu, OpRead16(memory, OpDp(cpu, GROUP_COLUMNS)));
    OpLoadA(cpu, (uint16_t)(first_tile + 1u));
    do {
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLoadA(cpu, (uint16_t)(first_tile + 2u));
    OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
}

static void AdvanceBorderSide(const Lufia2Memory *memory,
                              Lufia2CpuState *cpu, uint8_t distance) {
    OpTxa(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, distance));
    OpTax(cpu);
}

static void BeginBorderSides(const Lufia2Memory *memory,
                             Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpDp(cpu, GROUP_ORIGIN));
    cpu->carry = 0u;
    OpAdcValue(cpu, 64u);
    OpTax(cpu);
    OpLdy(cpu, OpRead16(memory, OpDp(cpu, GROUP_ROWS)));
}

Lufia2ExecutionResult Lufia2BoardGameDrawSelectionBorder(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionReady(cpu))
        return ExecutionHandoff(cpu, 0x84cee4u);
    BoardSelection selection = {memory, cpu, child, context,
                                 ExecutionReturned(0x84cf90u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    LocateGroupBorder(memory, cpu, false);
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, GROUP_ORIGIN)));
    DrawBorderHorizontal(memory, cpu, 0x0878u);
    BeginBorderSides(memory, cpu);
    do {
        OpLoadA(cpu, 0x087bu);
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
        AdvanceBorderSide(memory, cpu, GROUP_SPAN);
        OpLoadA(cpu, 0x087cu);
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
        AdvanceBorderSide(memory, cpu, GROUP_NEXT_ROW);
        OpDey(cpu);
    } while (!cpu->zero);
    DrawBorderHorizontal(memory, cpu, 0x087du);
    if (!SelectionCall(&selection, 0x84cf8cu, 0x84cbcdu))
        return selection.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return selection.result;
}

static void ClearBorderHorizontal(const Lufia2Memory *memory,
                                  Lufia2CpuState *cpu) {
    OpLdy(cpu, OpRead16(memory, OpDp(cpu, GROUP_COLUMNS)));
    do {
        OpStz(memory, cpu, OpAbsX(cpu, BOARD_TILES));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2BoardGameClearSelectionBorder(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionReady(cpu))
        return ExecutionHandoff(cpu, 0x84cf91u);
    BoardSelection selection = {memory, cpu, child, context,
                                 ExecutionReturned(0x84d015u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    LocateGroupBorder(memory, cpu, true);
    OpLdx(cpu, OpRead16(memory, OpDp(cpu, GROUP_ORIGIN)));
    ClearBorderHorizontal(memory, cpu);
    BeginBorderSides(memory, cpu);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, BOARD_TILES));
        AdvanceBorderSide(memory, cpu, GROUP_SPAN);
        OpStz(memory, cpu, OpAbsX(cpu, BOARD_TILES));
        AdvanceBorderSide(memory, cpu, GROUP_NEXT_ROW);
        OpDey(cpu);
    } while (!cpu->zero);
    ClearBorderHorizontal(memory, cpu);
    if (!SelectionCall(&selection, 0x84d011u, 0x84cbcdu))
        return selection.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return selection.result;
}

static void ChooseGroupMovement(const Lufia2Memory *memory,
                                Lufia2CpuState *cpu) {
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_TABLE + 2u));
    OpLdx(cpu, 0xda31u);
    OpWriteX(memory, cpu, OpDp(cpu, GROUP_TABLE), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_SELECTED_GROUP));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, GROUP_TABLE));
    OpSta(memory, cpu, OpDp(cpu, GROUP_ORIGIN));
    OpLda(memory, cpu, OpAbs(cpu, BOARD_SELECTED_GROUP));
    OpAndValue(cpu, 1u);
    const bool vertical = !cpu->zero;
    OpLdx(cpu, vertical ? 0xd9a1u : 0xd911u);
    OpWriteX(memory, cpu, OpDp(cpu, GROUP_TABLE), cpu->x);
    OpLda(memory, cpu, OpDp(cpu, GROUP_DIRECTION));
    OpAndValue(cpu, 5u);
    if (cpu->zero) {
        OpLdx(cpu, vertical ? 0xd9e9u : 0xd959u);
        OpWriteX(memory, cpu, OpDp(cpu, GROUP_TABLE), cpu->x);
    }
}

Lufia2ExecutionResult Lufia2BoardGameMoveSelectedGroup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !SelectionReady(cpu))
        return ExecutionHandoff(cpu, 0x84d016u);
    BoardSelection selection = {memory, cpu, child, context,
                                 ExecutionReturned(0x84d09au), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_DIRECTION));
    OpStz(memory, cpu, OpDp(cpu, GROUP_DIRECTION + 1u));
    OpLoadA(cpu, 0u);
    if (!SelectionCall(&selection, 0x84d01fu, 0x84a607u) ||
        !SelectionCall(&selection, 0x84d022u, 0x84d0efu))
        return selection.result;
    ChooseGroupMovement(memory, cpu);
    OpLdy(cpu, 0u);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_COLUMNS));
    do {
        OpLoadA(cpu, 5u);
        OpSta(memory, cpu, OpDp(cpu, GROUP_ROWS));
        do {
            OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, GROUP_TABLE));
            OpIny(cpu);
            OpIny(cpu);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, GROUP_ORIGIN));
            OpSta(memory, cpu, OpDp(cpu, GROUP_SPAN));
            if (!SelectionCall(&selection, 0x84d07cu, 0x84d09bu))
                return selection.result;
            OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, GROUP_TABLE));
            OpIny(cpu);
            OpIny(cpu);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, GROUP_SPAN));
            if (!SelectionCall(&selection, 0x84d086u, 0x84d0cbu))
                return selection.result;
            OpStepMem(memory, cpu, OpDp(cpu, GROUP_ROWS), -1);
        } while (!cpu->negative);
        if (!SelectionCall(&selection, 0x84d08du, 0x84cbcdu))
            return selection.result;
        PushY(memory, cpu);
        if (!SelectionCall(&selection, 0x84d091u, 0x84cda8u))
            return selection.result;
        OpPullY(memory, cpu);
        OpStepMem(memory, cpu, OpDp(cpu, GROUP_COLUMNS), -1);
    } while (!cpu->negative);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return selection.result;
}

Lufia2ExecutionResult Lufia2BoardGameLiftCellTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SelectionReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84d09bu);
    OpTax(cpu);
    for (unsigned tile = 0u; tile < 4u; ++tile) {
        OpLda(memory, cpu, OpAbsX(cpu, BOARD_TILES));
        OpSta(memory, cpu, OpAbs(cpu, (uint16_t)(BOARD_SAVED_TILES + tile * 2u)));
        OpStz(memory, cpu, OpAbsX(cpu, BOARD_TILES));
        if (tile == 1u) {
            OpTxa(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 62u);
            OpTax(cpu);
        } else if (tile != 3u) {
            OpInx(cpu);
            OpInx(cpu);
        }
    }
    return ExecutionReturned(0x84d0cau);
}

Lufia2ExecutionResult Lufia2BoardGamePlaceCellTiles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SelectionReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84d0cbu);
    OpTax(cpu);
    for (unsigned tile = 0u; tile < 4u; ++tile) {
        OpLda(memory, cpu, OpAbs(cpu, (uint16_t)(BOARD_SAVED_TILES + tile * 2u)));
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
        if (tile == 1u) {
            OpTxa(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 62u);
            OpTax(cpu);
        } else if (tile != 3u) {
            OpInx(cpu);
            OpInx(cpu);
        }
    }
    return ExecutionReturned(0x84d0eeu);
}

Lufia2ExecutionResult Lufia2BoardGameRotateSelectedGroup(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SelectionReady(cpu))
        return ExecutionHandoff(cpu, 0x84d0efu);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_TABLE + 2u));
    OpLdx(cpu, 0xd909u);
    OpWriteX(memory, cpu, OpDp(cpu, GROUP_TABLE), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 4u);
    OpLda(memory, cpu, OpDp(cpu, GROUP_DIRECTION));
    OpAndValue(cpu, 5u);
    if (cpu->zero)
        OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, GROUP_ROWS), cpu->x);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_SELECTED_GROUP));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTay(cpu);
    OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, GROUP_TABLE));
    OpSta(memory, cpu, OpDp(cpu, GROUP_ORIGIN));
    OpLda(memory, cpu, OpAbs(cpu, BOARD_SELECTED_GROUP));
    OpAndValue(cpu, 1u);
    OpLdx(cpu, cpu->zero ? 0xd8edu : 0xd8f9u);
    OpWriteX(memory, cpu, OpDp(cpu, GROUP_TABLE), cpu->x);
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_SPAN));
    OpLdy(cpu, 2u);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, GROUP_TABLE));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, GROUP_ORIGIN));
        OpTax(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, BOARD_CELLS));
        OpSta(memory, cpu, OpAbsY(cpu, BOARD_SAVED_CELLS - 2u));
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, GROUP_SPAN), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpDp(cpu, GROUP_SPAN));
    OpLdy(cpu, OpRead16(memory, OpDp(cpu, GROUP_ROWS)));
    OpStz(memory, cpu, OpDp(cpu, GROUP_ROWS));
    do {
        OpLdx(cpu, OpRead16(memory, OpDp(cpu, GROUP_ROWS)));
        OpLda(memory, cpu, OpAbsX(cpu, BOARD_SAVED_CELLS));
        OpSta(memory, cpu, OpDp(cpu, GROUP_COLUMNS));
        OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, GROUP_TABLE));
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, GROUP_ORIGIN));
        OpTax(cpu);
        OpLda(memory, cpu, OpDp(cpu, GROUP_COLUMNS));
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_CELLS));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        RolA16(cpu);
        OpWriteX(memory, cpu, OpDp(cpu, GROUP_COLUMNS), cpu->y);
        OpTay(cpu);
        OpTxa(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, BOARD_CELL_LOCATIONS));
        OpLdy(cpu, OpRead16(memory, OpDp(cpu, GROUP_COLUMNS)));
        OpStepMem(memory, cpu, OpDp(cpu, GROUP_ROWS), 1);
        OpStepMem(memory, cpu, OpDp(cpu, GROUP_ROWS), 1);
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, GROUP_SPAN), -1);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84d179u);
}

Lufia2ExecutionResult Lufia2BoardGameDrawCells(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!SelectionReady(cpu))
        return ExecutionHandoff(cpu, 0x84d17au);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x1bu));
    OpLdx(cpu, 0xda69u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x19u), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpLdx(cpu, 0x00c6u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x26u), cpu->x);
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpDp(cpu, 0x24u));
    do {
        OpLoadA(cpu, 5u);
        OpSta(memory, cpu, OpDp(cpu, 0x28u));
        do {
            OpLda(memory, cpu, OpAbsY(cpu, BOARD_CELLS));
            OpAndValue(cpu, 0xff00u);
            OpSta(memory, cpu, OpDp(cpu, 0x2du));
            OpLda(memory, cpu, OpAbsY(cpu, BOARD_CELLS));
            OpAndValue(cpu, 0xffu);
            cpu->carry = 0u;
            RolA16(cpu);
            OpWriteX(memory, cpu, OpDp(cpu, 0x22u), cpu->y);
            OpTay(cpu);
            OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 0x19u));
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, 0x2du));
            OpLdy(cpu, OpRead16(memory, OpDp(cpu, 0x22u)));
            OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
            OpInx(cpu);
            OpInx(cpu);
            OpIncA(cpu);
            OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
            OpSta(memory, cpu, OpDp(cpu, 0x22u));
            OpTxa(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 62u);
            OpTax(cpu);
            OpLda(memory, cpu, OpDp(cpu, 0x22u));
            cpu->carry = 0u;
            OpAdcValue(cpu, 15u);
            OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
            OpInx(cpu);
            OpInx(cpu);
            OpIncA(cpu);
            OpSta(memory, cpu, OpAbsX(cpu, BOARD_TILES));
            OpTxa(cpu);
            cpu->carry = 1u;
            OpSbcValue(cpu, 60u);
            OpTax(cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 0x28u), -1);
        } while (!cpu->zero);
        OpLda(memory, cpu, OpDp(cpu, 0x26u));
        cpu->carry = 0u;
        OpAdcValue(cpu, 192u);
        OpSta(memory, cpu, OpDp(cpu, 0x26u));
        OpTax(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x24u), -1);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84d1eeu);
}
