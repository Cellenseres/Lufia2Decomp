#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/board_game.h"
#include "system/wram.h"

enum {
    BOARD_PARTICLE_POSITIONS = 0x7000u,
    BOARD_PARTICLE_X_STEPS = 0x703cu,
    BOARD_PARTICLE_Y_STEPS = 0x7078u,
    BOARD_POSITION = 0x70b6u,
    BOARD_CELLS = 0x70b8u,
    BOARD_CELL_GROUPS = 0x711cu,
    BOARD_DRAW_ORDER = 0x714eu,
    BOARD_LINE_TOTALS = WRAM_BOARD_GAME_LINE_TOTALS & 0xffffu,
    BOARD_COMPLETED_LINES = WRAM_BOARD_GAME_COMPLETED_LINES & 0xffffu,
    BOARD_NEAR_LINES = WRAM_BOARD_GAME_NEAR_LINES & 0xffffu,
    BOARD_DRAWS_LEFT = WRAM_BOARD_GAME_DRAWS_REMAINING & 0xffffu,
    BOARD_SPECIAL_DRAWS = WRAM_BOARD_GAME_SPECIAL_DRAWS & 0xffffu,
    BOARD_FIRST_POSITION = WRAM_BOARD_GAME_FIRST_POSITION & 0xffffu,
    BOARD_SECOND_POSITION = WRAM_BOARD_GAME_SECOND_POSITION & 0xffffu,
    BOARD_OAM = WRAM_FIELD_OAM_LOW_BUFFER & 0xffffu,
    BOARD_SCENE = WRAM_SPECIAL_MENU_SCENE_STATE & 0xffffu,
    BOARD_CALL_LIMIT = 65536u
};

typedef struct BoardGame {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} BoardGame;

static bool BoardReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static bool BoardCall(BoardGame *game, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = game->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = (target >> 16) == 0x84u ? 2u : 3u;
    if (game->calls++ == BOARD_CALL_LIMIT) {
        game->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(game->memory, cpu, game->child, game->context,
                            site, target, frame, 0x84u)) {
        game->result = ExecutionReturned(site);
        game->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!BoardReady(cpu) ||
        cpu->accumulator_is_8_bit != byte_accumulator) {
        game->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static void ClearBoardWords(const Lufia2Memory *memory,
                             Lufia2CpuState *cpu, uint16_t base) {
    do {
        OpStz(memory, cpu, OpAbsX(cpu, base));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
}

Lufia2ExecutionResult Lufia2BoardGameResetRound(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !BoardReady(cpu))
        return ExecutionHandoff(cpu, 0x84d684u);
    BoardGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84d709u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 12u);
    ClearBoardWords(memory, cpu, BOARD_LINE_TOTALS);
    OpStz(memory, cpu, OpAbs(cpu, BOARD_COMPLETED_LINES));
    OpStz(memory, cpu, OpAbs(cpu, 0x69e2u));
    OpLoadA(cpu, 13u);
    OpSta(memory, cpu, OpAbs(cpu, BOARD_DRAWS_LEFT));
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x30u), cpu->x);
    OpLdy(cpu, 25u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, BOARD_CELLS));
        OpAndValue(cpu, 0xffu);
        OpOraValue(cpu, 0x0800u);
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_CELLS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, 0x061cu);
    OpWriteX(memory, cpu, OpDp(cpu, 4u), cpu->x);
    OpLdy(cpu, 3u);
    do {
        for (unsigned column = 0u; column < 4u; ++column) {
            OpStz(memory, cpu, OpAbsX(cpu, 0x2000u));
            if (column != 3u) {
                OpInx(cpu);
                OpInx(cpu);
            }
        }
        OpLda(memory, cpu, OpDp(cpu, 4u));
        cpu->carry = 0u;
        OpAdcValue(cpu, 64u);
        OpSta(memory, cpu, OpDp(cpu, 4u));
        OpTax(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_FIRST_POSITION));
    if (cpu->negative) {
        OpLda(memory, cpu, OpAbs(cpu, BOARD_POSITION));
        OpIncA(cpu);
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbs(cpu, BOARD_FIRST_POSITION));
    }
    OpLda(memory, cpu, OpAbs(cpu, BOARD_SECOND_POSITION));
    if (cpu->negative) {
        OpLda(memory, cpu, OpAbs(cpu, BOARD_POSITION));
        cpu->carry = 0u;
        OpAdcValue(cpu, 6u);
        OpSta(memory, cpu, OpAbs(cpu, BOARD_SECOND_POSITION));
    }
    OpStz(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0x359cu);
    OpLdy(cpu, 0xf932u);
    if (!BoardCall(&game, 0x84d704u, 0x808878u))
        return game.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return game.result;
}

