#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "lufia2/board_game.h"
#include "system/wram.h"

enum {
    BOARD_STAKE = WRAM_CARD_GAME_STAKE & 0xffffu,
    BOARD_RESERVED_STAKE = WRAM_CARD_GAME_RESERVED_STAKE & 0xffffu,
    BOARD_BALANCE = WRAM_MENU_EXCHANGE_BALANCE & 0xffffu,
    BOARD_BONUS = WRAM_BOARD_GAME_BONUS_MULTIPLIER & 0xffffu,
    BOARD_PAYOUT_STEP = WRAM_SPECIAL_MENU_PAYOUT_STEP & 0xffffu,
    BOARD_LINE_TOTALS = WRAM_BOARD_GAME_LINE_TOTALS & 0xffffu,
    BOARD_COMPLETED_LINES = WRAM_BOARD_GAME_COMPLETED_LINES & 0xffffu,
    BOARD_MARKERS = WRAM_BOARD_GAME_SPECIAL_DRAWS & 0xffffu,
    BOARD_OAM = WRAM_FIELD_OAM_LOW_BUFFER & 0xffffu,
    BOARD_WINDOW_HEIGHT = (WRAM_SPECIAL_MENU_WINDOW_PARAMETERS & 0xffffu) + 6u,
    BOARD_PARTICLE_FIRST = 0x70b4u,
    BOARD_PARTICLE_OAM = 0x70b6u,
    BOARD_PARTICLE_POSITIONS = 0x7000u,
    BOARD_PARTICLE_X_STEPS = 0x703cu,
    BOARD_PARTICLE_Y_STEPS = 0x7078u,
    BOARD_CELLS = 0x70b8u,
    BOARD_CELL_LOCATIONS = 0x70eau,
    BOARD_TILES = 0x2000u,
    STAKE_SELECTED = 0x0bu,
    STAKE_OAM_CURSOR = 0x2du,
    STAKE_ATTRIBUTES = 0x1cu,
    STAKE_DIGIT_POSITION = 0x06u,
    STAKE_DIGIT_ROWS = 0x04u,
    PARTICLE_POSITION = 0x22u,
    PARTICLE_FIRST = 0x24u,
    PARTICLE_OAM_CURSOR = 0x26u,
    BOARD_FRAME_FLAGS = 0x74u,
    BOARD_STEP_LIMIT = 65536u
};

typedef struct BoardResults {
    const Lufia2Memory *memory;
    Lufia2CpuState *cpu;
    Lufia2PushedChildCall child;
    void *context;
    Lufia2ExecutionResult result;
    unsigned calls;
} BoardResults;

static bool ResultsReady(const Lufia2CpuState *cpu) {
    return cpu->program_bank == 0x84u && !cpu->index_is_8_bit &&
           !cpu->direct_page && !cpu->decimal;
}

static bool ResultsCall(BoardResults *results, uint32_t site, uint32_t target) {
    Lufia2CpuState *cpu = results->cpu;
    const bool byte_accumulator = cpu->accumulator_is_8_bit != 0u;
    const uint8_t frame = (target >> 16) == 0x84u ? 2u : 3u;
    if (results->calls++ == BOARD_STEP_LIMIT) {
        results->result = ExecutionHandoff(cpu, site);
        return false;
    }
    if (!CallChildWithFrame(results->memory, cpu, results->child,
                            results->context, site, target, frame, 0x84u)) {
        results->result = ExecutionReturned(site);
        results->result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return false;
    }
    if (!ResultsReady(cpu) ||
        cpu->accumulator_is_8_bit != byte_accumulator) {
        results->result = ExecutionHandoff(cpu, site + frame + 1u);
        return false;
    }
    return true;
}

static void SetBoardFrameFlags(const Lufia2Memory *memory,
                               Lufia2CpuState *cpu) {
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, BOARD_FRAME_FLAGS));
}

