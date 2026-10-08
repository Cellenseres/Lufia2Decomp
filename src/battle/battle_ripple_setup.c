#include "core/child_call.h"
#include "core/cpu_ops.h"
#include "core/snes_registers.h"
#include "system/wram.h"
#include "lufia2/battle.h"

enum {
    RIPPLE_PHASE = WRAM_BATTLE_RIPPLE_PHASE & 0xffffu,
    RIPPLE_STATE = WRAM_UNK_7E1B19 & 0xffffu,
    RIPPLE_HDMA_MODE = WRAM_BATTLE_RIPPLE_HDMA_MODE & 0xffffu,
    RIPPLE_HDMA_REGISTER = WRAM_BATTLE_RIPPLE_HDMA_REGISTER & 0xffffu,
    RIPPLE_HDMA_SOURCE = WRAM_BATTLE_RIPPLE_HDMA_SOURCE & 0xffffu,
    RIPPLE_HDMA_SOURCE_BANK = WRAM_BATTLE_RIPPLE_HDMA_SOURCE_BANK & 0xffffu,
    RIPPLE_HDMA_SOURCE_ADDRESS = 0x9f17u,
    DP_HDMA_ENABLE = 0xdau,
    DP_HDMA_UPDATE = 0xdbu
};

Lufia2ExecutionResult Lufia2BattleInitializeRipple(const Lufia2Memory *memory,
    Lufia2CpuState *cpu, Lufia2PushedChildCall child, void *context) {
    if (cpu->program_bank != 0x85u || !cpu->accumulator_is_8_bit ||
        cpu->index_is_8_bit || cpu->decimal || cpu->direct_page != 0u ||
        cpu->stack < 0x1f00u || cpu->stack > 0x1ffcu || !child)
        return ExecutionHandoff(cpu, 0x85a701u);
    OpLdy(cpu, 0u);
    OpTyx(cpu);
    OpWriteX(memory, cpu, OpAbs(cpu, RIPPLE_PHASE), cpu->y);
    if (!CallChildWithFrame(memory, cpu, child, context,
            0x85a708u, 0x85a736u, 2u, 0x85u)) {
        Lufia2ExecutionResult result = ExecutionReturned(0x85a708u);
        result.flow = LUFIA2_EXECUTION_CHILD_UNWOUND;
        return result;
    }
    OpLoadA(cpu, 0x42u);
    OpSta(memory, cpu, OpAbs(cpu, RIPPLE_HDMA_MODE));
    OpLoadA(cpu, 0x0du);
    OpSta(memory, cpu, OpAbs(cpu, RIPPLE_HDMA_REGISTER));
    OpLdx(cpu, RIPPLE_HDMA_SOURCE_ADDRESS);
    OpWriteX(memory, cpu, OpAbs(cpu, RIPPLE_HDMA_SOURCE), cpu->x);
    OpLoadA(cpu, 0x85u);
    OpSta(memory, cpu, OpAbs(cpu, RIPPLE_HDMA_SOURCE_BANK));
    OpLoadA(cpu, 0x7eu);
    OpSta(memory, cpu, OpAbs(cpu, SNES_DASB(0u)));
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_HDMA_ENABLE), 1u);
    OpStz(memory, cpu, OpAbs(cpu, RIPPLE_STATE));
    OpLoadA(cpu, 0xffu);
    OpSta(memory, cpu, OpAbs(cpu, WRAM_UNK_7E1B17));
    OpLoadA(cpu, 1u);
    OpTestBits(memory, cpu, OpDp(cpu, DP_HDMA_UPDATE), 1u);
    return ExecutionReturned(0x85a735u);
}