static bool DecodeBoardTilemap(BoardGame *game, uint16_t resource,
                               uint32_t site) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    OpLdx(cpu, resource);
    OpWriteX(memory, cpu, OpDp(cpu, 0x54u), cpu->x);
    OpLdx(cpu, 0x4000u);
    OpWriteX(memory, cpu, OpDp(cpu, 0x60u), cpu->x);
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpDp(cpu, 0x62u));
    return BoardCall(game, site, 0x808e9du);
}

Lufia2ExecutionResult Lufia2BoardGameLoadTilemaps(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !BoardReady(cpu))
        return ExecutionHandoff(cpu, 0x84d70au);
    BoardGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84d764u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    if (!DecodeBoardTilemap(&game, 0x02a0u, 0x84d71bu))
        return game.result;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 1024u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x7e4000u));
        OpOraValue(cpu, 0x2000u);
        OpSta(memory, cpu, OpLongX(cpu, 0x7e2000u));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpSepWidths(cpu, 0x20u);
    if (!DecodeBoardTilemap(&game, 0x029fu, 0x84d747u))
        return game.result;
    OpRepWidths(cpu, 0x20u);
    OpLdx(cpu, 0u);
    OpLdy(cpu, 1024u);
    do {
        OpLda(memory, cpu, OpLongX(cpu, 0x7e4000u));
        OpAndValue(cpu, 0xdfffu);
        OpSta(memory, cpu, OpLongX(cpu, 0x7e2800u));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return game.result;
}

Lufia2ExecutionResult Lufia2BoardGameInitialize(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !BoardReady(cpu))
        return ExecutionHandoff(cpu, 0x84d765u);
    BoardGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84d85fu), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 49u);
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    OpLdy(cpu, 0u);
    do {
        OpLoadA(cpu, 96u);
        if (!BoardCall(&game, 0x84d773u, 0x808299u))
            return game.result;
        OpSta(memory, cpu, OpAbsY(cpu, BOARD_PARTICLE_POSITIONS));
        OpLoadA(cpu, 5u);
        if (!BoardCall(&game, 0x84d77du, 0x808299u))
            return game.result;
        OpIncA(cpu);
        OpTax(cpu);
        OpLoadA(cpu, 1u);
        if (!BoardCall(&game, 0x84d786u, 0x808299u))
            return game.result;
        OpDecA(cpu);
        if (!cpu->zero) {
            OpTxa(cpu);
            LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffu));
            OpIncA(cpu);
            OpAndValue(cpu, 0xffu);
            OpTax(cpu);
        }
        OpTxa(cpu);
        OpSta(memory, cpu, OpAbsY(cpu, BOARD_PARTICLE_X_STEPS));
        OpLoadA(cpu, 16u);
        if (!BoardCall(&game, 0x84d7a0u, 0x808299u))
            return game.result;
        OpTax(cpu);
        OpLoadA(cpu, 7u);
        if (!BoardCall(&game, 0x84d7a8u, 0x808299u))
            return game.result;
        OpDex(cpu);
        if (cpu->negative) {
            cpu->carry = 0u;
            OpAdcValue(cpu, 4u);
            OpCmpValue(cpu, 12u);
            if (!cpu->negative)
                OpLoadA(cpu, 11u);
        }
        OpIncA(cpu);
        cpu->carry = 0u;
        RolA16(cpu);
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffu));
        OpIncA(cpu);
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsY(cpu, BOARD_PARTICLE_Y_STEPS));
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 0x22u), -1);
    } while (!cpu->negative);
    OpLdx(cpu, 2u);
    OpLdy(cpu, 30u);
    OpLoadA(cpu, 0x2480u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_OAM));
        for (unsigned byte = 0u; byte < 4u; ++byte)
            OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLdx(cpu, 5u);
    OpLoadA(cpu, 0xaaaau);
    do {
        OpSta(memory, cpu, OpAbsY(cpu, 0x0300u));
        OpIny(cpu);
        OpIny(cpu);
        OpDex(cpu);
    } while (!cpu->zero);
    OpLoadA(cpu, 0u);
    OpTax(cpu);
    OpLdy(cpu, 25u);
    do {
        OpAndValue(cpu, 0xffu);
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_DRAW_ORDER));
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_CELLS));
        OpInx(cpu);
        OpInx(cpu);
        OpIncA(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLdy(cpu, 25u);
    do {
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_DRAW_ORDER));
        OpInx(cpu);
        OpInx(cpu);
        OpIncA(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLdy(cpu, 25u);
    OpLdx(cpu, 0u);
    ClearBoardWords(memory, cpu, BOARD_CELL_GROUPS);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x84u);
    OpSta(memory, cpu, OpDp(cpu, 0x0au));
    OpLdx(cpu, 0xda39u);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLoadA(cpu, 4u);
    OpSta(memory, cpu, OpDp(cpu, 4u));
    OpLdy(cpu, 0u);
    do {
        OpLoadA(cpu, 6u);
        OpSta(memory, cpu, OpDp(cpu, 2u));
        do {
            OpLda(memory, cpu, DirectLongIndirectY(memory, cpu, 8u));
            OpTax(cpu);
            OpLda(memory, cpu, OpDp(cpu, 0u));
            OpSta(memory, cpu, OpAbsX(cpu, BOARD_CELL_GROUPS));
            OpIny(cpu);
            OpIny(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
        } while (!cpu->zero);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), 1);
        OpStepMem(memory, cpu, OpDp(cpu, 4u), -1);
    } while (!cpu->zero);
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_CARD_GAME_STAKE & 0xffffu));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpAbs(cpu, BOARD_SCENE));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return game.result;
}