static bool LimitAvailableStake(BoardResults *results) {
    const Lufia2Memory *memory = results->memory;
    Lufia2CpuState *cpu = results->cpu;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_BALANCE + 2u));
    if (!cpu->zero)
        return true;
    OpLda(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, 0x7e0000u | BOARD_RESERVED_STAKE);
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, STAKE_SELECTED)));
    if (!cpu->carry) {
        OpLda(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
        cpu->carry = 0u;
        OpAdc(memory, cpu, 0x7e0000u | BOARD_RESERVED_STAKE);
        OpSta(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
        OpSta(memory, cpu, OpDp(cpu, STAKE_SELECTED));
        OpStz(memory, cpu, OpAbs(cpu, BOARD_RESERVED_STAKE));
    }
    return true;
}

static void SplitStakeDigits(const Lufia2Memory *memory,
                             Lufia2CpuState *cpu) {
    OpLdx(cpu, 0xffffu);
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTED));
    do {
        OpInx(cpu);
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 2u)));
    } while (!cpu->negative);
    OpWriteX(memory, cpu, OpDp(cpu, 0u), cpu->x);
    OpTxa(cpu);
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    RolA16(cpu);
    RolA16(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, 2u));
    OpSta(memory, cpu, OpDp(cpu, 2u));
    OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTED));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 2u)));
    ExchangeAccumulatorBytes(cpu);
    OpOra(memory, cpu, OpDp(cpu, 0u));
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
}

static void SelectStakeDigit(const Lufia2Memory *memory,
                             Lufia2CpuState *cpu, uint16_t mask,
                             bool high_digit, uint16_t position,
                             uint16_t attributes) {
    OpLda(memory, cpu, OpDp(cpu, 0u));
    OpAndValue(cpu, mask);
    if (high_digit)
        ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    RolA16(cpu);
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, STAKE_ATTRIBUTES));
    RolA16(cpu);
    OpAdc(memory, cpu, OpDp(cpu, STAKE_ATTRIBUTES));
    OpTax(cpu);
    OpLdy(cpu, position);
    OpLoadA(cpu, attributes);
}

static bool DrawSelectedStake(BoardResults *results) {
    const Lufia2Memory *memory = results->memory;
    Lufia2CpuState *cpu = results->cpu;
    SplitStakeDigits(memory, cpu);
    OpLoadA(cpu, 0xccu);
    OpSta(memory, cpu, OpDp(cpu, STAKE_OAM_CURSOR));
    SelectStakeDigit(memory, cpu, 0xff00u, true, 0xa828u, 0x3600u);
    if (!ResultsCall(results, 0x84d282u, 0x84d414u))
        return false;
    SelectStakeDigit(memory, cpu, 0xffu, false, 0xa838u, 0x3400u);
    return ResultsCall(results, 0x84d299u, 0x84d414u);
}

static bool ReserveSelectedStake(BoardResults *results) {
    const Lufia2Memory *memory = results->memory;
    Lufia2CpuState *cpu = results->cpu;
    OpLda(memory, cpu, OpAbs(cpu, BOARD_RESERVED_STAKE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, STAKE_SELECTED)));
    if (cpu->zero)
        return true;
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x21u);
    if (!ResultsCall(results, 0x84d2b2u, 0x84a607u))
        return false;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_RESERVED_STAKE));
    cpu->carry = 1u;
    OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, STAKE_SELECTED)));
    if (!cpu->negative) {
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
        OpSta(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
        if (cpu->carry)
            OpStepMem(memory, cpu, OpAbs(cpu, BOARD_BALANCE + 2u), 1);
    } else {
        OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTED));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, 0x7e0000u | BOARD_RESERVED_STAKE));
        OpSta(memory, cpu, OpDp(cpu, 0u));
        OpLda(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, OpDp(cpu, 0u)));
        OpSta(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
        if (!cpu->carry)
            OpStepMem(memory, cpu, OpAbs(cpu, BOARD_BALANCE + 2u), -1);
    }
    return true;
}

