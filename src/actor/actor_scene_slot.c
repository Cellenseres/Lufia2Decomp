#include "actor/actor_internal.h"
#include "core/cpu_ops.h"
#include "system/wram.h"

Lufia2ExecutionResult Lufia2ActorPrepareAuxiliarySlot(const Lufia2Memory *memory,
                                                      Lufia2CpuState *cpu) {
    if (cpu->program_bank != 0x83u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page || cpu->x >= 40u ||
        cpu->stack < 0x1f10u || cpu->stack > 0x1ffcu)
        return ExecutionHandoff(cpu, 0x83c110u);
    OpWriteX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT), cpu->x);
    SimulateJslFrame(memory, cpu, 0x83u, 0xc115u);
    Lufia2ActorRecordOffsets(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    OpLoadA(cpu, 0xfeu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E05D2));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E066A));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_UNK_7E1471));
    OpLda(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT));
    OpSta(memory, cpu, 0x7fd0a2u);
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_FACING));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FACING));
    OpLoadA(cpu, 0x20u);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_UNK_7FE316));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpAndValue(cpu, 0xfbu);
    OpOraValue(cpu, 0x22u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_STATE));
    OpLda(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    OpOraValue(cpu, 0x10u);
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_FLAGS));
    LoadA16(cpu, cpu->direct_page);
    OpSta(memory, cpu, OpLongX(cpu, WRAM_ACTOR_PRIMARY_TIMER));
    OpLdx(cpu, OpReadX(memory, cpu, OpDp(cpu, DP_ACTOR_SLOT)));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_X));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_X));
    OpLda(memory, cpu, OpAbs(cpu, WRAM_ACTOR_TILE_Y));
    OpSta(memory, cpu, OpAbsX(cpu, WRAM_ACTOR_TILE_Y));
    SimulateJslFrame(memory, cpu, 0x83u, 0xc15fu);
    Lufia2ActorSyncFinePosition(memory, cpu);
    SimulateRtlFrame(memory, cpu);
    return ExecutionReturned(0x83c160u);
}