static bool ShuffleBoardWords(BoardGame *game, uint16_t field,
                               uint32_t first_site, uint32_t second_site) {
    const Lufia2Memory *memory = game->memory;
    Lufia2CpuState *cpu = game->cpu;
    do {
        OpLoadA(cpu, 25u);
        if (!BoardCall(game, first_site, 0x808299u))
            return false;
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        OpLoadA(cpu, 25u);
        if (!BoardCall(game, second_site, 0x808299u))
            return false;
        cpu->carry = 0u;
        RolA16(cpu);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, field));
        OpSta(memory, cpu, OpDp(cpu, 0x24u));
        OpLda(memory, cpu, OpAbsY(cpu, field));
        OpSta(memory, cpu, OpAbsX(cpu, field));
        OpLda(memory, cpu, OpDp(cpu, 0x24u));
        OpSta(memory, cpu, OpAbsY(cpu, field));
        OpStepMem(memory, cpu, OpDp(cpu, 0x22u), -1);
    } while (!cpu->negative);
    return true;
}

Lufia2ExecutionResult Lufia2BoardGameShuffleCells(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !BoardReady(cpu))
        return ExecutionHandoff(cpu, 0x84d860u);
    BoardGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84d891u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 24u);
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    if (!ShuffleBoardWords(&game, BOARD_CELLS, 0x84d86bu, 0x84d875u))
        return game.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return game.result;
}

Lufia2ExecutionResult Lufia2BoardGameShuffleDrawOrder(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !BoardReady(cpu))
        return ExecutionHandoff(cpu, 0x84d892u);
    BoardGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84d8ecu), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    if (!BoardCall(&game, 0x84d895u, 0x84d860u))
        return game.result;
    OpLoadA(cpu, 80u);
    OpSta(memory, cpu, OpDp(cpu, 0x22u));
    if (!ShuffleBoardWords(&game, BOARD_DRAW_ORDER, 0x84d8a0u, 0x84d8aau))
        return game.result;
    OpLdx(cpu, 0u);
    OpLdy(cpu, 12u);
    do {
        OpStz(memory, cpu, OpAbsX(cpu, BOARD_LINE_TOTALS));
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->negative);
    OpLoadA(cpu, 30u);
    if (!BoardCall(&game, 0x84d8d6u, 0x808299u))
        return game.result;
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpAbs(cpu, BOARD_SPECIAL_DRAWS));
    OpLoadA(cpu, 60u);
    if (!BoardCall(&game, 0x84d8e2u, 0x808299u))
        return game.result;
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpAbs(cpu, BOARD_SPECIAL_DRAWS + 2u));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return game.result;
}