static bool ChangeSelectedStake(const Lufia2Memory *memory,
                                Lufia2CpuState *cpu) {
    OpTax(cpu);
    OpSepWidths(cpu, 0x20u);
    OpCmpValue(cpu, 6u);
    if (cpu->zero) {
        OpLoadA(cpu, 1u);
    } else {
        OpCmpValue(cpu, 5u);
        if (cpu->zero) {
            OpLoadA(cpu, 99u);
        } else {
            ExchangeAccumulatorBytes(cpu);
            RorA8(cpu);
            if (cpu->carry) {
                OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTED));
                cpu->carry = 0u;
                OpAdcValue(cpu, 10u);
                OpCmpValue(cpu, 99u);
                if (!cpu->negative)
                    OpLoadA(cpu, 99u);
            } else {
                RorA8(cpu);
                if (cpu->carry) {
                    OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTED));
                    cpu->carry = 1u;
                    OpSbcValue(cpu, 10u);
                    OpCmpValue(cpu, 1u);
                    if (cpu->negative)
                        OpLoadA(cpu, 1u);
                } else {
                    RorA8(cpu);
                    if (cpu->carry) {
                        OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTED));
                        OpIncA(cpu);
                        OpCmpValue(cpu, 99u);
                        if (!cpu->negative)
                            OpLoadA(cpu, 99u);
                    } else {
                        RorA8(cpu);
                        if (!cpu->carry) {
                            OpRepWidths(cpu, 0x20u);
                            OpTxa(cpu);
                            OpCmpValue(cpu, 1u);
                            return false;
                        }
                        OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTED));
                        OpDecA(cpu);
                        if (cpu->zero)
                            OpIncA(cpu);
                    }
                }
            }
        }
    }
    OpSta(memory, cpu, OpDp(cpu, STAKE_SELECTED));
    return true;
}

static bool FlyStakeSprites(BoardResults *results) {
    const Lufia2Memory *memory = results->memory;
    Lufia2CpuState *cpu = results->cpu;
    OpLoadA(cpu, 11u);
    OpSta(memory, cpu, OpDp(cpu, 0u));
    do {
        OpLdx(cpu, 0xccu);
        OpLdy(cpu, 11u);
        do {
            OpLda(memory, cpu, OpAbsX(cpu, BOARD_OAM));
            cpu->carry = 0u;
            OpAdcValue(cpu, 0x0206u);
            OpSta(memory, cpu, OpAbsX(cpu, BOARD_OAM));
            for (unsigned byte = 0u; byte < 4u; ++byte)
                OpInx(cpu);
            OpDey(cpu);
        } while (!cpu->negative);
        OpSepWidths(cpu, 0x20u);
        if (!ResultsCall(results, 0x84d38au, 0x84cbcdu))
            return false;
        OpRepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
    } while (!cpu->negative);
    return true;
}

static void TransferStakeSpritesToTiles(const Lufia2Memory *memory,
                                       Lufia2CpuState *cpu) {
    OpLdx(cpu, 0xccu);
    OpLdy(cpu, 0x061cu);
    OpWriteX(memory, cpu, OpDp(cpu, 4u), cpu->y);
    OpLoadA(cpu, 0x0c00u);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, 2u));
    do {
        OpLoadA(cpu, 2u);
        OpSta(memory, cpu, OpDp(cpu, 0u));
        do {
            for (unsigned column = 0u; column < 2u; ++column) {
                OpLoadA(cpu, 0xf000u);
                OpSta(memory, cpu, OpAbsX(cpu, BOARD_OAM));
                OpInx(cpu);
                OpInx(cpu);
                OpLda(memory, cpu, OpAbsX(cpu, BOARD_OAM));
                OpAndValue(cpu, 0x01ffu);
                OpOra(memory, cpu, OpDp(cpu, 6u));
                OpSta(memory, cpu, OpAbsY(cpu, BOARD_TILES));
                if (!column) {
                    OpIny(cpu);
                    OpIny(cpu);
                }
                OpInx(cpu);
                OpInx(cpu);
            }
            OpTya(cpu);
            cpu->carry = 0u;
            OpAdcValue(cpu, 62u);
            OpTay(cpu);
            OpStepMem(memory, cpu, OpDp(cpu, 0u), -1);
        } while (!cpu->negative);
        OpLda(memory, cpu, OpDp(cpu, 4u));
        cpu->carry = 0u;
        OpAdcValue(cpu, 4u);
        OpTay(cpu);
        OpLoadA(cpu, 0x0800u);
        OpSta(memory, cpu, OpDp(cpu, 6u));
        OpStepMem(memory, cpu, OpDp(cpu, 2u), -1);
    } while (!cpu->negative);
}

