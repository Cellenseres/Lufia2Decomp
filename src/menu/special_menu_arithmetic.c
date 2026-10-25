#include "core/cpu_ops.h"
#include "lufia2/special_menu.h"
#include "core/snes_registers.h"
#include "lufia2/card_game.h"
#include "lufia2/board_game.h"
#include "system/wram.h"
#include "menu/arithmetic_wait.h"

enum {
    MATH_QUOTIENT = 0u,
    MATH_TENS_PRODUCT = 2u,
    DRAW_SIGN = 0x22u,
    DRAW_PACKED_FACTOR = 0x26u
};

static void DivideByTen(const Lufia2Memory *memory, Lufia2CpuState *cpu,
                         uint32_t first_nop) {
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, SNES_WRDIVB);
    WaitForArithmetic(memory, cpu, first_nop, 8u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, SNES_RDDIVL);
}

Lufia2ExecutionResult Lufia2SpecialMenuDividePayoutStep(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u)
        return ExecutionHandoff(cpu, 0x84a3fcu);
    Push8(memory, cpu, PackStatus(cpu));
    OpRepWidths(cpu, 0x30u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_SPECIAL_MENU_PAYOUT_STEP & 0xffffu));
    OpSta(memory, cpu, SNES_WRDIVL);
    DivideByTen(memory, cpu, 0x84a40eu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_SPECIAL_MENU_PAYOUT_STEP & 0xffffu));
    UnpackStatus(cpu, Pull8(memory, cpu));
    return ExecutionReturned(0x84a420u);
}

Lufia2ExecutionResult Lufia2SpecialMenuDivideScore(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84ae43u);
    OpSta(memory, cpu, SNES_WRDIVL);
    DivideByTen(memory, cpu, 0x84ae4fu);
    return ExecutionReturned(0x84ae5du);
}

Lufia2ExecutionResult Lufia2CardGameSplitStakeTens(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u || cpu->accumulator_is_8_bit)
        return ExecutionHandoff(cpu, 0x84c31bu);
    OpSta(memory, cpu, SNES_WRDIVL);
    DivideByTen(memory, cpu, 0x84c327u);
    OpSta(memory, cpu, OpDp(cpu, MATH_QUOTIENT));
    OpSepWidths(cpu, 0x20u);
    OpSta(memory, cpu, SNES_WRMPYA);
    OpLoadA(cpu, 10u);
    OpSta(memory, cpu, SNES_WRMPYB);
    WaitForArithmetic(memory, cpu, 0x84c343u, 4u);
    OpRepWidths(cpu, 0x20u);
    OpLda(memory, cpu, SNES_RDMPYL);
    OpSta(memory, cpu, OpDp(cpu, MATH_TENS_PRODUCT));
    return ExecutionReturned(0x84c34fu);
}

Lufia2ExecutionResult Lufia2BoardGameScaleDraw(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x84u || cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x84ceb3u);
    OpStz(memory, cpu, OpDp(cpu, DRAW_SIGN));
    OpBitValue(cpu, 0x80u);
    if (!cpu->zero) {
        OpAndValue(cpu, 0x7fu);
        OpStepMem(memory, cpu, OpDp(cpu, DRAW_SIGN), 1);
    }
    ExchangeAccumulatorBytes(cpu);
    OpSta(memory, cpu, OpDp(cpu, DRAW_PACKED_FACTOR));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BOARD_GAME_DRAW_RADIUS & 0xffffu));
    OpOra(memory, cpu, OpDp(cpu, DRAW_PACKED_FACTOR));
    OpTay(cpu);
    PushDataBank(memory, cpu);
    OpSepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0u);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpTya(cpu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WRMPYA));
    WaitForArithmetic(memory, cpu, 0x84ced5u, 4u);
    OpLda(memory, cpu, OpAbs(cpu, SNES_RDMPYL));
    PullDataBank(memory, cpu);
    cpu->carry = 0u;
    RolA16(cpu);
    ExchangeAccumulatorBytes(cpu);
    OpAndValue(cpu, 0xffu);
    return ExecutionReturned(0x84cee3u);
}