static void StoreBoardLineTotal(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu) {
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, 2u));
    OpCmpValue(cpu, 100u);
    if (!cpu->negative)
        OpStepMem(memory, cpu, OpDp(cpu, 8u), 1);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 4u)));
    OpSta(memory, cpu, OpAbsX(cpu, BOARD_LINE_TOTALS));
    OpStepMem(memory, cpu, OpDp(cpu, 4u), 1);
    OpStepMem(memory, cpu, OpDp(cpu, 4u), 1);
}

static void AddBoardCellFlags(const Lufia2Memory *memory,
                               Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsY(cpu, BOARD_CELLS));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 2u));
}

Lufia2ExecutionResult Lufia2BoardGameCountCompletedLines(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!BoardReady(cpu))
        return ExecutionHandoff(cpu, 0x84cbe2u);
    Push8(memory, cpu, PackStatus(cpu));
    OpSepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpLdx(cpu, 5u);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, 2u), cpu->y);
    OpWriteX(memory, cpu, OpDp(cpu, 4u), cpu->y);
    OpWriteX(memory, cpu, OpDp(cpu, 8u), cpu->y);
    OpIny(cpu);
    do {
        do {
            AddBoardCellFlags(memory, cpu);
            OpIny(cpu);
            OpIny(cpu);
            OpDex(cpu);
        } while (!cpu->zero);
        StoreBoardLineTotal(memory, cpu);
        OpLdx(cpu, 5u);
        OpStz(memory, cpu, OpDp(cpu, 2u));
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->zero);
    OpLdy(cpu, 0u);
    OpLdx(cpu, 5u);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    OpWriteX(memory, cpu, OpDp(cpu, 2u), cpu->y);
    OpIny(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, 6u), cpu->y);
    do {
        do {
            AddBoardCellFlags(memory, cpu);
            OpTya(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 10u);
            OpTay(cpu);
            OpDex(cpu);
        } while (!cpu->zero);
        StoreBoardLineTotal(memory, cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 6u), 1);
        OpStepMem(memory, cpu, OpDp(cpu, 6u), 1);
        OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, 6u)));
        OpLdx(cpu, 5u);
        OpStz(memory, cpu, OpDp(cpu, 2u));
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->zero);
    OpLdy(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, 2u), cpu->y);
    OpLdx(cpu, 2u);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    OpLdx(cpu, 12u);
    OpWriteX(memory, cpu, OpDp(cpu, 6u), cpu->x);
    OpLdx(cpu, 5u);
    OpIny(cpu);
    do {
        do {
            AddBoardCellFlags(memory, cpu);
            OpTya(cpu);
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, 6u));
            OpTay(cpu);
            OpDex(cpu);
        } while (!cpu->zero);
        StoreBoardLineTotal(memory, cpu);
        OpLoadA(cpu, 8u);
        OpSta(memory, cpu, OpDp(cpu, 6u));
        OpLdx(cpu, 5u);
        OpStz(memory, cpu, OpDp(cpu, 2u));
        OpLdy(cpu, 9u);
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->zero);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, 8u)));
    OpWriteX(memory, cpu, OpAbs(cpu, BOARD_COMPLETED_LINES), cpu->x);
    OpRepWidths(cpu, 0x20u);
    OpStz(memory, cpu, OpAbs(cpu, BOARD_NEAR_LINES));
    OpLdx(cpu, 0u);
    OpLdy(cpu, 12u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, BOARD_LINE_TOTALS));
        OpCmpValue(cpu, 88u);
        if (!cpu->negative) {
            OpCmpValue(cpu, 100u);
            if (cpu->negative)
                OpStepMem(memory, cpu, OpAbs(cpu, BOARD_NEAR_LINES), 1);
        }
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84ccd2u);
}

Lufia2ExecutionResult Lufia2BoardGameAdvanceFrame(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !BoardReady(cpu))
        return ExecutionHandoff(cpu, 0x84cbcdu);
    BoardGame game = {memory, cpu, child, context,
                      ExecutionReturned(0x84cbe1u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    PushY(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    if (!BoardCall(&game, 0x84cbd1u, 0x84d558u))
        return game.result;
    OpLda(memory, cpu, OpDp(cpu, 0x74u));
    OpOraValue(cpu, 0x80u);
    OpSta(memory, cpu, OpDp(cpu, 0x74u));
    OpSta(memory, cpu, OpDp(cpu, 0x72u));
    if (!BoardCall(&game, 0x84cbdcu, 0x84a5a8u))
        return game.result;
    OpPullY(memory, cpu);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return game.result;
}