Lufia2ExecutionResult Lufia2BoardGameChooseStake(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ResultsReady(cpu))
        return ExecutionHandoff(cpu, 0x84d1efu);
    BoardResults results = {memory, cpu, child, context,
                            ExecutionReturned(0x84d413u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_STAKE));
    OpStz(memory, cpu, OpAbs(cpu, BOARD_RESERVED_STAKE));
    OpSta(memory, cpu, OpDp(cpu, STAKE_SELECTED));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    if (!ResultsCall(&results, 0x84d1fcu, 0x84a39bu))
        return results.result;
    OpLoadA(cpu, 6u);
    OpSta(memory, cpu, OpAbs(cpu, BOARD_WINDOW_HEIGHT));
    if (!ResultsCall(&results, 0x84d205u, 0x84a2b3u))
        return results.result;
    OpLdx(cpu, 0x3504u);
    OpLdy(cpu, 0xf954u);
    if (!ResultsCall(&results, 0x84d20eu, 0x808878u))
        return results.result;
    SetBoardFrameFlags(memory, cpu);
    if (!ResultsCall(&results, 0x84d218u, 0x84cbcdu))
        return results.result;
    bool redraw = true;
    for (;;) {
        if (redraw) {
            LimitAvailableStake(&results);
            if (!DrawSelectedStake(&results))
                return results.result;
            OpLdx(cpu, 0x3504u);
            OpLdy(cpu, 0xf954u);
            if (!ResultsCall(&results, 0x84d2a2u, 0x808878u) ||
                !ReserveSelectedStake(&results))
                return results.result;
            OpLdx(cpu, 0x3682u);
            OpLdy(cpu, 0xf945u);
            if (!ResultsCall(&results, 0x84d2eau, 0x808878u))
                return results.result;
            SetBoardFrameFlags(memory, cpu);
            OpRepWidths(cpu, 0x20u);
            OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTED));
            OpSta(memory, cpu, OpAbs(cpu, BOARD_RESERVED_STAKE));
        }
        do {
            if (!ResultsCall(&results, 0x84d2fbu, 0x84cbcdu) ||
                !ResultsCall(&results, 0x84d2feu, 0x849f1au))
                return results.result;
            OpCmpValue(cpu, 0u);
        } while (cpu->zero);
        redraw = ChangeSelectedStake(memory, cpu);
        if (!redraw && cpu->zero)
            break;
    }
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 2u);
    if (!ResultsCall(&results, 0x84d362u, 0x84a607u))
        return results.result;
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpDp(cpu, STAKE_SELECTED));
    OpSta(memory, cpu, OpAbs(cpu, BOARD_STAKE));
    if (!FlyStakeSprites(&results))
        return results.result;
    TransferStakeSpritesToTiles(memory, cpu);
    OpLdx(cpu, 0x3682u);
    OpLdy(cpu, 0xf945u);
    if (!ResultsCall(&results, 0x84d3f6u, 0x808878u))
        return results.result;
    OpLda(memory, cpu, OpAbs(cpu, BOARD_STAKE));
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0x359cu);
    OpLdy(cpu, 0xf932u);
    if (!ResultsCall(&results, 0x84d405u, 0x808878u))
        return results.result;
    SetBoardFrameFlags(memory, cpu);
    if (!ResultsCall(&results, 0x84d40fu, 0x84cbcdu))
        return results.result;
    UnpackStatus(cpu, Pull8(memory, cpu));
    return results.result;
}

