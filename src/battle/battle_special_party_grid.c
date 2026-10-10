#include "lufia2/battle.h"
#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"

enum {
    GRID_GRAPHICS_BYTES = 0x60,
    DP_GRID_TILE = 0x00,
    DP_GRID_COLUMNS = 0x02,
    DP_GRID_ROWS = 0x03,
    DP_GRID_ATTRIBUTES = 0x04,
    DP_GRID_Y = 0x05,
    DP_GRID_X = 0x06,
    DP_GRID_OUTPUT = 0x08,
    DP_GRID_SLOT = 0x0b,
    DP_GRID_PARTY_COUNT = 0x0d
};

static void CopySpecialPartyGraphics(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpWriteX(memory, cpu, OpAbs(cpu, SNES_WMADDL), cpu->x);
    OpLoadA(cpu, 0x7fu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_WMADDH));
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpLdx(cpu, 0u);
    do {
        OpLda(memory, cpu, OpAbsX(cpu,
            (uint16_t)WRAM_BATTLE_SPECIAL_PARTY_GRID_GRAPHICS_SOURCE));
        OpSta(memory, cpu, SNES_WMDATA);
        OpInx(cpu);
        OpCpx(cpu, GRID_GRAPHICS_BYTES);
    } while (!cpu->zero);
}

static void ConfigureSpecialPartyGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu) {
    OpLdx(cpu, OpReadX(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_OAM_CURSOR)));
    OpWriteX(memory, cpu, OpDp(cpu, DP_GRID_OUTPUT), cpu->x);
    OpLdx(cpu, 0u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_GRID_SLOT), cpu->x);
    OpStz(memory, cpu, OpAbs(cpu, (WRAM_SYSTEM_MULTIPLY_PRODUCT + 3u)));
    OpStz(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_OAM_COUNT));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_ENEMY_INITIAL_COUNT));
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_PARTY_COUNT));
    OpLoadA(cpu, 8u);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_COLUMNS));
    OpLoadA(cpu, 7u);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_ROWS));
    OpLoadA(cpu, 1u);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_Y));
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x58u);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_X));
    OpSepWidths(cpu, 0x20u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_PALETTE_PHASE));
    OpAndValue(cpu, 7u);
    OpAslA(cpu);
    OpOraValue(cpu, 0x21u);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_ATTRIBUTES));
    OpLoadA(cpu, 0u);
    OpSta(memory, cpu, OpDp(cpu, DP_GRID_TILE));
}

Lufia2ExecutionResult Lufia2BattlePrepareSpecialPartyGrid(
    const Lufia2Memory *memory, Lufia2CpuState *cpu,
    Lufia2PushedChildCall child, void *context) {
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit ||
        cpu->direct_page || cpu->decimal)
        return ExecutionHandoff(cpu, 0x8597d1u);
    OpLda(memory, cpu, OpAbs(cpu, WRAM_BATTLE_EFFECT_SPECIAL_PARTY));
    if (cpu->zero)
        return ExecutionReturned(0x8597d6u);
    PushDataBank(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLoadA(cpu, 9u);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_PALETTE_PHASE));
    OpLdx(cpu, (uint16_t)WRAM_BATTLE_SPECIAL_PARTY_GRID_GRAPHICS_A_LONG);
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_SPRITE_REBUILD_STATE));
    CopySpecialPartyGraphics(memory, cpu);
    Push8(memory, cpu, cpu->program_bank);
    PullDataBank(memory, cpu);
    OpLdx(cpu, (uint16_t)WRAM_BATTLE_SPECIAL_PARTY_GRID_GRAPHICS_B_LONG);
    CopySpecialPartyGraphics(memory, cpu);
    ConfigureSpecialPartyGrid(memory, cpu);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85985cu, 0x859884u, 3u, 0x85u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x85985cu, 0u};
    if (!cpu->accumulator_is_8_bit || cpu->index_is_8_bit)
        return ExecutionHandoff(cpu, 0x859860u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_GRID_SLOT)));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_SPECIAL_PARTY_SLOT_OAM_COUNT));
    OpStepMem(memory, cpu, OpDp(cpu, DP_GRID_SLOT), 1);
    cpu->carry = 0u;
    OpAdc(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_OAM_COUNT));
    OpSta(memory, cpu, OpAbs(cpu, WRAM_BATTLE_PARTY_OAM_COUNT));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_BATTLE_SPECIAL_PARTY_SLOT_OAM_TOTAL));
    OpLoadA(cpu, 0x7eu);
    PushAccumulator8(memory, cpu);
    PullDataBank(memory, cpu);
    OpRepWidths(cpu, 0x20u);
    OpLoadA(cpu, 0x37f0u);
    OpLdx(cpu, 0x2b96u);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85987du, 0x859790u, 2u, 0x85u))
        return (Lufia2ExecutionResult){LUFIA2_EXECUTION_CHILD_UNWOUND, 0x85987du, 0u};
    OpSepWidths(cpu, 0x20u);
    PullDataBank(memory, cpu);
    return ExecutionReturned(0x859883u);
}