Lufia2ExecutionResult Lufia2BoardGameDrawStakeDigitSprites(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ResultsReady(cpu) || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84d414u);
    OpSta(memory, cpu, OpDp(cpu, STAKE_ATTRIBUTES));
    OpWriteX(memory, cpu, OpDp(cpu, STAKE_DIGIT_POSITION), cpu->y);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, STAKE_DIGIT_ROWS));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, STAKE_OAM_CURSOR)));
    do {
        OpLda(memory, cpu, OpDp(cpu, STAKE_DIGIT_POSITION));
        OpSta(memory, cpu, OpAbsY(cpu, BOARD_OAM));
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpSta(memory, cpu, OpAbsY(cpu, BOARD_OAM + 4u));
        for (unsigned column = 0u; column < 2u; ++column) {
            OpLda(memory, cpu, OpLongX(cpu, 0x84a611u));
            OpOra(memory, cpu, OpDp(cpu, STAKE_ATTRIBUTES));
            OpSta(memory, cpu, OpAbsY(cpu, (uint16_t)(BOARD_OAM + 2u + column * 4u)));
            OpInx(cpu);
            OpInx(cpu);
        }
        OpTya(cpu);
        cpu->carry = 0u;
        OpAdcValue(cpu, 8u);
        OpTay(cpu);
        OpLda(memory, cpu, OpDp(cpu, STAKE_DIGIT_POSITION));
        cpu->carry = 0u;
        OpAdcValue(cpu, 0x0800u);
        OpSta(memory, cpu, OpDp(cpu, STAKE_DIGIT_POSITION));
        OpStepMem(memory, cpu, OpDp(cpu, STAKE_DIGIT_ROWS), -1);
    } while (!cpu->negative);
    OpWriteX(memory, cpu, OpDp(cpu, STAKE_OAM_CURSOR), cpu->y);
    return ExecutionReturned(0x84d455u);
}

static void CalculateBoardPayout(const Lufia2Memory *memory,
                                 Lufia2CpuState *cpu) {
    OpStz(memory, cpu, OpDp(cpu, 0u));
    OpLdx(cpu, 0u);
    OpLdy(cpu, 12u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu, BOARD_LINE_TOTALS));
        OpCmpValue(cpu, 101u);
        if (!cpu->negative) {
            OpLoadA(cpu, 2u);
            OpStepMem(memory, cpu, OpDp(cpu, 0u), 1);
            OpStepMem(memory, cpu, OpDp(cpu, 0u), 1);
            OpLda(memory, cpu, OpAbsX(cpu, BOARD_LINE_TOTALS));
            OpCmpValue(cpu, 108u);
            if (!cpu->negative) {
                OpStepMem(memory, cpu, OpDp(cpu, 0u), 1);
                OpStepMem(memory, cpu, OpDp(cpu, 0u), 1);
            }
        }
        OpInx(cpu);
        OpInx(cpu);
        OpDey(cpu);
    } while (!cpu->zero);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_BONUS));
    if (cpu->zero) {
        OpLda(memory, cpu, OpDp(cpu, 0u));
    } else {
        OpLda(memory, cpu, OpDp(cpu, 0u));
        if (cpu->zero) {
            OpStepMem(memory, cpu, OpDp(cpu, 0u), 1);
            OpIncA(cpu);
        }
        cpu->carry = 0u;
        RolA16(cpu);
        RolA16(cpu);
        cpu->carry = 0u;
        OpAdc(memory, cpu, OpDp(cpu, 0u));
    }
    OpSta(memory, cpu, OpAbs(cpu, BOARD_BONUS));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, BOARD_COMPLETED_LINES)));
    OpLda(memory, cpu, OpAbs(cpu, BOARD_STAKE));
    for (;;) {
        OpDey(cpu);
        if (cpu->zero)
            break;
        cpu->carry = 0u;
        RolA16(cpu);
    }
    OpSta(memory, cpu, OpDp(cpu, 0u));
    OpLdy(cpu, OpReadX(memory, cpu, OpAbs(cpu, BOARD_BONUS)));
    if (!cpu->zero) {
        OpDey(cpu);
        do {
            cpu->carry = 0u;
            OpAdc(memory, cpu, OpDp(cpu, 0u));
            OpDey(cpu);
        } while (!cpu->zero);
    }
    cpu->carry = 0u;
    RolA16(cpu);
    OpSta(memory, cpu, OpDp(cpu, 0u));
}

Lufia2ExecutionResult Lufia2BoardGamePayCompletedLines(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ResultsReady(cpu))
        return ExecutionHandoff(cpu, 0x84d456u);
    BoardResults results = {memory, cpu, child, context,
                            ExecutionReturned(0x84d537u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    if (!ResultsCall(&results, 0x84d459u, 0x84a39bu) ||
        !ResultsCall(&results, 0x84d45cu, 0x84a2b3u))
        return results.result;
    CalculateBoardPayout(memory, cpu);
    if (!ResultsCall(&results, 0x84d4bfu, 0x84a39bu))
        return results.result;
    OpLoadA(cpu, 5u);
    OpSta(memory, cpu, OpAbs(cpu, BOARD_WINDOW_HEIGHT));
    if (!ResultsCall(&results, 0x84d4c8u, 0x84a2b3u))
        return results.result;
    OpLdx(cpu, 0x3504u);
    OpLdy(cpu, 0xf9f1u);
    if (!ResultsCall(&results, 0x84d4d1u, 0x808878u))
        return results.result;
    OpLdx(cpu, 0x3682u);
    OpLdy(cpu, 0xf945u);
    if (!ResultsCall(&results, 0x84d4dbu, 0x808878u))
        return results.result;
    SetBoardFrameFlags(memory, cpu);
    do {
        if (!ResultsCall(&results, 0x84d4e5u, 0x84cbcdu) ||
            !ResultsCall(&results, 0x84d4e8u, 0x849f1au))
            return results.result;
        OpCmpValue(cpu, 0u);
    } while (cpu->zero);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 10000u);
    OpSta(memory, cpu, OpAbs(cpu, BOARD_PAYOUT_STEP));
    for (;;) {
        OpLda(memory, cpu, OpDp(cpu, 0u));
        cpu->carry = 1u;
        OpSbcValue(cpu, OpReadM(memory, cpu, 0x7e0000u | BOARD_PAYOUT_STEP));
        if (cpu->negative) {
            OpLda(memory, cpu, OpAbs(cpu, BOARD_PAYOUT_STEP));
            OpCmpValue(cpu, 1u);
            if (cpu->zero)
                break;
            if (!ResultsCall(&results, 0x84d508u, 0x84a3fcu))
                return results.result;
        } else {
            OpSta(memory, cpu, OpDp(cpu, 0u));
            if (!ResultsCall(&results, 0x84d50fu, 0x84a421u))
                return results.result;
            OpLdx(cpu, 0x3504u);
            OpLdy(cpu, 0xf9f1u);
            if (!ResultsCall(&results, 0x84d518u, 0x808878u))
                return results.result;
            OpLdx(cpu, 0x3682u);
            OpLdy(cpu, 0xf945u);
            if (!ResultsCall(&results, 0x84d522u, 0x808878u))
                return results.result;
            SetBoardFrameFlags(memory, cpu);
            if (!ResultsCall(&results, 0x84d52cu, 0x84cbcdu) ||
                !ResultsCall(&results, 0x84d52fu, 0x84cbcdu))
                return results.result;
            OpRepWidths(cpu, 0x20u);
        }
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return results.result;
}

Lufia2ExecutionResult Lufia2BoardGameCreditPayoutStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ResultsReady(cpu))
        return ExecutionHandoff(cpu, 0x84a421u);
    BoardResults results = {memory, cpu, child, context,
                            ExecutionReturned(0x84a454u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLoadA(cpu, 0x22u);
    if (!ResultsCall(&results, 0x84a427u, 0x84a607u))
        return results.result;
    OpLda(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, BOARD_PAYOUT_STEP));
    if (cpu->carry) {
        OpSepWidths(cpu, 0x20u);
        OpStepMem(memory, cpu, OpAbs(cpu, BOARD_BALANCE + 2u), 1);
        OpRepWidths(cpu, 0x20u);
    }
    OpSta(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
    OpLda(memory, cpu, OpAbs(cpu, BOARD_BALANCE + 2u));
    OpAndValue(cpu, 0xffu);
    OpCmpValue(cpu, 0x98u);
    if (!cpu->negative) {
        OpLoadA(cpu, 0x967fu);
        OpCmp(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
        if (!cpu->carry)
            OpSta(memory, cpu, OpAbs(cpu, BOARD_BALANCE));
    }
    UnpackStatus(cpu, Pull8(memory, cpu));
    return results.result;
}

Lufia2ExecutionResult Lufia2BoardGameRebuildCellLocations(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (!ResultsReady(cpu))
        return ExecutionHandoff(cpu, 0x84d538u);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLdy(cpu, 0u);
    OpLoadA(cpu, 25u);
    OpSta(memory, cpu, OpDp(cpu, 6u));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, BOARD_CELLS));
        OpAndValue(cpu, 0xffu);
        cpu->carry = 0u;
        RolA16(cpu);
        OpTax(cpu);
        OpTya(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_CELL_LOCATIONS));
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, 6u), -1);
    } while (!cpu->zero);
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84d557u);
}

static void AdvanceBoardMarkers(const Lufia2Memory *memory,
                                Lufia2CpuState *cpu) {
    OpLdy(cpu, 6u);
    OpLoadA(cpu, 2u);
    OpSta(memory, cpu, OpDp(cpu, PARTICLE_POSITION));
    do {
        OpLda(memory, cpu, OpAbsY(cpu, BOARD_MARKERS));
        if (!cpu->negative) {
            OpTax(cpu);
            OpLoadA(cpu, 0x2480u);
            OpSta(memory, cpu, OpAbsX(cpu, BOARD_OAM));
            OpTxa(cpu);
            cpu->carry = 1u;
            OpSbcValue(cpu, 4u);
            OpTax(cpu);
            OpCmpValue(cpu, 10u);
            if (cpu->negative)
                OpLdx(cpu, 0x6au);
            OpLoadA(cpu, 0x2880u);
            OpSta(memory, cpu, OpAbsX(cpu, BOARD_OAM));
            OpTxa(cpu);
            OpSta(memory, cpu, OpAbsY(cpu, BOARD_MARKERS));
            OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTICLE_FIRST)));
        }
        OpIny(cpu);
        OpIny(cpu);
        OpStepMem(memory, cpu, OpDp(cpu, PARTICLE_POSITION), -1);
    } while (!cpu->zero);
}

static void MoveParticleHorizontally(const Lufia2Memory *memory,
                                    Lufia2CpuState *cpu) {
    OpLda(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_POSITIONS));
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpLongX(cpu, 0x7e0000u | BOARD_PARTICLE_X_STEPS));
    if (cpu->negative) {
        OpRepWidths(cpu, 0x20u);
        OpAndValue(cpu, 0xff00u);
        OpTay(cpu);
        OpLda(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_X_STEPS));
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffu));
        OpIncA(cpu);
        OpSta(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_X_STEPS));
        OpTya(cpu);
    } else {
        OpCmpValue(cpu, 80u);
        OpRepWidths(cpu, 0x20u);
        if (!cpu->negative) {
            OpAndValue(cpu, 0xff00u);
            cpu->carry = 0u;
            OpAdcValue(cpu, 80u);
            OpTay(cpu);
            OpLda(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_X_STEPS));
            LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffu));
            OpIncA(cpu);
            OpAndValue(cpu, 0xffu);
            OpSta(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_X_STEPS));
            OpTya(cpu);
        }
    }
    OpSta(memory, cpu, OpDp(cpu, PARTICLE_POSITION));
}

static bool BounceBoardParticle(BoardResults *results) {
    const Lufia2Memory *memory = results->memory;
    Lufia2CpuState *cpu = results->cpu;
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpDp(cpu, PARTICLE_POSITION));
    OpLoadA(cpu, 4u);
    if (!ResultsCall(results, 0x84d5f8u, 0x808299u))
        return false;
    OpIncA(cpu);
    OpTay(cpu);
    OpLoadA(cpu, 2u);
    if (!ResultsCall(results, 0x84d601u, 0x808299u))
        return false;
    OpDecA(cpu);
    if (!cpu->zero) {
        OpTya(cpu);
        LoadA16(cpu, (uint16_t)(cpu->accumulator ^ 0xffu));
        OpIncA(cpu);
        OpAndValue(cpu, 0xffu);
        OpTay(cpu);
    }
    OpTya(cpu);
    OpSta(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_X_STEPS));
    OpLoadA(cpu, 16u);
    if (!ResultsCall(results, 0x84d61bu, 0x808299u))
        return false;
    OpTay(cpu);
    OpLoadA(cpu, 7u);
    if (!ResultsCall(results, 0x84d623u, 0x808299u))
        return false;
    OpDey(cpu);
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
    OpSta(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_Y_STEPS));
    OpLda(memory, cpu, OpDp(cpu, PARTICLE_POSITION));
    return true;
}

static bool MoveParticleVertically(BoardResults *results) {
    const Lufia2Memory *memory = results->memory;
    Lufia2CpuState *cpu = results->cpu;
    OpLda(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_Y_STEPS));
    OpIncA(cpu);
    OpAndValue(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_Y_STEPS));
    OpSepWidths(cpu, 0x20u);
    OpCmpValue(cpu, 128u);
    RorA8(cpu);
    OpRepWidths(cpu, 0x20u);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpDp(cpu, PARTICLE_POSITION));
    return cpu->negative || BounceBoardParticle(results);
}

static void WriteBoardParticleSprite(const Lufia2Memory *memory,
                                     Lufia2CpuState *cpu) {
    OpSta(memory, cpu, OpAbsX(cpu, BOARD_PARTICLE_POSITIONS));
    OpLdy(cpu, OpReadX(memory, cpu, OpDp(cpu, PARTICLE_OAM_CURSOR)));
    OpSepWidths(cpu, 0x20u);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x98u);
    ExchangeAccumulatorBytes(cpu);
    cpu->carry = 0u;
    OpAdcValue(cpu, 0x88u);
    ExchangeAccumulatorBytes(cpu);
    OpRepWidths(cpu, 0x20u);
    OpSta(memory, cpu, OpAbsY(cpu, BOARD_OAM));
    for (unsigned byte = 0u; byte < 4u; ++byte)
        OpIny(cpu);
    OpWriteX(memory, cpu, OpDp(cpu, PARTICLE_OAM_CURSOR), cpu->y);
}

Lufia2ExecutionResult Lufia2BoardGameAnimateParticles(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!child || !ResultsReady(cpu))
        return ExecutionHandoff(cpu, 0x84d558u);
    BoardResults results = {memory, cpu, child, context,
                            ExecutionReturned(0x84d683u), 0u};
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, BOARD_PARTICLE_OAM));
    OpSta(memory, cpu, OpDp(cpu, PARTICLE_OAM_CURSOR));
    OpLda(memory, cpu, OpAbs(cpu, BOARD_PARTICLE_FIRST));
    cpu->carry = 0u;
    RolA16(cpu);
    OpTax(cpu);
    OpSta(memory, cpu, OpDp(cpu, PARTICLE_FIRST));
    AdvanceBoardMarkers(memory, cpu);
    unsigned particles = 0u;
    for (;;) {
        if (particles++ == BOARD_STEP_LIMIT)
            return ExecutionHandoff(cpu, 0x84d59cu);
        MoveParticleHorizontally(memory, cpu);
        if (!MoveParticleVertically(&results))
            return results.result;
        WriteBoardParticleSprite(memory, cpu);
        OpInx(cpu);
        OpInx(cpu);
        OpTxa(cpu);
        OpCmpValue(cpu, 50u);
        if (cpu->zero)
            OpLdx(cpu, 0u);
        OpTxa(cpu);
        OpCmp(memory, cpu, OpDp(cpu, PARTICLE_FIRST));
        if (cpu->zero)
            break;
    }
    OpInx(cpu);
    OpInx(cpu);
    OpTxa(cpu);
    OpCmpValue(cpu, 50u);
    if (cpu->zero)
        OpLoadA(cpu, 0u);
    cpu->carry = 0u;
    RorA16(cpu);
    OpSta(memory, cpu, OpAbs(cpu, BOARD_PARTICLE_FIRST));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return results.result;
}
